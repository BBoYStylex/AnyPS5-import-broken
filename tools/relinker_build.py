import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import platform
import shutil
import subprocess
import sys
import tempfile
import time


VERSION = 1
STATE = ".anyps5-build.json"
TARGETS = {"linux": [], "windows": ["--windows"], "macos": ["--macos"]}
FLAGS = {"--skip-syscall-check", "--skip-sce-module", "--to-intel",
         "--to-rosetta", "--registry", "--lazy-binding",
         "--windows-diagnostics", "--windows-gui"}
VALUES = {"--sce-module-path", "--module-dir", "--exclude-sce-module", "--rpath"}


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(dir=path.parent, prefix=".write-")
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as stream:
            json.dump(value, stream, ensure_ascii=False, sort_keys=True)
            stream.write("\n")
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def inside(path, directory):
    return path == directory or directory in path.parents


def overlaps(left, right):
    return inside(left, right) or inside(right, left)


def relative_path(value):
    if not isinstance(value, str) or not value or "\\" in value:
        raise ValueError("Invalid artifact path")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in {".", ".."} for part in value.split("/")):
        raise ValueError("Invalid artifact path: " + value)
    if ":" in value or value == STATE:
        raise ValueError("Reserved artifact path: " + value)
    return path


def options_for(values, base, source):
    if not isinstance(values, list) or any(not isinstance(v, str) for v in values):
        raise ValueError("options must be a list of strings")
    result, module_dirs = [], []
    module_root = source.parent
    index = 0
    while index < len(values):
        option = values[index]
        if option in FLAGS or option in {"unused-filter=0", "unused-filter=1", "unused-filter=2"}:
            result.append(option)
        elif option in VALUES:
            index += 1
            if index == len(values) or not values[index] or values[index].startswith("--"):
                raise ValueError("Missing value for " + option)
            value = values[index]
            if option == "--sce-module-path":
                module_root = (base / value).resolve()
                value = str(module_root)
            if option == "--module-dir":
                directory = Path(value)
                if directory.is_absolute() or ".." in directory.parts:
                    raise ValueError("--module-dir must stay within the module root")
                module_dirs.append(directory)
            result.extend([option, value])
        else:
            raise ValueError("Unsupported batch option: " + option)
        index += 1
    dependencies = [source.parent / "sce_sys" / name for name in ("param.json", "icon0.png")]
    if "--skip-sce-module" not in result:
        directories = [module_root / name for name in ("sce_module", "sce_modules", "prx")]
        directories.extend(module_root / directory for directory in module_dirs)
        dependencies.extend(directory.resolve() for directory in directories)
    return result, sorted(set(dependencies))


def load_jobs(manifest, cache, command):
    manifest = manifest.resolve()
    document = json.loads(manifest.read_text(encoding="utf-8"))
    if not isinstance(document, dict) or type(document.get("version")) is not int or document["version"] != VERSION:
        raise ValueError("Expected manifest version 1")
    entries = document.get("builds")
    if not isinstance(entries, list) or not entries:
        raise ValueError("builds must be a nonempty list")
    jobs, names = [], set()
    for entry in entries:
        if not isinstance(entry, dict) or set(entry) - {"id", "input", "output", "target", "options"}:
            raise ValueError("Invalid build entry")
        name = entry.get("id")
        if not isinstance(name, str) or not name or name in names:
            raise ValueError("Build ids must be nonempty and unique")
        names.add(name)
        target = entry.get("target", "linux")
        if not isinstance(target, str) or target not in TARGETS:
            raise ValueError("Unknown target: " + str(target))
        for field in ("input", "output"):
            if not isinstance(entry.get(field), str) or not entry[field]:
                raise ValueError("Missing " + field + " for " + name)
        source = (manifest.parent / entry["input"]).resolve()
        output = (manifest.parent / entry["output"]).resolve()
        if not source.is_file() or output.name == STATE:
            raise ValueError("Invalid input or output for " + name)
        options, dependencies = options_for(entry.get("options", []), manifest.parent, source)
        jobs.append({"id": name, "input": source, "output": output, "target": target,
                     "options": TARGETS[target] + options, "dependencies": dependencies})
    protected = [manifest] + [Path(value).resolve() for value in command if Path(value).is_file()]
    for job in jobs:
        protected.extend([job["input"], *job["dependencies"]])
    for index, job in enumerate(jobs):
        root = job["output"].parent
        if any(overlaps(path, root) for path in protected):
            raise ValueError("Output directory overlaps build inputs: " + str(root))
        if overlaps(root, cache):
            raise ValueError("Cache and output directories must be separate")
        if any(overlaps(root, other["output"].parent) for other in jobs[:index]):
            raise ValueError("Each build needs a separate output directory")
    if any(overlaps(path, cache) for path in protected):
        raise ValueError("Cache directory overlaps build inputs")
    return jobs


def snapshot(job, command):
    paths = [job["input"], *job["dependencies"]]
    paths.extend(Path(value).resolve() for value in command if Path(value).is_file())
    records = []
    for path in sorted(set(paths)):
        if path.is_dir():
            children = []
            for child in sorted(path.iterdir()):
                if not child.is_dir() and not child.is_file():
                    raise ValueError("Unsupported dependency: " + str(child))
                children.append([child.name, "directory" if child.is_dir() else digest(child)])
            records.append([str(path), "directory", children])
        elif path.is_file():
            records.append([str(path), "file", digest(path)])
        elif path.exists():
            raise ValueError("Unsupported dependency: " + str(path))
        else:
            records.append([str(path), "missing"])
    value = {"version": VERSION, "host": [sys.platform, platform.machine()],
             "command": command, "output": str(job["output"]),
             "options": job["options"], "inputs": records}
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode("utf-8")).hexdigest()


def inventory(root):
    result = {}
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise ValueError("Output directories cannot contain symlinks")
        if path.is_file() and path != root / STATE:
            name = path.relative_to(root).as_posix()
            relative_path(name)
            result[name] = {"sha256": digest(path), "mode": path.stat().st_mode & 0o777}
        elif not path.is_dir() and not path.is_file():
            raise ValueError("Unsupported output file: " + str(path))
    return result


def artifact_map(value):
    if not isinstance(value, dict) or not value:
        raise ValueError("Missing artifact inventory")
    for name, metadata in value.items():
        relative_path(name)
        if not isinstance(metadata, dict) or set(metadata) != {"sha256", "mode"}:
            raise ValueError("Invalid artifact metadata")
        checksum, mode = metadata["sha256"], metadata["mode"]
        if not isinstance(checksum, str) or len(checksum) != 64 or any(c not in "0123456789abcdef" for c in checksum):
            raise ValueError("Invalid artifact digest")
        if type(mode) is not int or mode < 0 or mode > 0o777:
            raise ValueError("Invalid artifact permissions")
    return value


def acquire_output(root):
    root.mkdir(parents=True, exist_ok=True)
    marker = root / STATE
    current = inventory(root)
    if marker.exists():
        state = json.loads(marker.read_text(encoding="utf-8"))
        if not isinstance(state, dict) or state.get("version") != VERSION or not isinstance(state.get("artifacts"), dict):
            raise ValueError("Invalid build ownership marker")
        for name in state["artifacts"]:
            relative_path(name)
        unknown = set(current) - set(state["artifacts"])
        if unknown:
            raise ValueError("Output directory contains unowned files: " + ", ".join(sorted(unknown)))
    elif current:
        raise ValueError("Output directory must be empty for the first build")
    write_json(marker, {"version": VERSION, "artifacts": current, "complete": False})
    return current


def read_cache(entry, key):
    try:
        if entry.is_symlink():
            return None
        data = json.loads((entry / "entry.json").read_text(encoding="utf-8"))
        if data.get("version") != VERSION or data.get("key") != key:
            return None
        artifacts = artifact_map(data.get("artifacts"))
        if inventory(entry / "files") != artifacts:
            return None
        return artifacts
    except (OSError, ValueError, TypeError, AttributeError):
        return None


def clear_owned(root, current):
    for name in current:
        path = root / name
        path.chmod(path.stat().st_mode | 0o200)
        path.unlink()


def restore(entry, root, artifacts):
    for name in artifacts:
        target = root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(entry / "files" / name, target)


def publish(cache, key, root, artifacts):
    cache.mkdir(parents=True, exist_ok=True)
    entry = cache / key
    temporary = Path(tempfile.mkdtemp(dir=cache, prefix=".entry-"))
    try:
        (temporary / "files").mkdir()
        for name in artifacts:
            target = temporary / "files" / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root / name, target)
        write_json(temporary / "entry.json", {"version": VERSION, "key": key, "artifacts": artifacts})
        if entry.is_symlink():
            raise ValueError("Cache entry cannot be a symlink")
        if entry.exists():
            shutil.rmtree(entry)
        os.replace(temporary, entry)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)


def build_one(job, cache, command, timeout):
    started = time.perf_counter()
    result = {"id": job["id"], "target": job["target"], "output": str(job["output"])}
    root = job["output"].parent
    lock = root.parent / (root.name + ".anyps5-lock")
    owned = locked = False
    try:
        root.parent.mkdir(parents=True, exist_ok=True)
        with lock.open("x", encoding="utf-8") as stream:
            stream.write(str(os.getpid()))
        locked = True
        current = acquire_output(root)
        owned = True
        key = snapshot(job, command)
        result["cache_key"] = key
        entry = cache / key
        artifacts = read_cache(entry, key)
        if artifacts is not None:
            if current != artifacts:
                clear_owned(root, current)
                restore(entry, root, artifacts)
            status = "cached"
        else:
            clear_owned(root, current)
            with tempfile.TemporaryFile() as log:
                completed = subprocess.run([*command, *job["options"], str(job["input"]), str(job["output"])],
                                           stdout=log, stderr=log, timeout=timeout)
                log.seek(0, os.SEEK_END)
                log.seek(max(0, log.tell() - 8192))
                diagnostics = log.read().decode("utf-8", errors="replace")
            if completed.returncode:
                raise ValueError("Relinker exited with " + str(completed.returncode) + ": " + diagnostics.strip())
            if not job["output"].is_file():
                raise ValueError("Relinker did not produce the requested executable")
            artifacts = inventory(root)
            if snapshot(job, command) != key:
                raise ValueError("Build inputs changed during conversion; result was not cached")
            publish(cache, key, root, artifacts)
            status = "built"
        if snapshot(job, command) != key:
            raise ValueError("Build inputs changed while restoring cached outputs")
        write_json(root / STATE, {"version": VERSION, "artifacts": artifacts, "complete": True, "key": key})
        result.update(status=status, artifacts=len(artifacts))
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        result.update(status="failed", error=str(error))
        if owned:
            try:
                write_json(root / STATE, {"version": VERSION, "artifacts": inventory(root), "complete": False})
            except (OSError, ValueError):
                pass
    finally:
        if locked:
            lock.unlink()
    result["seconds"] = round(time.perf_counter() - started, 6)
    return result


def run_builds(manifest, cache, command, workers=1, timeout=300):
    if type(workers) is not int or workers < 1 or not math.isfinite(timeout) or timeout <= 0:
        raise ValueError("jobs and timeout must be positive")
    command = [str(value) for value in command]
    if not command or not Path(command[0]).is_file():
        raise ValueError("Relinker executable does not exist")
    cache = Path(cache).resolve()
    jobs = load_jobs(Path(manifest), cache, command)
    with ThreadPoolExecutor(max_workers=min(workers, len(jobs))) as executor:
        results = list(executor.map(lambda job: build_one(job, cache, command, timeout), jobs))
    return {"version": VERSION, "builds": results,
            "summary": {status: sum(item["status"] == status for item in results)
                        for status in ("built", "cached", "failed")}}


def main():
    parser = argparse.ArgumentParser(description="Build independent relinker targets in parallel with a verified content cache.")
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--relinker", required=True, type=Path)
    parser.add_argument("--cache-dir", type=Path, default=Path(".anyps5-cache"))
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=300)
    args = parser.parse_args()
    try:
        result = run_builds(args.manifest, args.cache_dir, [str(args.relinker.resolve())], args.jobs, args.timeout)
    except (OSError, ValueError) as error:
        parser.exit(2, "relinker_build: " + str(error) + "\n")
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))
    return int(result["summary"]["failed"] != 0)


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    raise SystemExit(main())

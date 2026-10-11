import argparse
from collections import deque
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import signal
import statistics
import struct
import subprocess
import tempfile
import threading
import time


LOG_LIMIT = 65536
ARTIFACT_LIMIT = 32 * 1024 * 1024


def digest_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def collect_cases(inputs):
    grouped = {}
    for item in inputs:
        path = Path(item).resolve(strict=True)
        files = sorted(path.rglob("*.req")) if path.is_dir() else [path]
        if not files:
            raise ValueError(f"no .req captures in {path}")
        for source in files:
            source = source.resolve(strict=True)
            if not source.is_file() or source.suffix != ".req":
                raise ValueError(f"not a .req file: {source}")
            digest = digest_file(source)
            case = grouped.setdefault(digest, {"id": digest, "paths": []})
            if str(source) not in case["paths"]:
                case["paths"].append(str(source))
    if not grouped:
        raise ValueError("no shader captures supplied")
    return [dict(case, paths=sorted(case["paths"])) for _, case in sorted(grouped.items())]


def resolve_command(executable, arguments):
    located = shutil.which(str(executable))
    path = Path(located if located else executable).resolve(strict=True)
    if not path.is_file():
        raise ValueError(f"not an executable file: {path}")
    return {"command": [str(path), *arguments], "sha256": digest_file(path)}


def drain(stream, result):
    blocks = deque()
    size = 0
    total = 0
    try:
        while True:
            block = stream.read(4096)
            if not block:
                break
            total += len(block)
            blocks.append(block)
            size += len(block)
            while size > LOG_LIMIT:
                size -= len(blocks.popleft())
    finally:
        stream.close()
        result.update(text=b"".join(blocks).decode("utf-8", errors="replace"), bytes=total, truncated=total > size)


def artifact_info(path):
    size = path.stat().st_size
    if size < 20 or size % 4 or size > ARTIFACT_LIMIT:
        raise ValueError("SPIR-V artifact has invalid or excessive size")
    with path.open("rb") as artifact:
        magic, version, generator, bound, reserved = struct.unpack("<5I", artifact.read(20))
    if magic != 0x07230203 or reserved != 0 or bound == 0 or version & 0xff0000ff != 0 or version >> 16 != 1 or (version >> 8) & 255 > 6:
        raise ValueError("SPIR-V artifact has an invalid header")
    return {"sha256": digest_file(path), "words": size // 4, "version": version, "generator": generator}


def replay_once(case, build, timeout):
    with tempfile.TemporaryDirectory(prefix="anyps5-shader-") as directory:
        root = Path(directory).resolve()
        request = root / "request.req"
        shutil.copyfile(case["paths"][0], request)
        if digest_file(request) != case["id"]:
            raise ValueError(f"capture changed during discovery: {case['paths'][0]}")
        command = [*build["command"], "--spv", str(request)]
        stdout, stderr = {}, {}
        started = time.perf_counter()
        try:
            process = subprocess.Popen(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                       start_new_session=os.name == "posix")
        except OSError as error:
            return {"status": "launch_error", "seconds": time.perf_counter() - started, "error": str(error)}
        readers = [
            threading.Thread(target=drain, args=(process.stdout, stdout), daemon=True),
            threading.Thread(target=drain, args=(process.stderr, stderr), daemon=True),
        ]
        for reader in readers:
            reader.start()
        timed_out = False
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            if os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()
            process.wait()
        elapsed = time.perf_counter() - started
        for reader in readers:
            reader.join(timeout=1)
        result = {"status": "timeout" if timed_out else "passed" if process.returncode == 0 else "failed",
                  "returncode": process.returncode, "seconds": elapsed, "stdout": stdout, "stderr": stderr}
        if any(reader.is_alive() for reader in readers):
            result.update(status="failed", error="replay left a child holding an output pipe")
        if result["status"] == "passed":
            try:
                result["artifact"] = artifact_info(root / "request.req.spv")
            except (OSError, ValueError) as error:
                result.update(status="invalid_artifact", error=str(error))
        return result


def summarize(samples):
    passed = all(sample["status"] == "passed" for sample in samples)
    digests = sorted({sample["artifact"]["sha256"] for sample in samples if "artifact" in sample})
    return {"status": "passed" if passed else "failed", "median_seconds": statistics.median(
        sample["seconds"] for sample in samples), "artifact_hashes": digests,
        "stable_output": passed and len(digests) == 1, "samples": samples}


def compare_runs(baseline, candidate, slowdown_percent, minimum_ms):
    if baseline["status"] == "passed" and candidate["status"] != "passed":
        return {"classification": "regression", "slowdown": False, "output_changed": False}
    if baseline["status"] != "passed":
        return {"classification": "resolved" if candidate["status"] == "passed" else "unchanged_failure",
                "slowdown": False, "output_changed": False}
    old, new = baseline["median_seconds"], candidate["median_seconds"]
    return {"classification": "passed", "slowdown": new > old * (1 + slowdown_percent / 100)
            and (new - old) * 1000 >= minimum_ms,
            "output_changed": baseline["artifact_hashes"] != candidate["artifact_hashes"],
            "ratio": new / old if old > 0 else None, "delta_ms": (new - old) * 1000}


def run_case(case, candidate, baseline, repeats, timeout, slowdown_percent, minimum_ms):
    samples = {"candidate": [], "baseline": []}
    for index in range(repeats):
        order = [("baseline", baseline), ("candidate", candidate)] if index % 2 == 0 else [
            ("candidate", candidate), ("baseline", baseline)]
        for name, build in order:
            if build is not None:
                samples[name].append(replay_once(case, build, timeout))
    result = dict(case, candidate=summarize(samples["candidate"]))
    if baseline is not None:
        result["baseline"] = summarize(samples["baseline"])
        result["comparison"] = compare_runs(result["baseline"], result["candidate"], slowdown_percent, minimum_ms)
    return result


def run_corpus(cases, candidate, baseline=None, jobs=1, repeats=3, timeout=30,
               slowdown_percent=25, minimum_ms=25):
    if type(jobs) is not int or not 1 <= jobs <= 32 or type(repeats) is not int or not 1 <= repeats <= 9:
        raise ValueError("jobs must be 1 to 32 and repeats 1 to 9")
    if not math.isfinite(timeout) or not 0 < timeout <= 3600:
        raise ValueError("timeout must be positive and at most 3600 seconds")
    if not all(math.isfinite(value) and value >= 0 for value in (slowdown_percent, minimum_ms)):
        raise ValueError("slowdown thresholds must be finite and non-negative")
    if not cases:
        raise ValueError("no shader captures supplied")
    started = time.perf_counter()
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = [pool.submit(run_case, case, candidate, baseline, repeats, timeout, slowdown_percent, minimum_ms)
                   for case in cases]
        results = [future.result() for future in futures]
    for build in (candidate, baseline):
        if build is not None and digest_file(Path(build["command"][0])) != build["sha256"]:
            raise ValueError("replay executable changed while the corpus was running")
    counts = {"cases": len(results), "captures": sum(len(case["paths"]) for case in cases),
              "passed": sum(result["candidate"]["status"] == "passed" for result in results),
              "regressions": 0, "resolved": 0, "slowdowns": 0, "output_changes": 0, "unstable_outputs": 0}
    for result in results:
        counts["unstable_outputs"] += sum(build["status"] == "passed" and not build["stable_output"]
                                          for build in [result["candidate"], *([result["baseline"]] if baseline else [])])
        if baseline is not None:
            comparison = result["comparison"]
            counts["regressions"] += comparison["classification"] == "regression"
            counts["resolved"] += comparison["classification"] == "resolved"
            counts["slowdowns"] += comparison["slowdown"]
            counts["output_changes"] += comparison["output_changed"]
    return {"version": 1, "candidate": candidate, "baseline": baseline,
            "settings": {"jobs": jobs, "repeats": repeats, "timeout": timeout,
                         "slowdown_percent": slowdown_percent, "minimum_ms": minimum_ms},
            "seconds": time.perf_counter() - started, "summary": counts, "results": results}


def exit_code(report, fail_on_output_change=False):
    summary = report["summary"]
    failed = summary["regressions"] or summary["slowdowns"]
    if report["baseline"] is None:
        failed = summary["passed"] != summary["cases"]
    if fail_on_output_change:
        failed = failed or summary["output_changes"] or summary["unstable_outputs"]
    return 1 if failed else 0


def write_report(path, report):
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n", dir=path.parent, delete=False) as output:
            temporary = Path(output.name)
            json.dump(report, output, indent=2, allow_nan=False)
            output.write("\n")
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description="Compare shader request replay failures, SPIR-V output and compile time.")
    parser.add_argument("captures", nargs="+", type=Path)
    parser.add_argument("--replay", required=True)
    parser.add_argument("--replay-arg", action="append", default=[])
    parser.add_argument("--baseline")
    parser.add_argument("--baseline-arg", action="append", default=[])
    parser.add_argument("--jobs", type=int, default=1)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--slowdown-percent", type=float, default=25)
    parser.add_argument("--minimum-ms", type=float, default=25)
    parser.add_argument("--fail-on-output-change", action="store_true")
    parser.add_argument("--json", type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.baseline_arg and not args.baseline:
            raise ValueError("--baseline-arg requires --baseline")
        cases = collect_cases(args.captures)
        candidate = resolve_command(args.replay, args.replay_arg)
        baseline = resolve_command(args.baseline, args.baseline_arg) if args.baseline else None
        output = args.json.resolve()
        protected = [Path(path) for case in cases for path in case["paths"]]
        protected.extend(Path(build["command"][0]) for build in (candidate, baseline) if build)
        if any(output == path or (output.exists() and os.path.samefile(output, path)) for path in protected):
            raise ValueError("JSON output must be separate from captures and replay executables")
        report = run_corpus(cases, candidate, baseline, args.jobs, args.repeats, args.timeout,
                            args.slowdown_percent, args.minimum_ms)
        write_report(output, report)
    except (OSError, ValueError) as error:
        parser.exit(2, f"shader_corpus: {error}\n")
    summary = report["summary"]
    print(f"{summary['passed']}/{summary['cases']} passed; {summary['regressions']} regressions; "
          f"{summary['slowdowns']} slowdowns; {summary['output_changes']} SPIR-V changes")
    return exit_code(report, args.fail_on_output_change)


if __name__ == "__main__":
    raise SystemExit(main())

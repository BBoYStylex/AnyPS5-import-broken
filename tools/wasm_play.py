#!/usr/bin/env python3
"""wasm_play -- build a dumped PS5 title for the browser and serve it.

Runs the steps of docs/dev/WASM_RUNTIME.md in one command on Linux, macOS and Windows:

  1. extract the archive (.rar/.7z with 7-Zip, .zip also without it),
  2. relink the decrypted eboot.bin and its modules for Linux, finding module
     directories such as Unity's Media/Modules and Media/Plugins,
  3. place the title's files under app0/,
  4. translate the relinker output to game.wasm with anyps5-wasm-aot,
  5. build the runtime with Emscripten,
  6. package app0/ as game.data,
  7. start the local server and open Chrome when it is installed.

Steps whose outputs are up to date are skipped, so the second run starts the server at
once. Everything is written to build-wasm-aot/<title id>/; each step's output is in its
logs/ directory.

usage: wasm_play.py <archive or folder> [--port 8080] [--build-only] [--rebuild] [--allow-unsupported]

Environment: EMSDK (default ~/emsdk, or emcc on PATH), ANYPS5_RELINKER (a relinker
executable), ANYPS5_WASM_BUILD (default build-wasm-<os>).
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
HOST = {"win32": "windows", "darwin": "macos"}.get(sys.platform, "linux")
EXE = ".exe" if HOST == "windows" else ""
TITLE_ID = re.compile(r"[A-Z]{4}\d{5}")
MODULE_ROOTS = {"sce_module", "sce_modules", "prx"}
NOT_APP0 = MODULE_ROOTS | {"decrypted", "fakelib"}


class Failure(Exception):
    pass


def step(text):
    print(f"\n==> {text}", flush=True)


def magic(path, size=4):
    try:
        with open(path, "rb") as file:
            return file.read(size)
    except OSError:
        return b""


def is_elf(path):
    return magic(path) == b"\x7fELF"


def runs_here(path):
    """Whether an executable was built for this host, so a stale build directory of another OS is skipped."""
    head = magic(path)
    if HOST == "windows":
        return head[:2] == b"MZ"
    if HOST == "macos":
        return head in (b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe")
    return head == b"\x7fELF"


def stale(output, *inputs):
    if not output.exists():
        return True
    built = output.stat().st_mtime
    return any(path.stat().st_mtime > built for path in inputs)


def run(name, command, logs, cwd=None, env=None):
    """Runs a command with its output in logs/<name>.log and returns that output; prints its end on failure."""
    log = logs / f"{name}.log"
    with open(log, "w", encoding="utf-8", errors="replace") as file:
        result = subprocess.run([str(part) for part in command], cwd=cwd, env=env, stdout=file, stderr=subprocess.STDOUT)
    output = log.read_text(encoding="utf-8", errors="replace")
    if result.returncode != 0:
        print("\n".join(output.replace("\r", "\n").splitlines()[-30:]), file=sys.stderr)
        raise Failure(f"{name} failed (exit {result.returncode}), full log: {log}")
    return output


def require(program, hint):
    path = shutil.which(program)
    if not path:
        raise Failure(f"{program} not found: {hint}")
    return path


def seven_zip():
    for name in ("7z", "7zz", "7za"):
        path = shutil.which(name)
        if path:
            return path
    if HOST == "windows":
        for root in (os.environ.get("ProgramFiles"), os.environ.get("ProgramFiles(x86)")):
            if root and Path(root, "7-Zip", "7z.exe").is_file():
                return str(Path(root, "7-Zip", "7z.exe"))
    return None


def extract(archive, destination, logs):
    partial = destination.with_name(destination.name + ".partial")
    shutil.rmtree(partial, ignore_errors=True)
    tool = seven_zip()
    if tool:
        run("extract", [tool, "x", "-y", f"-o{partial}", archive], logs)
    elif archive.suffix.lower() == ".zip":
        with zipfile.ZipFile(archive) as file:
            file.extractall(partial)
    else:
        hints = {"windows": "install 7-Zip from https://www.7-zip.org", "macos": "brew install sevenzip", "linux": "sudo apt install 7zip"}
        raise Failure(f"7-Zip is needed to extract {archive.name}: {hints[HOST]}")
    partial.rename(destination)


def find_title(root, staging):
    """Returns the title folder (the one holding sce_sys/param.json) and the folder of its decrypted eboot.bin.

    Dumps keep the decrypted ELF files in decrypted/, in place of the originals, or beside the encrypted
    originals as <name>.esbak; the last are linked into `staging` under their own names.
    """
    params = [path for path in root.rglob("param.json") if path.parent.name == "sce_sys" and "decrypted" not in path.parts[len(root.parts):]]
    if not params:
        raise Failure(f"no sce_sys/param.json under {root}")
    title = min(params, key=lambda path: len(path.parts)).parent.parent
    for folder in (title / "decrypted", title):
        if is_elf(folder / "eboot.bin"):
            return title, folder
    if is_elf(title / "eboot.bin.esbak"):
        for backup in title.rglob("*.esbak"):
            name, relative = backup.name[: -len(".esbak")], backup.parent.relative_to(title)
            if is_elf(backup) and (Path(name).suffix.lower() in (".prx", ".sprx") or (name == "eboot.bin" and not relative.parts)):
                link(backup, staging / relative / name)
        return title, staging
    raise Failure(f"eboot.bin in {title} is encrypted (SELF); a decrypted dump is needed "
                  f"(a decrypted container can be unwrapped with tools/unwrap_self.py)")


def module_dirs(elf_root):
    """Folders of ELF modules besides sce_module/, for example Unity's Media/Modules and Media/Plugins."""
    found = set()
    for path in elf_root.rglob("*"):
        if path.suffix.lower() not in (".prx", ".sprx") or not path.is_file() or not is_elf(path):
            continue
        relative = path.parent.relative_to(elf_root)
        if relative.parts and relative.parts[0] not in MODULE_ROOTS | {"sce_sys"}:
            found.add(relative.as_posix())
    return sorted(found)


def find_relinker(logs):
    if os.environ.get("ANYPS5_RELINKER"):
        return Path(os.environ["ANYPS5_RELINKER"])
    for build in ("build-relinker", f"build-{HOST}", "build"):
        path = REPO / build / "core" / "relinker" / f"relinker{EXE}"
        if path.is_file() and runs_here(path):
            return path
    step("Building the relinker (docs/dev/BUILD.md, relinker only)")
    require("cmake", "install CMake 3.22.1 or newer")
    require("ninja", "install Ninja")
    build = REPO / "build-relinker"
    run("relinker-configure", ["cmake", "-S", REPO, "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DANYPS5_RELINKER_ONLY=ON"], logs)
    run("relinker-build", ["cmake", "--build", build, "--target", "relinker", "--parallel"], logs)
    return build / "core" / "relinker" / f"relinker{EXE}"


def emscripten():
    """Returns the Emscripten folder and an environment that lets its tools find their configuration."""
    env = dict(os.environ)
    roots = [Path(os.environ["EMSDK"])] if os.environ.get("EMSDK") else []
    roots.append(Path.home() / "emsdk")
    for root in roots:
        folder = root / "upstream" / "emscripten"
        if (folder / "emcmake.py").is_file():
            env["EMSDK"] = str(root)
            env["PATH"] = os.pathsep.join([str(folder), str(root), env.get("PATH", "")])
            if (root / ".emscripten").is_file():
                env["EM_CONFIG"] = str(root / ".emscripten")
            return folder, env
    emcc = shutil.which("emcc")
    if emcc:
        resolved = Path(emcc).resolve().parent
        for folder in (resolved, resolved.parent / "libexec"):
            if (folder / "emcmake.py").is_file():
                return folder, env
    raise Failure("Emscripten not found: install emsdk to ~/emsdk (or set EMSDK), see docs/dev/WASM_RUNTIME.md")


def same_file(source, target):
    try:
        a, b = source.stat(), target.stat()
    except OSError:
        return False
    return (a.st_ino == b.st_ino and a.st_dev == b.st_dev and a.st_ino != 0) or (a.st_size == b.st_size and int(a.st_mtime) == int(b.st_mtime))


def link(source, target):
    """Hard-links source to target, or copies it where links are not possible; returns whether target changed."""
    if same_file(source, target):
        return False
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        target.unlink()
    try:
        os.link(source, target)
    except OSError:
        shutil.copy2(source, target)
    return True


def place_app0(title, app0, modules):
    """Links the title's files into app0/, leaving out the original modules the relinker replaced."""
    changed = False
    skip_modules = {Path(folder) for folder in modules}
    for folder, folders, files in os.walk(title):
        relative = Path(folder).relative_to(title)
        if not relative.parts:
            folders[:] = [name for name in folders if name not in NOT_APP0]
        for name in files:
            if name.endswith(".esbak") or (relative in skip_modules and Path(name).suffix.lower() in (".prx", ".sprx")):
                continue
            changed |= link(Path(folder, name), app0 / relative / name)
    return changed


def build(source, rebuild, allow_unsupported):
    require("cargo", "install Rust from https://rustup.rs")
    name = TITLE_ID.search(source.name)
    work = REPO / "build-wasm-aot" / (name.group(0) if name else re.sub(r"[^A-Za-z0-9._-]+", "_", source.stem))
    logs, linux, web = work / "logs", work / "linux", work / "web"
    if rebuild:
        shutil.rmtree(linux, ignore_errors=True)
        shutil.rmtree(web, ignore_errors=True)
    for folder in (logs, linux / "app0", web):
        folder.mkdir(parents=True, exist_ok=True)

    root = source
    if source.is_file():
        root = work / "src"
        if not root.is_dir():
            step(f"Extracting {source.name}")
            extract(source, root, logs)
    title, elf_root = find_title(root, work / "elf")
    modules = module_dirs(elf_root)

    app = linux / "app.elf"
    relinked = stale(app, elf_root / "eboot.bin")
    if relinked:
        relinker = find_relinker(logs)
        step("Relinking for Linux" + (f" (module folders: {', '.join(modules)})" if modules else ""))
        arguments = [argument for folder in modules for argument in ("--module-dir", folder)]
        run("relink", [relinker, *arguments, elf_root / "eboot.bin", app], logs)

    changed = place_app0(title, linux / "app0", modules)

    wasm = web / "game.wasm"
    if stale(wasm, app):
        step("Translating to game.wasm (can take several minutes)")
        output = run("wasm-aot", ["cargo", "run", "--release", "--manifest-path", REPO / "core" / "wasm-aot" / "Cargo.toml",
                                  "--bin", "anyps5-wasm-aot", "--", app, "-o", wasm, "--report", work / "report.json",
                                  *(["--allow-unsupported"] if allow_unsupported else [])], logs)
        summary = re.search(r"^.* bytes of wasm$", output, re.M)
        if summary:
            print(output[summary.start():].rstrip())

    folder, env = emscripten()
    runtime_build = Path(os.environ.get("ANYPS5_WASM_BUILD", REPO / f"build-wasm-{HOST}"))
    cache = runtime_build / "CMakeCache.txt"
    if cache.is_file():
        toolchain = re.search(r"^CMAKE_TOOLCHAIN_FILE:\w+=(.*)$", cache.read_text(encoding="utf-8", errors="replace"), re.M)
        if toolchain and not Path(toolchain.group(1)).is_file():
            raise Failure(f"{runtime_build} was configured on another machine ({toolchain.group(1)} is missing); "
                          f"delete it or set ANYPS5_WASM_BUILD to another folder")
    else:
        step(f"Configuring the runtime in {runtime_build.name}")
        require("cmake", "install CMake 3.22.1 or newer")
        generator = ["-G", "Ninja"] if shutil.which("ninja") else []
        run("runtime-configure", [sys.executable, folder / "emcmake.py", "cmake", "-S", REPO, "-B", runtime_build, *generator,
                                  "-DANYPS5_BUILD_WASM_RUNTIME=ON", "-DCMAKE_BUILD_TYPE=Release",
                                  f"-DPython3_EXECUTABLE={sys.executable}"], logs, env=env)
    step("Building the runtime")
    run("runtime", ["cmake", "--build", runtime_build, "--target", "anyps5-runtime-web", "--parallel"], logs, env=env)

    data = web / "game.data.js"
    if relinked or changed or not data.is_file() or not (web / "game.data").is_file():
        size = sum(path.stat().st_size for path in (linux / "app0").rglob("*") if path.is_file())
        step(f"Packaging game data ({size / 2**20:.0f} MiB)")
        run("package", [sys.executable, folder / "tools" / "file_packager.py", web / "game.data", "--preload", "app0@/app0",
                        f"--js-output={data}", "--no-node"], logs, cwd=linux, env=env)
    print(f"\nBuilt {web}")
    return runtime_build / "core" / "wasm-runtime", web


def browser():
    """Returns a command that opens a URL in Chrome (or Edge on Windows), with the WebGPU switches Linux needs."""
    if HOST == "macos":
        return ["open", "-a", "Google Chrome"] if Path("/Applications/Google Chrome.app").is_dir() else None
    if HOST == "windows":
        roots = [os.environ.get(name) for name in ("ProgramFiles", "ProgramFiles(x86)", "LOCALAPPDATA")]
        for relative in (Path("Google", "Chrome", "Application", "chrome.exe"), Path("Microsoft", "Edge", "Application", "msedge.exe")):
            for root in filter(None, roots):
                if Path(root, relative).is_file():
                    return [str(Path(root, relative))]
        return None
    if not (os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        return None
    for name in ("google-chrome", "google-chrome-stable", "chromium", "chromium-browser"):
        if shutil.which(name):
            profile = REPO / "build-wasm-aot" / "chrome-profile"
            return [name, f"--user-data-dir={profile}", "--enable-unsafe-webgpu", "--enable-features=Vulkan"]
    return None


def serve(runtime, web, port):
    url = f"http://127.0.0.1:{port}/"
    opener = browser()
    print(f"\nOpen {url} in Chrome and click the page once to start audio.")
    print(f"From another computer, forward the port first: ssh -N -L {port}:127.0.0.1:{port} <user>@<this computer>")
    if HOST == "linux":
        print("Chrome on Linux needs --enable-unsafe-webgpu --enable-features=Vulkan.")
    print("Stop the server with Ctrl+C.\n", flush=True)
    server = subprocess.Popen([sys.executable, REPO / "core" / "wasm-runtime" / "web" / "serve.py",
                               "--runtime", runtime, "--game", web, "--port", str(port)])
    try:
        time.sleep(1)
        if opener and server.poll() is None:
            subprocess.Popen([*opener, url], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return server.wait()
    except KeyboardInterrupt:
        server.terminate()
        return 0


def main():
    parser = argparse.ArgumentParser(description="Builds a dumped PS5 title for the browser and serves it (docs/dev/WASM_RUNTIME.md)")
    parser.add_argument("source", type=Path, help=".rar/.7z/.zip of the title, or its extracted folder")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--build-only", action="store_true", help="build without starting the server")
    parser.add_argument("--rebuild", action="store_true", help="redo the relink, translation and packaging; the extracted archive is kept")
    parser.add_argument("--allow-unsupported", action="store_true",
                        help="translate instructions anyps5-wasm-aot does not support to traps that stop the game only if it runs them")
    arguments = parser.parse_args()
    source = arguments.source.resolve()
    if not source.exists():
        parser.error(f"{source} does not exist")
    try:
        runtime, web = build(source, arguments.rebuild, arguments.allow_unsupported)
    except Failure as error:
        print(f"wasm_play: {error}", file=sys.stderr)
        return 1
    return 0 if arguments.build_only else serve(runtime, web, arguments.port)


if __name__ == "__main__":
    sys.exit(main())

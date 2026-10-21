# WebAssembly runtime

Runs a relinked title in a browser with WebGPU.

## Parts

- [core/wasm-aot](../../core/wasm-aot): `anyps5-wasm-aot` (Rust). Translates the relinker's Linux output to one `game.wasm`: `app.elf`, the guest modules it needs, and every other converted module (`*.guest.prx`) beside it, which the title can load at run time. A module `app.elf` does not need runs its initializers, and so its `module_start`, when the title first opens it with `sceKernelLoadStartModule` or `dlopen`.
- [core/wasm-runtime](../../core/wasm-runtime): Emscripten build of the system libraries that runs `game.wasm`.
  - Graphics: WebGPU on a dedicated GPU thread. Shaders come from the recompiler's [WGSL backend](../../core/shader/recompiler/WgslBackend). Draws render to one color target and an optional depth target; compute dispatches run on the GPU and the buffers they write are copied back to guest memory before the queue goes on. A graphics submission that carries a flip waits until the queue has reached the flip of the one before, as the native driver does: a title that paces itself on pending flips would otherwise rewrite per-frame memory that the queue has not read yet. Textures are detiled on the CPU, BC-compressed ones included. The GPU needs the `subgroups` feature for pixel programs that permute quads, `texture-compression-bc`, and `float32-filterable` for 32-bit float textures. `?env=APS5_TRACE_GPU=1` logs each submission and dispatch; `?env=APS5_CAPTURE_SHADERS=<dir>` writes each new draw and dispatch to `<dir>` for `anyps5-wgsl`.
  - Input: keyboard and standard gamepads through `ScePad`.
  - Audio: `SceAudioOut` ports mixed in an AudioWorklet.
  - Saves: `/_sd` in IndexedDB, synced after each unmount and save commit.
  - Signals: WebAssembly cannot interrupt another thread, so `sceKernelRaiseException` on another thread (Unity's garbage collector stops threads this way) waits until the target polls it: the translated loops of a game that imports it, every runtime call before it returns, and every wait in a runtime call. `?env=APS5_TRACE_SIGNALS=1` logs each raise and delivery.
- [core/wasm-runtime/web](../../core/wasm-runtime/web): loader page and a local server that sends the COOP/COEP headers `SharedArrayBuffer` needs.

## Requirements

- Emscripten 6.0.12 (`emcmake` in `PATH`), Rust with Cargo, Python 3.
- Node.js 26 for the tests (memory64).
- A browser with WebGPU: Chrome on macOS or Windows. On Linux, start Chrome with `--enable-unsafe-webgpu --enable-features=Vulkan`.

## One command

`tools/wasm_play.py` runs the build and run steps below on Linux, macOS and Windows. It takes the title's archive (`.rar` or `.7z` need 7-Zip) or its extracted folder, with a decrypted `eboot.bin` in `decrypted/`, in place, or as `eboot.bin.esbak`, and finds module folders such as Unity's `Media/Modules` and `Media/Plugins` by itself:

```sh
python3 -m venv .venv
. .venv/bin/activate
python tools/wasm_play.py <title archive or folder>
```

On Windows, activate with `.venv\Scripts\activate`. Output goes to `build-wasm-aot/<title id>/`, with each step's output in its `logs/`. Steps whose outputs are up to date are skipped, so the next run starts the server at once; `--rebuild` redoes them. It opens Chrome when it is installed (on Linux with the WebGPU switches and its own profile in `build-wasm-aot/chrome-profile`). `--build-only` skips the server, `--port` changes it. `--allow-unsupported` translates instructions `anyps5-wasm-aot` does not support to traps that stop the game only when it runs them, and lists them; without it the translation stops at the first one. The relinker comes from `ANYPS5_RELINKER`, `build-relinker`, `build-<os>` or `build`, or is built relinker-only in `build-relinker`; the runtime is built in `ANYPS5_WASM_BUILD` (default `build-wasm-<os>`); Emscripten comes from `EMSDK`, `~/emsdk` or `emcc` on `PATH`.

## Build

1. Relink the title for Linux as described in [USAGE](../user/USAGE.md). `<out>` is the relinker's `linux` output directory, `<game>` an absolute path to an empty directory.
2. Create and activate a Python virtual environment:

   ```sh
   python3 -m venv .venv
   . .venv/bin/activate
   ```

3. Translate, build the runtime and package the game data:

   ```sh
   cargo run --release --manifest-path core/wasm-aot/Cargo.toml --bin anyps5-wasm-aot -- <out>/app.elf -o <game>/game.wasm
   emcmake cmake -S . -B build-wasm-runtime -DANYPS5_BUILD_WASM_RUNTIME=ON -DCMAKE_BUILD_TYPE=Release
   cmake --build build-wasm-runtime --target anyps5-runtime-web
   cd <out> && python <emsdk>/upstream/emscripten/tools/file_packager.py <game>/game.data --preload app0@/app0 --js-output=<game>/game.data.js --no-node
   ```

## Run

1. From the repository root, with the virtual environment active:

   ```sh
   python core/wasm-runtime/web/serve.py --runtime build-wasm-runtime/core/wasm-runtime --game <game> --port 8080
   ```

2. Open `http://127.0.0.1:8080/`. The server listens on `127.0.0.1` only; from another machine forward the port first: `ssh -N -L 8080:127.0.0.1:8080 <host>`.
3. Click the page once to start audio.

The panel in the top left shows the download of `game.wasm` and `game.data`, the current step and how long it has taken. Once the game runs it shrinks to a translucent "Running" box; click it for the last log lines. When the game stops, the panel turns red with the message that stopped it.

- `?env=NAME=VALUE` sets an environment variable for the runtime; repeat it for more.
- Saves stay in the browser profile; the same browser and origin continue them.

| Key | Pad |
|---|---|
| Enter, Space | Cross |
| C | Circle |
| I | Triangle |
| R | Square |
| Escape | Options |
| Arrows | D-pad |
| W A S D | Left stick |
| T F G H | Right stick |
| Q / E, Alt | L1 / R1 |
| Shift / Ctrl | L3 / R3 |
| Backspace / Tab | Touchpad left / right |

## Tests

```sh
emcmake cmake -S . -B build-wasm-runtime -DANYPS5_BUILD_WASM_RUNTIME=ON -DCMAKE_BUILD_TYPE=Release -DANYPS5_NODE=<node 26>
cmake --build build-wasm-runtime
ctest --test-dir build-wasm-runtime -R wasm_
cargo test --release --manifest-path core/wasm-aot/Cargo.toml
python -m unittest discover -s tools/tests -p test_wasm_nid_thunks.py
```

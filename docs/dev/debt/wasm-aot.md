# WebAssembly AOT

### Functional

- [Instruction coverage](../../../core/wasm-aot/src/codegen/function.rs) is incomplete. Unsupported instructions fail translation by default; `--allow-unsupported` records them in the report and emits a trap. The synthetic native and Unicorn oracle tests do not establish complete console CPU or floating-point compatibility.
- [Runtime integration](../../../core/wasm-aot/tests/runtime.rs) needs linked Emscripten runtimes and Node.js 26. Both cases are explicitly ignored in the standalone translator suite and run with `--ignored` in the runtime workflow. Three private-title tests also require external assets and are explicitly ignored in CI.

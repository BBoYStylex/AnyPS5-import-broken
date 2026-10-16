# AGENTS.md

AnyPS5 converts a PS5 ELF into a native Linux ELF or Windows PE linked against reimplemented system libraries; it is not an emulator. [CONTRIBUTING.md](CONTRIBUTING.md) wins over this file.

## Layout

- `core/relinker`: ELF rewriting and NID binding, C++20 standard library only.
- `core/libs/prx/<lib>`: one shared library per system library; `APS5_EXPORT("<nid>", func)` when the name is unknown.
- `core/shader/recompiler`: RDNA to IR to SPIR-V; Vulkan backend in `core/libs/prx/libSceAgcDriver`.
- Docs: [BUILD](docs/dev/BUILD.md), [CONVENTIONS](docs/dev/CONVENTIONS.md), [technical debt](docs/dev/TechnicalDebt.md).

## Build and test

```
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build && cmake --build build --target libs
ctest --test-dir build --output-on-failure -R <test>
python3 tools/check_conventions.py --base origin/main
```

Relinker-only changes can use the [relinker-only build](docs/dev/BUILD.md#relinker-only).

## Code

- Do exactly the right thing or throw `std::runtime_error`: no fallbacks, retries, silent success or behaviour switched by environment variables.
- Unimplemented exports call `NotImplemented_nid_no_patch(__func__)`. A silent stub only when it unblocks a title and affects only UI, listed in `docs/dev/debt/<lib>.md`.
- General behaviour, not what one title needs. Unverified return codes and shader semantics throw or go in technical debt; cite the hardware measurement or exact source (ISA, LLVM, ACO, Mesa).
- No code comments: technical debt comments are written only by humans.
- Debt entries go in `docs/dev/debt/<name>.md`, named after the first `core/` path they link, next to related entries.
- PascalCase classes and public methods, camelCase private members and arguments, `I` for interfaces. Standard C++ over extensions.
- Do not reimplement modules titles ship (`sce_module/`, `sce_modules/`, `prx/`). Third-party code is a submodule built from source.
- Tests fail without the change, pass with it, finish in 30 s on lavapipe with no shader cache and build on Windows (MinGW). No export-only tests.

## Pull requests

- Search open and closed PRs and issues first; never duplicate an open PR. Agree the order with PRs in the overlap comment and declare it with `Depends on`.
- One topic, based on current `main`; update by rebase, never by merging `main`.
- Conventional Commits for commits and PR title. Fill in the [template](.github/pull_request_template.md), including `Depends on`, `Unblocks`, `GPUs tested` and `AI-assisted`.
- A fix for a title names it on `Unblocks` and pastes its stop line on current `main`.
- Speed claims need the summary lines and traces of `main` and the change, same machine and scene ([Performance report](.github/ISSUE_TEMPLATE/performance.yml)).
- Notes, logs and images go in the PR, not the repository. Never add firmware, keys or game files.

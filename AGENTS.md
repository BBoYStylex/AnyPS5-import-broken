# AGENTS.md

AnyPS5 converts a PS5 ELF into a native Linux ELF or Windows PE linked against reimplemented system libraries; it is not an emulator.

Read [CONTRIBUTING.md](CONTRIBUTING.md) and [CONVENTIONS](docs/dev/CONVENTIONS.md) before changing anything: they are the rules, and a pull request that breaks them is closed. Build steps are in [BUILD](docs/dev/BUILD.md).

- `core/relinker`: ELF rewriting and NID binding. `core/libs/prx/<lib>`: one shared library per system library. `core/shader/recompiler`: RDNA to SPIR-V; Vulkan backend in `core/libs/prx/libSceAgcDriver`.
- Before starting, search open and closed pull requests by function name, and the title's [Game bring-up](https://github.com/boykopovar/AnyPS5/discussions/categories/game-bring-up) discussion: the same function is often already written. For a closed one, read why it was closed.
- Before opening, run `python3 tools/check_conventions.py --base origin/main` and `python3 tools/pr_overlap.py --branch <branch>`.
- Agents add no code comments: technical debt comments are written only by humans.

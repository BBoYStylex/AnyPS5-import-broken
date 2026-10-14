# Contributing

## Code

- Follow the [coding conventions](docs/dev/CONVENTIONS.md): naming, no comments except [technical debt](docs/dev/TechnicalDebt.md), Conventional Commits.
- Every function either does exactly what it is supposed to or throws. Unimplemented exports call `NotImplemented_nid_no_patch(__func__)` (see [libSceAudioIn](core/libs/prx/libSceAudioIn/Export.cpp)).
- Pull requests that add or change shader instruction semantics say where they come from: measured on hardware (which GPU and what was checked, e.g. with the [hardware oracle](docs/dev/HW_ORACLE.md)) or the exact source (ISA section, LLVM, ACO or Mesa file). Reviewers check the semantics on hardware. Cases the source doesn't settle throw, and behaviour not verified on hardware is recorded in [technical debt](docs/dev/TechnicalDebt.md).
- A silent stub is allowed only when it unblocks a title and only affects the UI; add it under Silent stubs in its library's [technical debt](docs/dev/debt/) file.
- Missing imports fail at startup with a clear error; don't replace them with fallbacks that keep running.
- Implement the general behaviour of a function, not what one title happens to need.
- Don't add replacements for modules that titles ship themselves in `sce_module/`, `sce_modules/` or `prx/`: engine or middleware modules (Cohtml, FMOD, GOG Galaxy) and SDK libraries that only wrap other system libraries (NpCppWebApi over NpWebApi2). The relinker converts and loads the title's own module, and a host library with the same name is left out of `DT_NEEDED`. Only system libraries, which reach the kernel or the hardware, are reimplemented in [core/libs/prx](core/libs/prx).
- Avoid non-standard extensions (`__attribute__`, etc.) where standard C++ is enough. Helper symbols that must not become NIDs use the `_nid_no_patch` or `_nid_no_patch_cut` suffix.
- The relinker uses only the C++20 standard library.
- Third-party code is added as a submodule under `3rdparty/` and built from source, not found on the system. With `ANYPS5_USE_SYSTEM_DEPS=ON` the packages packaged by the distributions (SDL2, freetype, Vulkan-Headers, SPIRV-Headers, glslang and SPIRV-Tools) come from the system instead; that resolution lives in [cmake/SystemDeps.cmake](cmake/SystemDeps.cmake) and the default build never reads the system.
- Don't add tests that only check that a symbol is exported: a missing export already fails at startup.
- Each test finishes in 30 s on the CI runners, Vulkan tests included (Linux runs them on lavapipe), counting the first run with no shader cache. ctest runs in parallel there, so a slow test competes for the cores and times out on some runs and not others. Split or shrink a test that gets close instead of raising its timeout. CI reports every test over 30 s as a warning on the pull request.

## Build and test

Toolchains are listed in the [build instructions](docs/dev/BUILD.md).

For relinker changes, use the [relinker-only build](docs/dev/BUILD.md#relinker-only) to build and test without third-party submodules, system libraries or a GPU. It also works on macOS. Changes to system libraries or shaders still need the full build and relevant runtime tests.

Shared system-library tests live under `core/libs/tests`. Use these subject directories:

- [files](core/libs/tests/files): file and directory I/O, paths and descriptors.
- [memory](core/libs/tests/memory): allocation, mappings, shared memory and write tracking.
- [threads](core/libs/tests/threads): thread lifecycle, synchronization, scheduling and thread-local storage.
- [networking](core/libs/tests/networking): sockets, HTTP, TLS, name resolution and network configuration.
- [graphics](core/libs/tests/graphics): shaders, graphics resources, display output and presentation.
- [media](core/libs/tests/media): audio, video, image codecs, fonts and speech.
- [input](core/libs/tests/input): controllers, keyboards, input methods and input devices.
- [savedata](core/libs/tests/savedata): save storage, metadata, search, dialogs and write failures.
- [runtime](core/libs/tests/runtime): strings, formatting, math, locale, containers and runtime utilities.
- [exceptions](core/libs/tests/exceptions): exception handling, unwinding, register contexts and backtraces.
- [modules](core/libs/tests/modules): dynamic loading, module identity, initialization and patched-library integration.
- [process](core/libs/tests/process): process lifecycle, arguments, environment, CPU information, signals and resource limits.
- [time](core/libs/tests/time): clocks, timers, sleeps and time conversion.
- [services](core/libs/tests/services): application metadata, platform services, dialogs and diagnostics.

Keep fixtures with the tests that use them. Library-local tests stay beside their implementations. The standalone `shared_memory`, `write_watch` and `register_context` CMake entry points retain their existing paths. Test executable names and CTest names are independent of these source directories.

Every project CTest case has a component label: `libs`, `relinker`, `tools`, `shader` or `decoder`. Library cases also have a subject label matching the directories above; NID patcher cases use `nid`. These labels describe the code being tested, not hardware requirements.

Register library cases with `add_library_test(memory NAME guest_memory COMMAND guest_memory_tests)` to receive `libs` and `memory`. Register other components with `add_component_test(relinker NAME relinker_conversion COMMAND relinker_conversion_tests)`. Both helpers live in `cmake/TestLabels.cmake`, included by the root build and standalone library test entry points. Add further labels with `set_property(TEST guest_memory APPEND PROPERTY LABELS extra_label)` to preserve the existing labels.

List or run a group using its exact label:

```sh
ctest --test-dir build -N -L "^memory$"
ctest --test-dir build --output-on-failure -L "^memory$"
```

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Python 3 is optional; without it some relinker tests are not registered.

Every push to `main`, including a merged pull request, automatically runs full Linux and Windows builds and tests and the relinker matrix on Linux, Windows and macOS, using the pushed commit without contributor checks.

For pull requests, full Linux and Windows builds and tests run only when a collaborator with write access selects Actions > Build > Run workflow on `main` and enters an open pull request number targeting `main`. A collaborator can start the same manual run from the command line, replacing `1234` with the pull request number:

```
gh workflow run build.yml --repo boykopovar/AnyPS5 --ref main --field pr=1234
```

This manual run also builds and tests the relinker on Linux, Windows and macOS, regardless of the author's previous contributions or the changed files. The relinker matrix also runs automatically when a pull request targeting `main` is opened, updated or reopened, if its author already has a commit in `main` and it changes a file the relinker-only build uses; full builds never run automatically for pull requests. Every job that runs automatically for pull requests runs only when the pull request changes its files: a `paths` filter on the `pull_request` trigger, or `needs: eligibility` with an `if` on the files output it sets in `build.yml`. The Conventions check fails a workflow change that adds a job without one. All build jobs build the selected pull request merged into the current `main` (GitHub's merge commit), so a pull request that no longer merges cleanly must be rebased first. New commits require another manual run for full builds.

With `--field full=false`, the manual run skips the full builds and runs only the relinker matrix on Linux, Windows and macOS, if the pull request changes a file the relinker-only build uses, such as the Python tools and their tests in `tools/`.

The Conventions check runs automatically for every contributor, including first-time contributors. It uses the base branch checker and reads pull request Git objects without executing pull request code. The Conventions check runs on every pull request and fails when a rule on this page is broken. It accepts code comments only when the pull request also changes [technical debt](docs/dev/TechnicalDebt.md), the index or a file in `docs/dev/debt/`, and only UTF-8 text files. Run it locally before pushing:

```
python3 tools/check_conventions.py --base origin/main
```

## Branches and pull requests

The [pull request template](.github/pull_request_template.md) is the checklist for these rules.

- One branch per topic, based on current `main`. Follow-up work goes in a new pull request, not into an open one.
- Open pull requests per author are limited by how many of theirs are merged into `main`: 1 with none merged, 20 with at least 1, no limit from 20. A pull request opened or reopened over the limit is closed automatically; reopen it once another one is merged or closed.
- Before starting, check that no open pull request already implements the same functions.
- Keep the branch up to date with `main` and resolve conflicts yourself; rebasing and force-pushing is fine.
- A pull request that conflicts with `main` and has no activity for 7 days is labeled `stale` with a reminder. It is not closed.
- If a pull request needs another one first, say so on the checklist line (`Depends on: #N`; `none` otherwise).
- If a pull request fixes a stop or a wrong result seen on a title, name it on the checklist line with the title ID and its [Game bring-up](https://github.com/boykopovar/AnyPS5/discussions/categories/game-bring-up) discussion (`Unblocks: PPSA01325 #3232`; `none` otherwise). Reviewers review these first.
- Fixes a title needs go to `main` as pull requests. A Game bring-up discussion lists the open pull requests and local changes the title still needs; update the list when one is merged, closed or replaced.
- The PR overlap comment lists open pull requests that change the same lines or exports; agree with their authors which one goes first and declare it with `Depends on`.
- Run the tests before opening the pull request and describe what was tested (OS, title or homebrew).
- A pull request that claims a change in speed shows the summary lines of `main` and of the change, from the same machine and scene, and attaches the traces, captured as the [Performance report](.github/ISSUE_TEMPLATE/performance.yml) issue form describes. A slow scene is reported with the same form.
- Changes to the AGC driver, the shader recompiler or VideoOut name each GPU the GPU tests ran on (vendor, model, driver, OS) on the `GPUs tested` checklist line. CI runs these tests only on lavapipe, and hardware drivers fail where lavapipe and other vendors pass, so a reviewer can ask for a run on another vendor before merging.
- Investigation notes, reports, screenshots and logs go in the pull request, not in the repository. Images for documentation go in the [gist](https://gist.github.com/boykopovar/0e53f2e1426f29ecd41e3b51540b8a90) comments.
- Say whether the change was written with AI assistance. The author of the pull request is responsible for every line of it.

## Documentation

Text in the repository is dry, strict and concise. Record known gaps, unknown NIDs and signatures in [technical debt](docs/dev/TechnicalDebt.md) instead of new files. An entry goes in `docs/dev/debt/<name>.md`, named after the first `core/` path it links (`<name>` for `core/libs/prx/<name>/...`, otherwise for `core/<name>/...`), under the same `###` sections as the index; entries that link neither stay in the index. A pull request whose `TechnicalDebt.md` change conflicts after the split rebases and runs `python3 tools/debt_split.py resolve` during the stopped rebase, which moves its added, edited and removed entries to their files; `python3 tools/debt_split.py split` moves entries left in the index.

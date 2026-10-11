### What

Implemented C++ signatures for 5 previously unparameterized PRX function stubs in libSceAudioOut, libSceNet, and libkernel:

| Library | Function | Signature |
|---|---|---|
| libSceAudioOut | `sceAudioOut2UserGetSupportedAttributes` | `int(user_id, void* attributes, uint32_t* num_attributes)` |
| libSceNet | `sceNetResolverAbort` | `int(rid)` |
| libkernel | `sceCoredumpWriteUserString` | `int(const char* string)` |
| libkernel | `sceCoredumpAttachUserMemoryFile` | `int(const void* addr, size_t size)` |
| libkernel | `sceCoredumpAttachMemoryRegion` | `int(const void* addr, size_t size)` |

Each stub validates its arguments, returns appropriate error codes for invalid pointers, and calls `NotImplemented_nid_no_patch(__func__)` for the unimplemented body. `sceNetResolverAbort` additionally removes the resolver from the registry and notifies waiters.

### Tested

Linux (Ubuntu 24.04, GCC 13.3), `BUILD_TESTING=ON`:
- `ninja -C build`: default target (all libraries, tools and tests) builds clean, 0 errors
- `ctest --test-dir build -R 'audio_out|guest_pad|guest_rtc|patched_libraries|nid'`: 14/14 pass
- `ctest --test-dir build`: 685/685 pass
- `python3 tools/check_conventions.py --base origin/main`: all conventions checks passed
- Verified all 5 new NIDs present in built `.prx` files via `nm -D`

No title run: stubs throw at runtime per convention.

### Checklist

- [x] Based on current `main`; no other open PR implements the same functions
- [x] One topic per PR; follow-ups go in a new PR
- [x] No comments in code except [technical debt](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/CONVENTIONS.md)
- [x] Unimplemented paths throw (`NotImplemented_nid_no_patch`); silent stubs are listed in [TechnicalDebt](https://github.com/boykopovar/AnyPS5/blob/main/docs/dev/TechnicalDebt.md#silent-stubs)
- [x] No notes, investigation `.md` files or images added to the repository (attach them to this PR)
- [x] New third-party code is a submodule built from source
- [x] Generic behaviour, not specific to one title
- [x] Depends on: none
- [x] AI-assisted: yes
# Project technical debt

<<<<<<< HEAD
- The [native ARM64 AOT compiler](../../core/aot/compiler.py) is experimental. Function/ABI contracts and indirect-call target sets are supplied with input hashes; their completeness is not inferred or proved. It supports a bounded scalar, SSE and VEX subset and rejects unsupported operations. Acyclic direct-call memory effects are inferred from validated code for private frame and fixed image writes. Provider memory effects are caller-verified annotations, not automatically inferred or bound to the linked binary. Their correctness, absence of guest reentry and external entry-point binding preconditions remain the integrator's responsibility. Unqualified imports, indirect calls, recursive call chains, unresolved parameter writes and unproved indexed aliases still invalidate pointer-cell proofs. Indexed frame writes have bounded proofs only for single-entry straight-line loops with constant initialization, one positive 64-bit increment and an exactly reachable equality exit before the increment. Calls, extra entries/exits, wrapping counters and dynamic bounds are excluded; these proofs do not reconstruct pointer cells. Fixed-size parameter-relative writes are inferred only for proven pointer origins with constant offsets and propagated through acyclic direct/tail calls. Dynamic extents, indexed parameter writes and unresolved aliases remain conservative. Exact stack pointer cells retain provenance through full-width spills/reloads and equal-origin control-flow joins, including declared incoming stack arguments. General range analysis, nonconstant effect sizes and arbitrary struct-pointer reconstruction remain unimplemented. Scalar stack-byte constants support native null arguments and known effect sizes; these constants do not establish native pointer provenance. Explicit checked stack accesses emit native bounds/alignment guards; object layouts still require recovery evidence, invalid accesses trap rather than deliver guest faults, and instrumentation costs are not game-performance qualified. Arbitrary pointer reconstruction, x87, guest fault delivery, guest TLS, C++ exceptions, aggregates, varargs, atomics, concurrent memory ordering, module closure and lifecycle remain incomplete. Its emitted objects need native import providers. The optional native image registry validates process metadata and allocator callback bindings for one main image; module unloading, dynamic metadata replacement and allocator extension callback ABIs are not supported. The optional immutable-data profile uses native symbolic relocations and startup page-protection checks; guest/provider writes, remapping and rebinding of those regions are outside its contract. The optional source-built hello_notify runtime provides one statically linked main image and its required native providers; it does not provide a complete game runtime or application package. The current macOS guest heap uses the existing host-allocation path rather than a reserved PS5 address arena. Main-image allocation registration reads actual Mach VM page protections; dynamic module registration is not implemented. Linux x86-64 validates and cross-compiles the example without execution. The optional Linux ARM64 validation target emits ELF and executes the source-built example natively; it uses the same lowering and native providers with ELF RELRO instead of Mach-O readonly sections. This validates the tested program path, not all guest instructions or Linux ABI combinations; no full Linux ARM64 game port is provided. The example annotation builder accepts only its pinned sources, known ABI declarations, matched object code and fixed DWARF stack location; it is not general automatic ABI recovery.
- Native AOT integer and SIMD semantics follow [Intel SDM Volume 2](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html), with [x86-64 SysV ABI](https://gitlab.com/x86-psABIs/x86-64-ABI) to [Apple ARM64 ABI](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms) reconstruction. The floating-point profile requires nearest-even rounding, preserved subnormals, masked exceptions and unobserved status flags/NaN payloads; FPCR is checked at entry. Division and aligned-access violations trap instead of reproducing guest exception delivery. Native tests and Rosetta SSE references do not qualify PS5 hardware behavior; AVX2 has independent integer-oracle tests but no x86 AVX2 execution oracle.
||||||| a11c8f7f7
=======
Entries about one library or component are in [debt](debt/), one file per library or component, under the same sections as this page. Entries that belong to none stay here.
>>>>>>> origin/main

### Build

- Building on Windows requires a specific version of mingw - MinGW-w64 GCC 15.2.0 (`winlibs-gcc15`, `x86_64-ucrt-posix-seh`)
- Even compiled prx libraries on Windows require nearby (static linking of these dependencies causes conflicts):
  - libgcc_s_seh-1.dll
  - libstdc++-6.dll
  - libwinpthread-1.dll

### Direct memory

- Windows direct-memory backing reserves a paging-file section and commits each mapped guest page before setting its protection. Committed physical contents remain in the section across unmap/remap and are released when the backing and its remaining views are destroyed; commitment is not recovered merely by unmapping a page. Linux retains its existing backing. Peak gameplay memory and Windows versions other than the tested host remain unverified.

### Silent stubs

Throughout the project, every function at every stage either **does exactly what it's supposed to or throws an exception**. Everywhere... except:

### Unknown function info

- AudioIn and NpSessionSignaling exports added without an implementation have assumed signatures
- Font, Http2, Net, Ssl, SystemService and libkernel exports imported by PPSA12544 and added without an implementation have unknown signatures
- The Share, NpTrophy2, NpEntitlementAccess and WebBrowserDialog exports imported by PPSA12544's Unity plugins and added without an implementation have unknown signatures
- Acm, GameLiveStreaming, Net, NpEntitlementAccess, NpManager, NpWebApi2, VoiceQoS, WebBrowserDialog and libkernel exports imported by PPSA14632 and added without an implementation are declared without parameters

### Functional

- [Cancellation coverage](ARCHITECTURE.md#thread-cancellation) checks deferred request handling and recovery, not full asynchronous support or console ABI compatibility. Cancellation during file/socket I/O, sleep interruption, cancellation versus exit/detach, condition signal conservation under cancellation, and safe rejection of non-null invalid or reclaimed thread handles remain unverified. No console title or homebrew establishes the cleanup NIDs or buffer layout. Accepted asynchronous mode currently has no diagnostic for its deferred behavior.
- `--to-intel` guest module trampolines are covered only by a synthetic relinker test; no game title has been verified with them on Linux or Windows.

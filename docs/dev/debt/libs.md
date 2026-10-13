# libs technical debt

### Unknown function info

- [SceAvPlayerVideoEx](../../../core/libs/SceTypes.hpp) (libSceAvPlayer) - frame rate field and encoding unknown; it is left zero in frame and stream info

### Functional

- [CLI glibc providers](../../../core/libs/native/NativeAotGlibcMath.cpp) bridge binary64 libm arguments, the explicit x86 extended argument to nexttoward, glibc tm/timeval layouts, environment and basic string/path services. Transcendentals use Darwin libm: arbitrary bit-identical glibc libm results, unmasked exceptions, guest floating-point status and non-C locale equivalence are not established. Historical SVID math modes are excluded. Non-null gettimeofday timezone arguments and unmapped pathconf names fail explicitly. The jq qualification uses the pinned source without Oniguruma; regex support is excluded. Thread-specific keys are process-unique 32-bit IDs with per-thread values and at most four destructor passes before join completes; key deletion does not invoke destructors. Cancellation, explicit pthread_exit and detach remain unsupported.

- The [native graphics closure](../../../core/libs/native/CMakeLists.txt) excludes `Math.cpp`, which now asserts x87 long-double storage for `__signbitl`. Apple ARM64 long double is binary64; weakening that assertion would accept a different guest ABI. The other math providers in that translation unit are also unavailable in this optional closure until the x87 providers are separated. Requiring an excluded provider fails at link time; the qualified cube does not use them.

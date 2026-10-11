#include <cstdint>
#include <cstdlib>

// Every library compiled with -fstack-protector references this, so it has to be
// visible outside libc.prx. ELF exports data symbols by default; Mach-O needs an
// explicit default visibility or the reference fails to resolve at load time.
__attribute__((visibility("default"))) std::uintptr_t __stack_chk_guard = 0xDEADBEEFCAFEBABEull;

#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_GENERAL_ASMMACROS_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_GENERAL_ASMMACROS_HPP

#ifdef _WIN32
#define APS5_ASM_FUNCTION(name) ".globl " name "\n.def " name "; .scl 2; .type 32; .endef\n" name ":\n"
#elif defined(__APPLE__)
// Mach-O symbols carry no type, and the Darwin assembler knows neither ELF's
// ".type @function" nor COFF's ".def". Names written in inline asm need the
// leading underscore the C compiler adds, which dyld strips again on lookup.
#define APS5_ASM_FUNCTION(name) ".globl _" name "\n_" name ":\n"
#else
#define APS5_ASM_FUNCTION(name) ".globl " name "\n.type " name ", @function\n" name ":\n"
#endif

// ELF-only symbol metadata and relocation suffixes. Mach-O has no ".type" or
// ".size" directives, and its calls need no @PLT.
#ifdef __APPLE__
#define APS5_ASM_TYPE(name)
#define APS5_ASM_SIZE(name, start)
#define APS5_ASM_CALL(name) "    call _" name "\n"
#define APS5_ASM_GLOBAL(name) ".globl " name "\n"
#else
#define APS5_ASM_TYPE(name) ".type " name ", @function\n"
#define APS5_ASM_SIZE(name, start) ".size " name ", .-" start "\n"
#define APS5_ASM_CALL(name) "    call " name "@PLT\n"
#define APS5_ASM_GLOBAL(name) ".globl " name "\n"
#endif

#endif

#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_GENERAL_EXPORTMACROS_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_GENERAL_EXPORTMACROS_HPP

#if defined(__APPLE__)
    // Mach-O has no assembler-level symbol alias, so emit a tail jump instead.
    // It preserves every calling convention and the target's control flow.
    // Symbols written in inline asm need the leading underscore that the C
    // compiler adds, which dyld strips again when it reports them to the loader.
    #define APS5_EXPORT(exportName, funcName) \
    __asm__(".globl \"_" exportName "_nid_no_patch_cut\"\n\t" \
    "\"_" exportName "_nid_no_patch_cut\":\n\t" \
    "jmp _" #funcName "\n")
#elif defined(__GNUC__) || defined(__clang__)
    #define APS5_EXPORT(exportName, funcName) \
    __asm__(".globl \"" exportName "_nid_no_patch_cut\"\n\t" \
    ".set \"" exportName "_nid_no_patch_cut\", " #funcName)
#elif defined(_MSC_VER)
    #define APS5_EXPORT(exportName, funcName) \
    __pragma(comment(linker, "/export:" exportName "_nid_no_patch_cut=" #funcName))
#else
    #define APS5_EXPORT(exportName, funcName)
#endif

#endif

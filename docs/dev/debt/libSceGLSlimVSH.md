# libSceGLSlimVSH technical debt

### Unknown function info

- [libSceGLSlimVSH pending exports](../../../core/libs/prx/libSceGLSlimVSH/Export.cpp) - 203 named callable exports are listed in the public [ps5-payload-dev SDK](https://github.com/ps5-payload-dev/sdk)'s `sce_stubs/libSceGLSlimVSH.c` table, which declares them as functions. Their signatures and behaviour are unverified: the table settles the names and the NIDs the guest imports, and nothing else, so the declarations use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`. They are for import resolution only and make no ABI claim: the console's EGL and OpenGL ES entry points this library covers are not implemented here, and a title that calls one gets the throw

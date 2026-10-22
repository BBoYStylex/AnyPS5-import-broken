# libSceGLSlimVSH technical debt

### Unknown function info

- The four `glOrbis*` extension exports in [Export.cpp](../../../core/libs/prx/libSceGLSlimVSH/Export.cpp) are present in the PS5 3.20 export inventory from [PS5-3.20_Libs](https://github.com/DNNDHH/PS5-3.20_Libs). Their exact prototypes and behavior are unverified; each uses a no-parameter throwing import-resolution placeholder.

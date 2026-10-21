# libmdbg_syscore technical debt

### Unknown function info

- [libmdbg_syscore pending exports](../../../core/libs/prx/libmdbg_syscore/Export.cpp) - 139 named exports are listed in the local SPRX export inventory and the public PS5 3.20 export list. Their signatures and behavior are unverified; the no-parameter declarations only resolve known imports and make no ABI claim.

# PR #8747: libSceLibcInternal exports

### Unknown function info

- `fegetexcept` is listed in the PS5 3.20 export inventory and cross-checked against PS5 firmware 13.60. Its return behavior and interaction with floating-point exception state are unverified; the declaration throws through `NotImplemented_nid_no_patch`.

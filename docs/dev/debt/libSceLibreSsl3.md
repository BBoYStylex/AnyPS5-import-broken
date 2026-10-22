# libSceLibreSsl3 technical debt

### Functional

- [ERR_load_crypto_strings and ERR_peek_last_error](../../../core/libs/prx/libSceLibreSsl3/Export.cpp) share libSceLibreSsl's string registry and calling-thread queue. This relationship and PS5 equivalence are unverified; the public contracts follow LibreSSL 3.4.1 at OpenBSD commit `078b3d1df8ca2011c1605cd40418c58fa0f5b143`. `ERR_print_errors_cb` remains throwing: the public callback contract is known, but PS5 thread identifiers and diagnostic metadata have not been established.

### Unknown function info

- [libSceLibreSsl3 pending exports](../../../core/libs/prx/libSceLibreSsl3/Export.cpp) - 87 named callable exports are listed in the [PS5 3.20 export table](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceLibreSsl3.c) and were cross-checked against the same module in PS5 firmware 13.60. Their signatures and behaviour are unverified; the declarations use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.

# libSceLibreSsl technical debt

### Unknown function info

- [libSceLibreSsl pending exports](../../../core/libs/prx/libSceLibreSsl/Export.cpp) - 396 named callable exports are listed in the [PS5 3.20 export table](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceLibreSsl.c) and were cross-checked against the same module in PS5 firmware 13.60. Their signatures and behaviour are unverified; the declarations use the no-parameter unknown-signature convention and throw through `NotImplemented_nid_no_patch`.
- [OCSP_cert_status_str, OCSP_crl_reason_str, OCSP_response_status_str](../../../core/libs/prx/libSceLibreSsl/Export.cpp) (libSceLibreSsl) - not compared with the console. The tables and the `(UNKNOWN)` fallback follow LibreSSL [ocsp_prn.c](https://github.com/libressl/openbsd/blob/bc9483725c5815efb64b6d9acc52fc5fba139212/src/lib/libcrypto/ocsp/ocsp_prn.c), whose tables and the status values in `ocsp.h` are unchanged since OpenSSL 0.9.7, so they do not depend on the console's LibreSSL release

#include "prx/libc/include/general/VabiMacros.hpp"
#include <climits>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
const char* APS5_VABI OCSP_cert_status_str_nid_postfix(long);
const char* APS5_VABI OCSP_crl_reason_str_nid_postfix(long);
const char* APS5_VABI OCSP_response_status_str_nid_postfix(long);
}

static void Require(bool value) { if (!value) std::abort(); }

using Lookup = const char* (APS5_VABI*)(long);

static void Check(Lookup lookup, std::initializer_list<const char*> expected) {
    long status = 0;
    for (const char* text : expected) {
        const char* result = lookup(status);
        Require(result != nullptr && std::strcmp(result, text) == 0 && lookup(status) == result);
        ++status;
    }
    for (long unknown : {-1L, -2L, status, status + 1, 100L, LONG_MIN, LONG_MAX}) {
        Require(std::strcmp(lookup(unknown), "(UNKNOWN)") == 0);
    }
}

int main() {
    Check(OCSP_cert_status_str_nid_postfix, {"good", "revoked", "unknown"});
    Check(OCSP_crl_reason_str_nid_postfix, {"unspecified", "keyCompromise", "cACompromise", "affiliationChanged",
                                            "superseded", "cessationOfOperation", "certificateHold", "(UNKNOWN)", "removeFromCRL"});
    Check(OCSP_response_status_str_nid_postfix, {"successful", "malformedrequest", "internalerror", "trylater",
                                                 "(UNKNOWN)", "sigrequired", "unauthorized"});
    return 0;
}

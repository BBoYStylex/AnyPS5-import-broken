#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceNetShowIfconfig();
int APS5_VABI sceNetResolverAbort(int);
int APS5_VABI sceNetResolverCreate(const char*, int, int);
int APS5_VABI sceNetResolverDestroy(int);
int APS5_VABI scePadUnknown_n3kSX62fgNo();
int APS5_VABI _ZN3sce4Json5Value21setNullAccessCallBackEPFRKS1_NS0_9ValueTypeEPS2_PvES6_();
int APS5_VABI kqueue_nid_postfix();
int APS5_VABI kevent_nid_postfix();
}

namespace {

using NetResolverAbortFunc = int (APS5_VABI *)(int);

struct Export {
    const char* name;
    int (APS5_VABI *function)();
    NetResolverAbortFunc net_resolver_abort = nullptr;
};

const Export exports[] = {
    {"sceNetShowIfconfig", sceNetShowIfconfig},
    {"sceNetResolverAbort", nullptr, sceNetResolverAbort},
    {"scePadUnknown_n3kSX62fgNo", scePadUnknown_n3kSX62fgNo},
    {"_ZN3sce4Json5Value21setNullAccessCallBackEPFRKS1_NS0_9ValueTypeEPS2_PvES6_", _ZN3sce4Json5Value21setNullAccessCallBackEPFRKS1_NS0_9ValueTypeEPS2_PvES6_},
    {"kqueue_nid_postfix", kqueue_nid_postfix},
    {"kevent_nid_postfix", kevent_nid_postfix},
};

bool ReportsNotImplemented(const Export& entry) {
    try {
        if (entry.net_resolver_abort) {
            int rid = sceNetResolverCreate("test", 0, 0);
            if (rid > 0) {
                entry.net_resolver_abort(rid);
                sceNetResolverDestroy(rid);
            } else {
                int result = entry.net_resolver_abort(1);
                if (result != (0x80410100 | 9)) {
                    std::fprintf(stderr, "sceNetResolverAbort returned %d instead of EBADF for invalid ID\n", result);
                    return false;
                }
            }
        } else {
            entry.function();
        }
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) == std::string(entry.name) + " not implemented") return true;
        std::fprintf(stderr, "%s threw: %s\n", entry.name, error.what());
        return false;
    }
    std::fprintf(stderr, "%s returned instead of reporting that it is not implemented\n", entry.name);
    return false;
}

}

int main() {
    bool correct = true;
    for (const auto& entry : exports) correct &= ReportsNotImplemented(entry);
    return correct ? 0 : 1;
}

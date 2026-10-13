#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceNetShowIfconfig();
int APS5_VABI scePadUnknown_n3kSX62fgNo();
int APS5_VABI _ZN3sce4Json5Value21setNullAccessCallBackEPFRKS1_NS0_9ValueTypeEPS2_PvES6_();
}

namespace {

bool CheckSceNetShowIfconfig() {
    return sceNetShowIfconfig() == 0;
}

bool ReportsNotImplemented(const char* name, int (APS5_VABI *function)()) {
    try {
        function();
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) == std::string(name) + " not implemented") return true;
        std::fprintf(stderr, "%s threw: %s\n", name, error.what());
        return false;
    }
    std::fprintf(stderr, "%s returned instead of reporting that it is not implemented\n", name);
    return false;
}

}

int main() {
    bool correct = true;
    correct &= CheckSceNetShowIfconfig();
    correct &= ReportsNotImplemented("scePadUnknown_n3kSX62fgNo", scePadUnknown_n3kSX62fgNo);
    correct &= ReportsNotImplemented("_ZN3sce4Json5Value21setNullAccessCallBackEPFRKS1_NS0_9ValueTypeEPS2_PvES6_", _ZN3sce4Json5Value21setNullAccessCallBackEPFRKS1_NS0_9ValueTypeEPS2_PvES6_);
    return correct ? 0 : 1;
}
#include "SceTypes.hpp"

#include <cstdlib>

extern "C" int APS5_VABI scePadUnknown_n3kSX62fgNo();

namespace {

void Require(bool value) {
    if (!value) std::abort();
}

}

int main() {
    Require(scePadUnknown_n3kSX62fgNo() == 0);
    Require(scePadUnknown_n3kSX62fgNo() == 0);
}

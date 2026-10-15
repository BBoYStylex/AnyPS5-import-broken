#include "SceTypes.hpp"

#include <cstdlib>

extern "C" int APS5_VABI sceNetShowNetstat(void);

namespace {

void Require(bool value) {
    if (!value) std::abort();
}

}

int main() {
    Require(sceNetShowNetstat() == 0);
    Require(sceNetShowNetstat() == 0);
}

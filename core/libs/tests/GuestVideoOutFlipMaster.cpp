#include "SceTypes.hpp"
#include <cstdlib>

extern "C" int APS5_VABI sceVideoOutSetFlipMaster();
extern "C" int APS5_VABI sceVideoOutClearFlipMaster();

namespace {

void Require(bool value) {
    if (!value) std::abort();
}

}

int main() {
    Require(sceVideoOutSetFlipMaster() == 0);
    Require(sceVideoOutClearFlipMaster() == 0);
    Require(sceVideoOutSetFlipMaster() == 0);
    Require(sceVideoOutClearFlipMaster() == 0);
}

#include "SceTypes.hpp"

#include <cstdlib>

extern "C" int APS5_VABI sceVrTracker2Finalize();

namespace {

void Require(bool value) {
    if (!value) std::abort();
}

}

int main() {
    Require(sceVrTracker2Finalize() == 0);
    Require(sceVrTracker2Finalize() == 0);
}

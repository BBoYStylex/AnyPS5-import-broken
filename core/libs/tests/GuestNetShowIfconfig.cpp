#include "SceTypes.hpp"

#include <cstdlib>

extern "C" int APS5_VABI sceNetShowIfconfig();

namespace {

void Require(bool value) {
    if (!value) std::abort();
}

}

int main() {
    Require(sceNetShowIfconfig() == 0);
    Require(sceNetShowIfconfig() == 0);
}

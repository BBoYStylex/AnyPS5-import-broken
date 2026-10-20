#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceNpPartyGetState(std::uint16_t* state);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

constexpr std::uint16_t kNotInParty = 2;

}

int main() {
    bool threwNullState = false;
    try {
        sceNpPartyGetState(nullptr);
    } catch (const std::invalid_argument&) {
        threwNullState = true;
    }
    Require(threwNullState);

    std::uint16_t state = 0xFFFF;
    Require(sceNpPartyGetState(&state) == 0);
    Require(state == kNotInParty);
}

#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr uint16_t NP_PARTY_STATE_NOT_IN_PARTY = 2;

}

extern "C" {

int APS5_VABI sceNpPartyGetState(uint16_t* state) {
    if (state == nullptr) APS5_INVALID_ARG_EX;
    *state = NP_PARTY_STATE_NOT_IN_PARTY;
    return 0;
}

}

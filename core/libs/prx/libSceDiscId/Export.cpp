#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

struct DiscId;
struct DiscId2;

namespace {

constexpr std::int32_t SCE_DISC_ID_ERROR_INVALID_ARGUMENT = static_cast<std::int32_t>(0x80D50001);
constexpr std::int32_t SCE_DISC_ID_ERROR_NO_DISC = static_cast<std::int32_t>(0x80D50002);

}

extern "C" {

int APS5_VABI sceDiscIdGet(DiscId* discId) {
    if (discId == nullptr) return SCE_DISC_ID_ERROR_INVALID_ARGUMENT;
    return SCE_DISC_ID_ERROR_NO_DISC;
}

int APS5_VABI sceDiscIdGet2(DiscId2* discId, std::uint32_t flags) {
    if (discId == nullptr || flags != 0) return SCE_DISC_ID_ERROR_INVALID_ARGUMENT;
    return SCE_DISC_ID_ERROR_NO_DISC;
}

}

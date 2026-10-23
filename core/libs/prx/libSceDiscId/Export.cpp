#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

struct DiscId;
struct DiscId2;

namespace {

constexpr std::int32_t SCE_DISC_ID_ERROR_NO_DISC = static_cast<std::int32_t>(0x80D50002);

}

extern "C" {

int APS5_VABI sceDiscIdGet(DiscId* discId) {
    (void)discId;
    return SCE_DISC_ID_ERROR_NO_DISC;
}

int APS5_VABI sceDiscIdGet2(DiscId2* discId, std::uint32_t flags) {
    (void)discId;
    (void)flags;
    return SCE_DISC_ID_ERROR_NO_DISC;
}

}

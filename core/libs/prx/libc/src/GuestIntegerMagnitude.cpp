#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
namespace {
std::int64_t Magnitude(std::int64_t value) {
    return value < 0 ? static_cast<std::int64_t>(0ULL - static_cast<std::uint64_t>(value)) : value;
}
}
extern "C" {
std::int64_t APS5_VABI labs_nid_postfix(std::int64_t value) { return Magnitude(value); }
std::int64_t APS5_VABI llabs_nid_postfix(std::int64_t value) { return Magnitude(value); }
std::int64_t APS5_VABI imaxabs_nid_postfix(std::int64_t value) { return Magnitude(value); }
}

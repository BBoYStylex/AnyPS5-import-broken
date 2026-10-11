#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
extern "C" void* APS5_VABI memccpy_nid_postfix(void* destination, const void* source, int character, std::size_t length) {
    auto* out = static_cast<unsigned char*>(destination);
    const auto* in = static_cast<const unsigned char*>(source);
    const auto stop = static_cast<unsigned char>(character);
    for (std::size_t index = 0; index < length; ++index) {
        out[index] = in[index];
        if (in[index] == stop) return out + index + 1;
    }
    return nullptr;
}

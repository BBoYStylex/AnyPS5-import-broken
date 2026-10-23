#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" void APS5_VABI sceMbusEventCallbackFuncsInit_(void* callbacks, std::uint32_t size);

extern "C" [[noreturn]] void NotImplemented_nid_no_patch(const char*) {
    throw std::runtime_error("unexpected unimplemented export call");
}

int main() {
    std::array<std::byte, 20> bytes;
    bytes.fill(std::byte{0xa5});

    sceMbusEventCallbackFuncsInit_(bytes.data() + 2, 16);

    if (bytes[0] != std::byte{0xa5} || bytes[1] != std::byte{0xa5}) return EXIT_FAILURE;
    for (std::size_t index = 2; index < 18; ++index) {
        if (bytes[index] != std::byte{0}) return EXIT_FAILURE;
    }
    if (bytes[18] != std::byte{0xa5} || bytes[19] != std::byte{0xa5}) return EXIT_FAILURE;

    sceMbusEventCallbackFuncsInit_(nullptr, 16);
    sceMbusEventCallbackFuncsInit_(bytes.data(), 0);
    if (bytes[0] != std::byte{0xa5}) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

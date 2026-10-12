#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
int APS5_VABI RemotePlayGetConnectionStatus(int user_id, int* status);
int APS5_VABI ShareGetCurrentStatus(std::uint32_t feature_flag, void* status);
}

static void Require(bool value) { if (!value) std::abort(); }

int main() {
    int connection = -1;
    Require(RemotePlayGetConnectionStatus(1, &connection) == 0 && connection == 0);
    bool threw = false;
    try {
        RemotePlayGetConnectionStatus(1, nullptr);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw);

    constexpr int invalidParam = static_cast<int>(0x81960002);
    std::uint8_t status[18];
    std::memset(status, 0x5a, sizeof(status));
    Require(ShareGetCurrentStatus(1, status) == 0);
    for (std::size_t index = 0; index < sizeof(status); ++index) Require(status[index] == (index < 16 ? 0 : 0x5a));
    std::memset(status, 0x5a, sizeof(status));
    Require(ShareGetCurrentStatus(0, status) == invalidParam && status[0] == 0x5a);
    Require(ShareGetCurrentStatus(1, nullptr) == invalidParam);
    return 0;
}

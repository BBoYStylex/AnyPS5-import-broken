#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

extern "C" {
int APS5_VABI sceNotificationSend(int type, std::uint8_t flag, std::uint8_t* message);
int APS5_VABI sceNotificationSendById(int type, std::uint8_t flag, std::uint8_t* id, std::uint8_t* jsonData);
int APS5_VABI sceNotificationShowPsButtonPersistentBanner(std::uint8_t* jsonParam);
void APS5_VABI sceNotificationHidePsButtonPersistentBanner();
}

namespace {

void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "guest notification: %s\n", message);
    std::abort();
}

template <typename F>
void RequireThrows(F call, const char* message) {
    try {
        call();
    } catch (const std::exception&) {
        return;
    }
    Require(false, message);
}

std::uint8_t* Bytes(std::string& text) {
    return reinterpret_cast<std::uint8_t*>(text.data());
}

}

int main() {
    auto message = std::string("hello from the guest");
    Require(sceNotificationSend(0, 0, Bytes(message)) == 0, "send failed");

    auto longest = std::string(1023, 'a');
    Require(sceNotificationSend(0, 0, Bytes(longest)) == 0, "send failed for the longest message");

    RequireThrows([] { sceNotificationSend(0, 0, nullptr); }, "send accepted a null message");

    std::vector<std::uint8_t> unterminated(1024, 'a');
    RequireThrows([&] { sceNotificationSend(0, 0, unterminated.data()); }, "send accepted an unterminated message");

    auto id = std::string("id-1");
    auto data = std::string("{\"key\":1}");
    Require(sceNotificationSendById(0, 0, Bytes(id), Bytes(data)) == 0, "sendById failed");
    RequireThrows([] { sceNotificationSendById(0, 0, nullptr, nullptr); }, "sendById accepted a null id");
    RequireThrows([&] { sceNotificationSendById(0, 0, Bytes(id), nullptr); }, "sendById accepted a null payload");

    auto banner = std::string("{\"banner\":1}");
    Require(sceNotificationShowPsButtonPersistentBanner(Bytes(banner)) == 0, "show failed");
    RequireThrows([] { sceNotificationShowPsButtonPersistentBanner(nullptr); }, "show accepted a null parameter");
    sceNotificationHidePsButtonPersistentBanner();
}

#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceAudioInExtClose(void);
int APS5_VABI sceAudioInExtInput(void);
int APS5_VABI sceAudioInExtOpen(void);
int APS5_VABI sceAudioInExtSetAecMode(void);
int APS5_VABI sceAudioInSetConnectionsForUser(void);
int APS5_VABI sceAudioInSetFocusForUser(void);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

template <typename Call>
void RequireThrows(const char* name, Call&& call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        Require(std::string(error.what()) == std::string(name) + " not implemented");
        return;
    }
    std::abort();
}

}

int main() {
    RequireThrows("sceAudioInExtClose", [] { sceAudioInExtClose(); });
    RequireThrows("sceAudioInExtInput", [] { sceAudioInExtInput(); });
    RequireThrows("sceAudioInExtOpen", [] { sceAudioInExtOpen(); });
    RequireThrows("sceAudioInExtSetAecMode", [] { sceAudioInExtSetAecMode(); });
    RequireThrows("sceAudioInSetConnectionsForUser", [] { sceAudioInSetConnectionsForUser(); });
    RequireThrows("sceAudioInSetFocusForUser", [] { sceAudioInSetFocusForUser(); });
}

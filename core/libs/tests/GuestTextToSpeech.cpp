#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceTextToSpeechInitImpl();
int APS5_VABI sceTextToSpeechOpenImpl();
int APS5_VABI sceTextToSpeechSpeakImpl();
int APS5_VABI sceTextToSpeechSpeakContinuousImpl();
int APS5_VABI sceTextToSpeechGetAudioDataImpl();
int APS5_VABI sceTextToSpeechGetStatusImpl();
int APS5_VABI sceTextToSpeechPauseImpl();
int APS5_VABI sceTextToSpeechResumeImpl();
int APS5_VABI sceTextToSpeechCancelImpl();
int APS5_VABI sceTextToSpeechCloseImpl();
int APS5_VABI sceTextToSpeechTermImpl();
}

static int failures = 0;

static void RequireNotImplemented(const char* name, const std::function<void()>& call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) == std::string(name) + " not implemented") return;
        std::fprintf(stderr, "%s threw: %s\n", name, error.what());
        ++failures;
        return;
    }
    std::fprintf(stderr, "%s returned instead of reporting that it is not implemented\n", name);
    ++failures;
}

int main() {
    RequireNotImplemented("sceTextToSpeechInitImpl", [] { sceTextToSpeechInitImpl(); });
    RequireNotImplemented("sceTextToSpeechOpenImpl", [] { sceTextToSpeechOpenImpl(); });
    RequireNotImplemented("sceTextToSpeechSpeakImpl", [] { sceTextToSpeechSpeakImpl(); });
    RequireNotImplemented("sceTextToSpeechSpeakContinuousImpl", [] { sceTextToSpeechSpeakContinuousImpl(); });
    RequireNotImplemented("sceTextToSpeechGetAudioDataImpl", [] { sceTextToSpeechGetAudioDataImpl(); });
    RequireNotImplemented("sceTextToSpeechGetStatusImpl", [] { sceTextToSpeechGetStatusImpl(); });
    RequireNotImplemented("sceTextToSpeechPauseImpl", [] { sceTextToSpeechPauseImpl(); });
    RequireNotImplemented("sceTextToSpeechResumeImpl", [] { sceTextToSpeechResumeImpl(); });
    RequireNotImplemented("sceTextToSpeechCancelImpl", [] { sceTextToSpeechCancelImpl(); });
    RequireNotImplemented("sceTextToSpeechCloseImpl", [] { sceTextToSpeechCloseImpl(); });
    RequireNotImplemented("sceTextToSpeechTermImpl", [] { sceTextToSpeechTermImpl(); });
    if (failures) std::abort();
    std::puts("TextToSpeech tests passed");
    return 0;
}

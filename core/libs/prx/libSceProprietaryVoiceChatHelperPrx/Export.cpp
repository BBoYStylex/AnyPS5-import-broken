#include "prx/libc/include/General.hpp"
#include <stdexcept>
#include <string>

extern "C" {

int APS5_VABI sceProprietaryVoiceChatHelperGetVoiceChatUsageState(void) {
    throw std::runtime_error(std::string(__func__) + ": signature unknown");
}

int APS5_VABI sceProprietaryVoiceChatHelperInitialize(void) {
    throw std::runtime_error(std::string(__func__) + ": signature unknown");
}

int APS5_VABI sceProprietaryVoiceChatHelperSetVoiceChatState(void) {
    throw std::runtime_error(std::string(__func__) + ": signature unknown");
}

int APS5_VABI sceProprietaryVoiceChatHelperTerminate(void) {
    throw std::runtime_error(std::string(__func__) + ": signature unknown");
}

}

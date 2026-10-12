#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include "prx/libSceSystemService/SystemService.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI ErrorDialogOpen(const void* param);
int APS5_VABI ErrorDialogClose(void);
int APS5_VABI ShareGetCurrentStatus(std::uint32_t featureFlag, ShareCurrentStatus* status);
int APS5_VABI ShareInitialize(std::size_t heapSize, int threadPriority, std::uint64_t affinityMask);
int APS5_VABI ShareTerminate(void);
int APS5_VABI SystemServiceParamGetInt(int paramId, int* value);
int APS5_VABI SystemServiceParamGetString(int paramId, char* buffer, std::size_t bufferSize);

void NotImplemented_nid_no_patch(const char* name) {
    throw std::runtime_error(std::string(name) + " not implemented");
}
}

static void Require(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::abort();
    }
}

int main() {
    constexpr int errorDialogParam = static_cast<int>(0x80ED0003u);
    constexpr int errorDialogInvalidState = static_cast<int>(0x80ED0005u);
    constexpr int shareInvalidParam = static_cast<int>(0x81960002u);

    struct DialogParam {
        std::int32_t size;
        std::int32_t errorCode;
        std::int32_t userId;
        std::int32_t reserved;
    };

    DialogParam param{16, 0, 0, 0};
    DialogParam invalidParam{8, 0, 0, 0};

    Require(ErrorDialogClose() == errorDialogInvalidState, "close before open");
    Require(ErrorDialogOpen(nullptr) == errorDialogParam, "null dialog parameter");
    Require(ErrorDialogOpen(&invalidParam) == errorDialogParam, "invalid dialog size");
    Require(ErrorDialogOpen(&param) == 0, "open dialog");
    Require(ErrorDialogOpen(&param) == errorDialogInvalidState, "duplicate dialog open");
    Require(ErrorDialogClose() == 0, "close dialog");

    ShareCurrentStatus status;
    std::memset(&status, 0xFF, sizeof(status));
    Require(ShareGetCurrentStatus(0, &status) == shareInvalidParam, "zero share flag");
    Require(ShareGetCurrentStatus(1, nullptr) == shareInvalidParam, "null share status");
    Require(ShareGetCurrentStatus(1, &status) == 0, "get share status");
    const auto* statusBytes = reinterpret_cast<const unsigned char*>(&status);
    for (std::size_t i = 0; i < sizeof(status); ++i) {
        Require(statusBytes[i] == 0, "share status should be cleared");
    }
    Require(ShareInitialize(0, 0, 0) == 0, "share initialize");
    Require(ShareTerminate() == 0, "share terminate");

    int value = -1;
    Require(SystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT, &value) == 0,
            "get time format");
    Require(value == SYSTEM_SERVICE_PARAM_TIME_FORMAT_24HOUR, "time format value");
    Require(SystemServiceParamGetInt(0, nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER,
            "null integer output");

    char systemName[SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH]{};
    Require(SystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME,
                                        systemName, sizeof(systemName)) == 0,
            "get system name");
    Require(std::strcmp(systemName, "PS5") == 0, "system name value");

    char shortBuffer[SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH - 1];
    std::memset(shortBuffer, 'x', sizeof(shortBuffer));
    bool threw = false;
    try {
        SystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME,
                                    shortBuffer, sizeof(shortBuffer));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "short string buffer should be reported");
    Require(shortBuffer[0] == 'x', "short buffer should remain unchanged");

    threw = false;
    try {
        SystemServiceParamGetString(SYSTEM_SERVICE_PARAM_ID_LANG,
                                    systemName, sizeof(systemName));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, "unsupported string parameter should be reported");

    std::cout << "PASS: WebAppLauncher compatibility tests\n";
    return 0;
}

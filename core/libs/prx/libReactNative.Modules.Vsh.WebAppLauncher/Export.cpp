#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t SHARE_ERROR_INVALID_PARAM = static_cast<std::int32_t>(0x81960002);
constexpr std::int32_t SHARE_ERROR_NOT_SUPPORTED = static_cast<std::int32_t>(0x81960007);
constexpr std::int32_t SHARE_REQUEST_ID_INVALID = -1;
constexpr int SYSTEM_SERVICE_ERROR_PARAMETER = static_cast<int>(0x80AA0002u);
constexpr int SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME = 9;
constexpr std::size_t SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH = 65;

}

extern "C" {

int APS5_VABI ErrorDialogClose(void) {
    return 0;
}

int APS5_VABI ErrorDialogOpen(const void* param) {
    if (param == nullptr) return SYSTEM_SERVICE_ERROR_PARAMETER;
    return 0;
}

int APS5_VABI ShareGetCurrentStatus(uint32_t feature_flag, ShareCurrentStatus* status) {
    if (feature_flag == 0 || status == nullptr) {
        return SHARE_ERROR_INVALID_PARAM;
    }
    std::memset(status, 0, sizeof(*status));
    return 0;
}

int APS5_VABI ShareInitialize(size_t heap_size, int thread_priority, uint64_t affinity_mask) {
    (void)heap_size;
    (void)thread_priority;
    (void)affinity_mask;
    return 0;
}

int APS5_VABI ShareTerminate(void) {
    return 0;
}

int APS5_VABI SystemServiceParamGetInt(int param_id, int* value) {
    if (value == nullptr) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    switch (param_id) {
        case 1: *value = 1; break;
        case 2: *value = 0; break;
        case 3: *value = 0; break;
        case 4: *value = 0; break;
        case 5: *value = 0; break;
        case 6: *value = 0; break;
        case 7: *value = 1; break;
        default: *value = 0; break;
    }
    return 0;
}

int APS5_VABI SystemServiceParamGetString(int param_id, char* buf, size_t buf_size) {
    if (buf == nullptr || buf_size == 0) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    if (param_id != SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    if (buf_size < SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    constexpr char SystemName[] = "PS5";
    std::memcpy(buf, SystemName, sizeof(SystemName));
    return 0;
}

}

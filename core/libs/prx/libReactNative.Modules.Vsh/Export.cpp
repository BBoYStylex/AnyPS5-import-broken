#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr std::int32_t ERROR_INVALID_PARAM = static_cast<std::int32_t>(0x81960002);
constexpr std::int32_t ERROR_NOT_SUPPORTED = static_cast<std::int32_t>(0x81960007);
constexpr std::int32_t REQUEST_ID_INVALID = -1;
constexpr int REMOTEPLAY_CONNECTION_STATUS_DISCONNECT = 0;

}

extern "C" {

int APS5_VABI RemotePlayGetConnectionStatus(int user_id, int* status) {
    (void)user_id;
    if (!status) throw std::invalid_argument("RemotePlayGetConnectionStatus: null status");
    *status = REMOTEPLAY_CONNECTION_STATUS_DISCONNECT;
    return 0;
}

int APS5_VABI ShareGetCurrentStatus(uint32_t feature_flag, ShareCurrentStatus* status) {
    if (feature_flag == 0 || status == nullptr) {
        return ERROR_INVALID_PARAM;
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

}

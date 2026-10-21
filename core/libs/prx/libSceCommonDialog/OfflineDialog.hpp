#ifndef CORE_LIBS_PRX_LIBSCECOMMONDIALOG_OFFLINEDIALOG_HPP
#define CORE_LIBS_PRX_LIBSCECOMMONDIALOG_OFFLINEDIALOG_HPP

#include <atomic>
#include <cstdint>
#include <cstring>

namespace {

class OfflineDialog {
public:
    static constexpr int STATUS_NONE = 0;
    static constexpr int STATUS_INITIALIZED = 1;
    static constexpr int STATUS_FINISHED = 3;
    static constexpr int ERROR_NOT_INITIALIZED = static_cast<int>(0x80B80003u);
    static constexpr int ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80B80004u);
    static constexpr int ERROR_NOT_FINISHED = static_cast<int>(0x80B80005u);
    static constexpr int ERROR_ARG_NULL = static_cast<int>(0x80B8000Du);
    static constexpr std::int32_t RESULT_USER_CANCELED = 1;

    int Initialize() {
        int expected = STATUS_NONE;
        if (!status.compare_exchange_strong(expected, STATUS_INITIALIZED)) return ERROR_ALREADY_INITIALIZED;
        return 0;
    }

    int Open(const void* param) {
        int current = status.load();
        do {
            if (current == STATUS_NONE) return ERROR_NOT_INITIALIZED;
            if (param == nullptr) return ERROR_ARG_NULL;
        } while (!status.compare_exchange_weak(current, STATUS_FINISHED));
        return 0;
    }

    int GetStatus() const {
        return status.load();
    }

    int GetResult(void* result) const {
        const int current = status.load();
        if (current == STATUS_NONE) return ERROR_NOT_INITIALIZED;
        if (result == nullptr) return ERROR_ARG_NULL;
        if (current != STATUS_FINISHED) return ERROR_NOT_FINISHED;
        std::memcpy(result, &RESULT_USER_CANCELED, sizeof(RESULT_USER_CANCELED));
        return 0;
    }

    int Close() {
        int current = status.load();
        do {
            if (current == STATUS_NONE) return ERROR_NOT_INITIALIZED;
        } while (!status.compare_exchange_weak(current, STATUS_FINISHED));
        return 0;
    }

    int Terminate() {
        if (status.exchange(STATUS_NONE) == STATUS_NONE) return ERROR_NOT_INITIALIZED;
        return 0;
    }

private:
    std::atomic<int> status{STATUS_NONE};
};

}

#endif

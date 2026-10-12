#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include "SceTypes.hpp"
#include "HitLog.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceSystemService/SystemService.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

constexpr std::int32_t kErrorDialogParam =
    static_cast<std::int32_t>(0x80ED0003u);
constexpr std::int32_t kErrorDialogInvalidState =
    static_cast<std::int32_t>(0x80ED0005u);
constexpr std::size_t kErrorDialogParamSize = 16;

// Headless compatibility state. This does not render a native PS5 dialog.
std::mutex g_dialogMutex;
bool g_dialogOpen = false;

bool Readable(const void* p, std::size_t n) {
#ifdef _WIN32
    if (p == nullptr) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof(mbi)) == 0 || mbi.State != MEM_COMMIT) return false;
    if ((mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) return false;
    const auto end = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return reinterpret_cast<std::uintptr_t>(p) + n <= end;
#else
    (void)n;
    return p != nullptr;
#endif
}

int ConsoleLanguage() {
    const char* configured = std::getenv("ANYPS5_LANGUAGE");

    if (configured == nullptr || configured[0] == '\0') {
        return SYSTEM_SERVICE_PARAM_LANG_ENGLISH_US;
    }

    const std::string_view text(configured);
    int language = -1;
    const auto [end, error] =
        std::from_chars(text.data(), text.data() + text.size(), language);

    if (error != std::errc{} ||
        end != text.data() + text.size() ||
        language < SYSTEM_SERVICE_PARAM_LANG_JAPANESE ||
        language > SYSTEM_SERVICE_PARAM_LANG_UKRAINIAN) {
        throw std::runtime_error(
            "SystemServiceParamGetInt: invalid ANYPS5_LANGUAGE value");
    }

    return language;
}

} // namespace

extern "C" {

int APS5_VABI ErrorDialogOpen(const void* param) {
    if (!Readable(param, kErrorDialogParamSize)) {
        return kErrorDialogParam;
    }

    std::int32_t size = 0;
    std::memcpy(&size, param, sizeof(size));

    if (static_cast<std::size_t>(size) != kErrorDialogParamSize) {
        return kErrorDialogParam;
    }

    std::int32_t errorCode = 0;
    std::int32_t userId = 0;

    const auto* bytes = static_cast<const std::uint8_t*>(param);
    std::memcpy(&errorCode, bytes + 4, sizeof(errorCode));
    std::memcpy(&userId, bytes + 8, sizeof(userId));

    std::lock_guard lock(g_dialogMutex);

    if (g_dialogOpen) {
        return kErrorDialogInvalidState;
    }

    g_dialogOpen = true;

    APS5_HIT("ERRORDIALOG",
             "WebAppLauncher open: error_code=0x%08X user_id=%d",
             static_cast<unsigned int>(errorCode), userId);

    return 0;
}

int APS5_VABI ErrorDialogClose(void) {
    std::lock_guard lock(g_dialogMutex);

    if (!g_dialogOpen) {
        return kErrorDialogInvalidState;
    }

    g_dialogOpen = false;
    APS5_HIT("ERRORDIALOG", "WebAppLauncher close");

    return 0;
}

int APS5_VABI ShareGetCurrentStatus(
    std::uint32_t feature_flag,
    ShareCurrentStatus* status) {

    if (feature_flag == 0 || status == nullptr) {
        return static_cast<std::int32_t>(0x81960002u);
    }

    std::memset(status, 0, sizeof(*status));
    return 0;
}

int APS5_VABI ShareInitialize(
    std::size_t heap_size,
    int thread_priority,
    std::uint64_t affinity_mask) {

    (void)heap_size;
    (void)thread_priority;
    (void)affinity_mask;

    return 0;
}

int APS5_VABI ShareTerminate(void) {
    return 0;
}

int APS5_VABI SystemServiceParamGetInt(int paramId, int* value) {
    if (value == nullptr) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }

    switch (paramId) {
        case SYSTEM_SERVICE_PARAM_ID_LANG:
            *value = ConsoleLanguage();
            break;

        case SYSTEM_SERVICE_PARAM_ID_DATE_FORMAT:
            *value = SYSTEM_SERVICE_PARAM_DATE_FORMAT_DDMMYYYY;
            break;

        case SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT:
            *value = SYSTEM_SERVICE_PARAM_TIME_FORMAT_24HOUR;
            break;

        case SYSTEM_SERVICE_PARAM_ID_TIME_ZONE:
        case SYSTEM_SERVICE_PARAM_ID_SUMMERTIME:
            *value = 0;
            break;

        case SYSTEM_SERVICE_PARAM_ID_GAME_PARENTAL_LEVEL:
            *value = SYSTEM_SERVICE_PARAM_GAME_PARENTAL_OFF;
            break;

        case SYSTEM_SERVICE_PARAM_ID_ENTER_BUTTON_ASSIGN:
            *value = SYSTEM_SERVICE_PARAM_ENTER_BUTTON_CROSS;
            break;

        default:
            *value = 0;
            break;
    }

    return SYSTEM_SERVICE_OK;
}

int APS5_VABI SystemServiceParamGetString(
    int paramId, char* buf, std::size_t bufSize) {

    if (buf == nullptr || bufSize == 0) {
        return SYSTEM_SERVICE_ERROR_PARAMETER;
    }

    if (paramId != SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME) {
        NotImplemented_nid_no_patch(
            "SystemServiceParamGetString: unsupported parameter");
    }

    if (bufSize < SYSTEM_SERVICE_MAX_SYSTEM_NAME_LENGTH) {
        NotImplemented_nid_no_patch(
            "SystemServiceParamGetString: buffer shorter than 65 bytes");
    }

    constexpr char systemName[] = "PS5";
    std::memcpy(buf, systemName, sizeof(systemName));

    return SYSTEM_SERVICE_OK;
}

} // extern "C"

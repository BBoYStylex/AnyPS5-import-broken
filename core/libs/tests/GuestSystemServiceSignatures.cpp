#include "prx/libSceSystemService/SystemService.hpp"
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(std::is_same_v<decltype(&sceLncUtilSetAppFocus), int (APS5_VABI*)(std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceLncUtilSuspendApp), int (APS5_VABI*)(std::int32_t, std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceShellCoreUtilShowErrorDialog), int (APS5_VABI*)(std::int32_t)>);
static_assert(std::is_same_v<decltype(&sceShellCoreUtilNotificationRequestedForIDU), int (APS5_VABI*)(std::int32_t, std::int32_t, const char*)>);
static_assert(std::is_same_v<decltype(&sceSystemServiceAddLocalProcess), int (APS5_VABI*)(std::int32_t, const char*, std::int32_t, const char* const*)>);
static_assert(std::is_same_v<decltype(&sceSystemServiceChangeCpuClock), int (APS5_VABI*)(std::int32_t)>);

namespace {

template <typename Call>
void RequireThrows(const char* name, Call&& call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).rfind(name, 0) == 0) return;
        std::fprintf(stderr, "%s threw %s\n", name, error.what());
        std::exit(1);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s threw a non-runtime error: %s\n", name, error.what());
        std::exit(1);
    }
    std::fprintf(stderr, "%s returned instead of throwing\n", name);
    std::exit(1);
}

}

int main() {
    RequireThrows("sceLncUtilSetAppFocus", [] { sceLncUtilSetAppFocus(1, 0); });
    RequireThrows("sceLncUtilSuspendApp", [] { sceLncUtilSuspendApp(1, 0); });
    RequireThrows("sceShellCoreUtilShowErrorDialog", [] { sceShellCoreUtilShowErrorDialog(1); });
    RequireThrows("sceShellCoreUtilNotificationRequestedForIDU", [] { sceShellCoreUtilNotificationRequestedForIDU(1, 2, "text"); });
    RequireThrows("sceSystemServiceAddLocalProcess", [] { sceSystemServiceAddLocalProcess(1, "path", 0, nullptr); });
    RequireThrows("sceSystemServiceChangeCpuClock", [] { sceSystemServiceChangeCpuClock(1); });
    std::puts("libSceSystemService pending exports throw");
    return 0;
}

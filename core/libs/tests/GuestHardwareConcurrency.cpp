#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdio>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern "C" unsigned int APS5_VABI _ZNSt8__sce_v226_Thrd_hardware_concurrencyEv_nid_postfix() noexcept;

int main() {
    unsigned int expected = 0;
#ifdef _WIN32
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    expected = info.dwNumberOfProcessors;
#else
    const long processors = sysconf(_SC_NPROCESSORS_ONLN);
    if (processors > 0) expected = static_cast<unsigned int>(processors);
#endif
    const auto query = _ZNSt8__sce_v226_Thrd_hardware_concurrencyEv_nid_postfix;
    const unsigned int mainResult = query();
    if (expected != 0 && mainResult != expected) {
        std::fprintf(stderr, "guest processor hint %u differs from OS count %u\n", mainResult, expected);
        return 1;
    }
    std::array<unsigned int, 2> results{};
    std::array<std::thread, 2> workers;
    for (std::size_t index = 0; index < workers.size(); ++index)
        workers[index] = std::thread([&, index] { results[index] = query(); });
    for (auto& worker : workers) worker.join();
    for (const auto result : results) {
        if (result != mainResult) {
            std::fprintf(stderr, "guest processor hint differs across threads\n");
            return 1;
        }
    }
}

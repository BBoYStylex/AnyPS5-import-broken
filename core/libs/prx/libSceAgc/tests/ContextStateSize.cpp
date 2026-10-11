#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/Shutdown.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

extern "C" std::uint64_t APS5_VABI sceAgcDcbContextStateOpGetSize(std::uint32_t operation);
extern "C" std::uint32_t* APS5_VABI sceAgcDcbContextStateOp_0100(CommandBuffer* buf, std::uint32_t operation);

namespace {

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testSizes() {
    const std::array<std::uint64_t, 4> sizes{20, 108, 108, 128};
    for (std::uint32_t operation = 0; operation < sizes.size(); ++operation) {
        check(sceAgcDcbContextStateOpGetSize(operation) == sizes[operation], "context state size mismatch");
        std::array<std::uint32_t, 64> words{};
        CommandBuffer buffer{words.data(), words.data() + words.size(), words.data(), words.data() + words.size(),
                             nullptr, nullptr, 0};
        check(sceAgcDcbContextStateOp_0100(&buffer, operation) == words.data(), "context state op did not start at the cursor");
        const auto written = static_cast<std::uint64_t>(buffer.cursor_up - words.data()) * sizeof(std::uint32_t);
        check(written == sizes[operation], "context state size differs from the written packets");
    }
}

void testUnsupportedOperations() {
    check(sceAgcDcbContextStateOpGetSize(4) == 0, "unsupported context state operation size was not zero");
    check(sceAgcDcbContextStateOpGetSize(0xffffffffu) == 0, "unsupported context state operation size was not zero");
}

}

int main() {
    try {
        testSizes();
        testUnsupportedOperations();
        LibcRunShutdown_nid_postfix();
        std::puts("AGC context state size tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        try { LibcRunShutdown_nid_postfix(); }
        catch (const std::exception& shutdown) { std::fprintf(stderr, "shutdown: %s\n", shutdown.what()); }
        return 1;
    }
}

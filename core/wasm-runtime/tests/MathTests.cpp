#include "wasm-runtime/Guest.hpp"
#include <array>
#include <bit>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <limits>

extern "C" {
float y0f_nid_postfix(float);
float y1f_nid_postfix(float);
float ynf_nid_postfix(int, float);
long double fminl_nid_postfix(long double, long double);
long double fmaxl_nid_postfix(long double, long double);
float _FSinh_nid_postfix(float, float);
float _FCosh_nid_postfix(float, float);
int* __error_nid_postfix();
}

namespace WasmRuntime {

GuestCpu& CurrentCpu() {
    static GuestCpu cpu{};
    return cpu;
}

}  // namespace WasmRuntime

static void CheckBesselZero() {
    auto& cpu = WasmRuntime::CurrentCpu();
    const char* names[] = {"y0f", "y1f", "ynf"};
    for (int function = 0; function < 3; ++function) {
        for (const int order : {INT_MIN, -3, -2, -1, 0, 1, 2, 3, INT_MAX}) {
            for (const float input : {0.0f, -0.0f}) {
                for (const std::uint32_t controls : {0x1f80u, 0x3f80u, 0xdfc0u}) {
                    for (const std::uint32_t flags : {0u, 1u, 4u, 0x20u, 0x3fu}) {
                        const auto expectedMxcsr = controls | flags;
                        cpu.mxcsr = expectedMxcsr;
                        *__error_nid_postfix() = 12345;
                        const float result = function == 0 ? y0f_nid_postfix(input) : function == 1 ? y1f_nid_postfix(input) : ynf_nid_postfix(order, input);
                        if (std::bit_cast<std::uint32_t>(result) != 0xff800000u || cpu.mxcsr != expectedMxcsr || *__error_nid_postfix() != 12345) {
                            std::fprintf(stderr, "Guest %s(%d, %08x): result %08x, mxcsr %x, expected %x, errno %d\n", names[function], order,
                                         std::bit_cast<std::uint32_t>(input), std::bit_cast<std::uint32_t>(result), cpu.mxcsr, expectedMxcsr, *__error_nid_postfix());
                            std::abort();
                        }
                    }
                }
            }
        }
    }
}

static void CheckLongDoubleNan() {
    const auto signaling = std::bit_cast<long double>(std::array<std::uint64_t, 2>{1, 0x7fff000000000000ull});
    for (const auto nan : {signaling, -signaling, std::numeric_limits<long double>::quiet_NaN()}) {
        for (const auto function : {fminl_nid_postfix, fmaxl_nid_postfix}) {
            if (function(nan, 1.0L) != 1.0L || function(1.0L, nan) != 1.0L) std::abort();
        }
    }
}

static void CheckHyperbolicExceptions() {
    auto& cpu = WasmRuntime::CurrentCpu();
    for (const auto function : {_FSinh_nid_postfix, _FCosh_nid_postfix}) {
        cpu.mxcsr = 0x1fa0;
        *__error_nid_postfix() = 12345;
        const auto invalid = function(std::numeric_limits<float>::infinity(), 0.0f);
        if ((std::bit_cast<std::uint32_t>(invalid) & 0x7fffffffu) <= 0x7f800000u || cpu.mxcsr != 0x1fa1 || *__error_nid_postfix() != EDOM) std::abort();
        cpu.mxcsr = 0x1f81;
        *__error_nid_postfix() = 12345;
        const auto overflow = function(100.0f, 1.0f);
        if (std::bit_cast<std::uint32_t>(overflow) != 0x7f800000u || cpu.mxcsr != 0x1fa9 || *__error_nid_postfix() != ERANGE) std::abort();
    }
}

int main() {
    CheckBesselZero();
    CheckLongDoubleNan();
    CheckHyperbolicExceptions();
}

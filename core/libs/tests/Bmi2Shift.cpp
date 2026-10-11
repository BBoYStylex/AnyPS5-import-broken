#include "prx/libc/src/specifics/windows/CrashReport.cpp"
#include <array>
#include <cpuid.h>
#include <stdexcept>
#include <vector>

namespace GuestArena {
extern "C" bool GuestArenaHandleWrite_nid_postfix(std::uintptr_t) { return false; }
}

namespace {

using Bytes = std::vector<std::uint8_t>;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

Bytes Encoding(unsigned prefix, bool wide, unsigned destination, unsigned source, unsigned count) {
    return {0xc4,
        static_cast<std::uint8_t>(2 | (destination < 8 ? 0x80 : 0) | 0x40 | (source < 8 ? 0x20 : 0)),
        static_cast<std::uint8_t>((wide ? 0x80 : 0) | ((count ^ 15) << 3) | prefix),
        0xf7, static_cast<std::uint8_t>(0xc0 | ((destination & 7) << 3) | (source & 7))};
}

CONTEXT Initial() {
    CONTEXT context{};
    for (unsigned reg = 0; reg < 16; ++reg) Bmi2Shift::Register(context, reg) = 0xfedcba9876543210ull + reg;
    context.Rip = 0x100000;
    context.EFlags = 0x8d7;
    context.FltSave.XmmRegisters[0].Low = 0x1122334455667788;
    return context;
}

std::uint64_t Reference(unsigned prefix, bool wide, std::uint64_t value, std::uint64_t count) {
    const unsigned width = wide ? 64 : 32;
    const std::uint64_t mask = wide ? ~std::uint64_t{0} : 0xffffffff;
    value &= mask;
    for (unsigned i = 0; i < (count & (width - 1)); ++i) {
        if (prefix == 1) value = (value * 2) & mask;
        else {
            const auto sign = prefix == 2 ? value & (std::uint64_t{1} << (width - 1)) : 0;
            value = (value / 2) | sign;
        }
    }
    return value;
}

void Check(const Bytes& bytes, CONTEXT context, unsigned destination, std::uint64_t result) {
    auto expected = context;
    Bmi2Shift::Register(expected, destination) = result;
    expected.Rip += bytes.size();
    Require(Bmi2Shift::Emulate(bytes.data(), bytes.size(), context), "Shift rejected");
    Require(std::memcmp(&context, &expected, sizeof(context)) == 0, "Wrong result or unrelated context changed");
    for (std::size_t size = 0; size < bytes.size(); ++size) {
        auto truncated = expected;
        const auto before = truncated;
        Require(!Bmi2Shift::Emulate(bytes.data(), size, truncated), "Truncated instruction accepted");
        Require(std::memcmp(&truncated, &before, sizeof(before)) == 0, "Rejected instruction changed context");
    }
}

void Registers() {
    const Bytes crash{0xc4, 0x62, 0xe9, 0xf7, 0xc0};
    auto context = Initial();
    context.Rax = 1;
    context.Rdx = 0x25;
    Check(crash, context, 8, 0x2000000000ull);
    EXCEPTION_RECORD record{};
    record.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
    context.Rip = reinterpret_cast<std::uint64_t>(crash.data());
    EXCEPTION_POINTERS info{&record, &context};
    Require(ReportCrash(&info) == EXCEPTION_CONTINUE_EXECUTION, "Crash handler did not resume BMI2 shift");
    Require(context.R8 == 0x2000000000ull && context.Rip == reinterpret_cast<std::uint64_t>(crash.data()) + 5, "Crash recovery failed");
    record.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
    Require(!HandleBmi2Shift(&info), "Non-illegal exception intercepted");

    const std::array<std::uint64_t, 8> values{0, 1, ~std::uint64_t{0}, 0x8000000000000000ull,
        0x80000000, 0x7fffffff, 0x123456789abcdef0ull, 0x7fffffffffffffffull};
    const std::array<std::uint64_t, 10> counts{0, 1, 31, 32, 33, 63, 64, 65, 0x12345678, ~std::uint64_t{0}};
    for (unsigned prefix = 1; prefix <= 3; ++prefix) {
        for (bool wide : {false, true}) {
            for (unsigned destination = 0; destination < 16; ++destination) {
                for (unsigned source = 0; source < 16; ++source) {
                    for (unsigned count = 0; count < 16; ++count) {
                        const auto bytes = Encoding(prefix, wide, destination, source, count);
                        context = Initial();
                        const auto value = values[(source + destination) % values.size()];
                        Bmi2Shift::Register(context, source) = value;
                        Bmi2Shift::Register(context, count) = counts[(count + destination) % counts.size()];
                        Check(bytes, context, destination, Reference(prefix, wide,
                            Bmi2Shift::Register(context, source), Bmi2Shift::Register(context, count)));
                    }
                }
            }
            for (const auto value : values) {
                for (const auto count : counts) {
                    context = Initial();
                    context.Rcx = value;
                    context.Rdx = count;
                    Check(Encoding(prefix, wide, 0, 1, 2), context, 0, Reference(prefix, wide, value, count));
                }
            }
        }
    }
}

void AppendDisplacement(Bytes& bytes, std::int32_t value) {
    const auto* data = reinterpret_cast<const std::uint8_t*>(&value);
    bytes.insert(bytes.end(), data, data + 4);
}

void Memory() {
    std::uint64_t value = 0x8765432187654321ull;
    const auto address = reinterpret_cast<std::uint64_t>(&value);
    auto context = Initial();
    context.Rdx = 4;
    context.Rcx = address;
    Check({0xc4, 0xe2, 0xe9, 0xf7, 0x01}, context, 0, value << 4);
    context.Rcx = address + 8;
    Check({0xc4, 0xe2, 0xeb, 0xf7, 0x41, 0xf8}, context, 0, value >> 4);
    context.R13 = address + 512;
    Bytes displaced{0xc4, 0xc2, 0xe9, 0xf7, 0x85};
    AppendDisplacement(displaced, -512);
    Check(displaced, context, 0, value << 4);
    context.R12 = 1;
    context.R13 = address - 8;
    Check({0xc4, 0x82, 0xe9, 0xf7, 0x44, 0xe5, 0x00}, context, 0, value << 4);
    context.Rsp = address;
    Check({0xc4, 0xe2, 0xe9, 0xf7, 0x04, 0x24}, context, 0, value << 4);
    Bytes relative{0xc4, 0xc2, 0x69, 0xf7, 0x05};
    AppendDisplacement(relative, -128);
    context.Rip = address + 128 - relative.size();
    Check(relative, context, 0, (value << 4) & 0xffffffff);
    Bytes noBase{0xc4, 0xc2, 0xe9, 0xf7, 0x04, 0x0d};
    AppendDisplacement(noBase, -16);
    context.Rcx = address + 16;
    Check(noBase, context, 0, value << 4);

    auto* pages = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Require(pages != nullptr, "Cannot allocate memory fixture");
    std::memcpy(pages + 4093, &value, sizeof(value));
    context.Rcx = reinterpret_cast<std::uint64_t>(pages + 4093);
    Check({0xc4, 0xe2, 0xe9, 0xf7, 0x01}, context, 0, value << 4);
    DWORD old;
    Require(VirtualProtect(pages + 4096, 4096, PAGE_NOACCESS, &old), "Cannot protect memory fixture");
    const auto before = context;
    const Bytes crossing{0xc4, 0xe2, 0xe9, 0xf7, 0x01};
    Require(!Bmi2Shift::Emulate(crossing.data(), crossing.size(), context), "Unreadable memory accepted");
    Require(std::memcmp(&context, &before, sizeof(context)) == 0, "Failed memory read changed context");
    VirtualFree(pages, 0, MEM_RELEASE);
}

void Rejections() {
    const std::array<Bytes, 8> invalid{{
        {0xc4, 0xe2, 0xe8, 0xf7, 0xc1}, {0xc4, 0xe2, 0xed, 0xf7, 0xc1},
        {0xc4, 0xe1, 0xe9, 0xf7, 0xc1}, {0xc4, 0xe2, 0xe9, 0xf6, 0xc1},
        {0x66, 0xc4, 0xe2, 0xe9, 0xf7, 0xc1}, {0x67, 0xc4, 0xe2, 0xe9, 0xf7, 0x01},
        {0xf0, 0xc4, 0xe2, 0xe9, 0xf7, 0xc1}, {0xc5, 0xe9, 0xf7, 0xc1}
    }};
    for (const auto& bytes : invalid) {
        auto context = Initial();
        const auto before = context;
        Require(!Bmi2Shift::Emulate(bytes.data(), bytes.size(), context), "Invalid or unsupported encoding accepted");
        Require(std::memcmp(&context, &before, sizeof(context)) == 0, "Rejected encoding changed context");
    }
}

std::uint64_t injectedRip = 0;
unsigned injectedFaults = 0;

LONG WINAPI InjectShift(EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION && info->ContextRecord->Rip == injectedRip) {
        info->ContextRecord->Rip += 2;
        ++injectedFaults;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void Recovery() {
    const Bytes bytes{0xb8, 1, 0, 0, 0, 0xba, 0x25, 0, 0, 0,
        0x0f, 0x0b, 0xc4, 0x62, 0xe9, 0xf7, 0xc0, 0x4c, 0x89, 0xc0, 0xc3};
    auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Require(code != nullptr, "Cannot allocate recovery fixture");
    std::memcpy(code, bytes.data(), bytes.size());
    DWORD old;
    Require(VirtualProtect(code, 4096, PAGE_EXECUTE_READ, &old), "Cannot execute recovery fixture");
    FlushInstructionCache(GetCurrentProcess(), code, bytes.size());
    injectedRip = reinterpret_cast<std::uint64_t>(code + 10);
    auto* handler = AddVectoredExceptionHandler(1, InjectShift);
    Require(handler != nullptr, "Cannot install fault injection handler");
    const auto run = reinterpret_cast<std::uint64_t (*)()>(code);
    const auto result = run();
    RemoveVectoredExceptionHandler(handler);
    VirtualFree(code, 0, MEM_RELEASE);
    Require(injectedFaults == 1 && result == 0x2000000000ull, "Windows exception recovery failed");
}

void Native() {
    unsigned eax, ebx, ecx, edx;
    if (!__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) || (ebx & (1u << 8)) == 0) {
        std::puts("Native BMI2 comparison unavailable on this CPU; context tests passed");
        return;
    }
    auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Require(code != nullptr, "Cannot allocate native code fixture");
    for (unsigned prefix = 1; prefix <= 3; ++prefix) {
        for (bool wide : {false, true}) {
            auto bytes = Encoding(prefix, wide, 0, 1, 2);
            bytes.push_back(0xc3);
            DWORD old;
            Require(VirtualProtect(code, 4096, PAGE_READWRITE, &old), "Cannot write native fixture");
            std::memcpy(code, bytes.data(), bytes.size());
            Require(VirtualProtect(code, 4096, PAGE_EXECUTE_READ, &old), "Cannot execute native fixture");
            FlushInstructionCache(GetCurrentProcess(), code, bytes.size());
            const auto native = reinterpret_cast<std::uint64_t (*)(std::uint64_t, std::uint64_t)>(code);
            std::uint64_t value = 0x9e3779b97f4a7c15ull;
            for (unsigned i = 0; i < 1024; ++i) {
                value ^= value << 13;
                value ^= value >> 7;
                value ^= value << 17;
                auto context = Initial();
                context.Rcx = value;
                context.Rdx = i;
                bytes.pop_back();
                Check(bytes, context, 0, native(value, i));
                bytes.push_back(0xc3);
            }
        }
    }
    VirtualFree(code, 0, MEM_RELEASE);
    std::puts("Native BMI2 comparison passed");
}

}

int main() {
    try {
        Registers();
        Memory();
        Rejections();
        Recovery();
        Native();
        std::puts("BMI2 shift tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}

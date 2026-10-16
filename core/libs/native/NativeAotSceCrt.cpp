#include "native/NativeAotSceCrt.hpp"
#include "native/NativeAotSce.hpp"
#include "native/NativeAotSceAbi.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/FileStream.hpp"
#include "prx/libc/include/WindowsFormatting.hpp"
#include "prx/libc/include/WindowsScanning.hpp"
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

extern "C" {
int pthread_create_nid_postfix(Pthread*, const PthreadAttr*, PthreadEntry, void*);
void qsort_nid_postfix(void*, std::size_t, std::size_t, int (*)(const void*, const void*));
}

namespace {
using State = NativeAotGuestState;
using NativeAotGuest::Argument;
using NativeAotGuest::Return;

template<class T> T* Pointer(const State& state, std::size_t index) { return reinterpret_cast<T*>(Argument(state, index)); }

struct Arguments {
    alignas(16) std::array<std::byte, 176> registers{};
    VaList list{};
    Arguments(const State& state, unsigned fixed) {
        if (fixed > 6) throw std::invalid_argument("fixed variadic arguments exceed integer registers");
        for (unsigned index = 0; index < 6; ++index) {
            const auto value = Argument(state, index);
            std::memcpy(registers.data() + index * 8, &value, 8);
        }
        std::memcpy(registers.data() + 48, state.Xmm, 128);
        list.gp_offset = fixed * 8;
        list.fp_offset = 48;
        list.reg_save_area = registers.data();
        list.overflow_arg_area = reinterpret_cast<void*>(state.Gpr[4] + 8);
    }
};

void Sprintf(State& state) {
    Arguments args(state, 2);
    Return(state, LibcDetail::FormatWindows(Pointer<char>(state, 0), SIZE_MAX, Pointer<char>(state, 1), &args.list));
}

void Snprintf(State& state) {
    Arguments args(state, 3);
    Return(state, LibcDetail::FormatWindows(Pointer<char>(state, 0), Argument(state, 1), Pointer<char>(state, 2), &args.list));
}

void Vsnprintf(State& state) {
    Return(state, LibcDetail::FormatWindows(Pointer<char>(state, 0), Argument(state, 1), Pointer<char>(state, 2), Pointer<VaList>(state, 3)));
}

void Fprintf(State& state) {
    Arguments args(state, 2);
    std::string text;
    const auto count = LibcDetail::FormatWindows(nullptr, 0, Pointer<char>(state, 1), &args.list, &text);
    auto* file = Pointer<FileStream>(state, 0);
    const auto written = std::fwrite(text.data(), 1, text.size(), file->GetHandle());
    file->SyncStatus();
    Return(state, written == text.size() ? count : -1);
}

void CheckScan(const char* format) {
    if (format == nullptr) return;
    while ((format = std::strchr(format, '%')) != nullptr) {
        ++format;
        if (*format == '%') { ++format; continue; }
        while (*format == '*' || (*format >= '0' && *format <= '9')) ++format;
        if (*format == 'L' || (*format == 'l' && std::strchr("sc[", format[1]) != nullptr))
            throw std::invalid_argument("extended or wide scanning is not qualified on ARM64");
        if (*format == '[') {
            ++format;
            if (*format == '^') ++format;
            if (*format == ']') ++format;
            while (*format && *format != ']') ++format;
        }
    }
}

void Sscanf(State& state) {
    const auto* format = Pointer<char>(state, 1);
    CheckScan(format);
    Arguments args(state, 2);
    Return(state, LibcDetail::ScanWindows(Pointer<char>(state, 0), format, &args.list));
}

void Fscanf(State& state) {
    const auto* format = Pointer<char>(state, 1);
    CheckScan(format);
    Arguments args(state, 2);
    auto* file = Pointer<FileStream>(state, 0);
    Return(state, LibcDetail::ScanFileWindows_nid_no_patch(file->GetHandle(), format, &args.list));
    file->SyncStatus();
}

thread_local std::uint64_t comparator = 0;
int Compare(const void* left, const void* right) {
    return static_cast<int>(NativeAotSce::InvokeCallback(comparator, reinterpret_cast<std::uint64_t>(left), reinterpret_cast<std::uint64_t>(right)));
}

void Qsort(State& state) {
    const auto target = Argument(state, 3);
    if (!NativeAotGuest::IsConvertedFunction(target)) throw std::invalid_argument("qsort callback has no AOT entry");
    struct Scope {
        std::uint64_t previous = comparator;
        explicit Scope(std::uint64_t value) { comparator = value; }
        ~Scope() { comparator = previous; }
    } scope(target);
    qsort_nid_postfix(Pointer<void>(state, 0), Argument(state, 1), Argument(state, 2), Compare);
}

struct ThreadEntry { std::uint64_t target, argument; };
void* EnterThread(void* pointer) {
    std::unique_ptr<ThreadEntry> entry(static_cast<ThreadEntry*>(pointer));
    return reinterpret_cast<void*>(NativeAotSce::InvokeCallback(entry->target, entry->argument));
}

void CreateThread(State& state) {
    if (!NativeAotGuest::ConcurrentMemory()) throw std::invalid_argument("SCE worker requires ordered memory");
    const auto target = Argument(state, 2);
    if (!NativeAotGuest::IsConvertedFunction(target)) throw std::invalid_argument("thread callback has no AOT entry");
    auto entry = std::make_unique<ThreadEntry>(ThreadEntry{target, Argument(state, 3)});
    NativeAotGuest::PromoteSharedMemory();
    const auto status = pthread_create_nid_postfix(Pointer<Pthread>(state, 0), Pointer<const PthreadAttr>(state, 1), EnterThread, entry.get());
    if (status == 0) static_cast<void>(entry.release());
    Return(state, status);
}

void Lldiv(State& state) {
    const auto a = static_cast<std::int64_t>(Argument(state, 0));
    const auto b = static_cast<std::int64_t>(Argument(state, 1));
    if (b == 0 || (a == std::numeric_limits<std::int64_t>::min() && b == -1))
        throw std::invalid_argument("lldiv quotient is not representable");
    state.Gpr[0] = static_cast<std::uint64_t>(a / b);
    state.Gpr[2] = static_cast<std::uint64_t>(a % b);
}

double Exp2(double x) { const auto saved = errno; const auto value = std::exp2(x); errno = saved; return value; }
double Exp(double x) { return std::exp(x); }
double Sin(double x) { return std::sin(x); }
double Log10(double x) { return std::log10(x); }
float Exp2f(float x) { return std::exp2(x); }

}

namespace NativeAotSce {
NativeAotGuest::Provider CrtProvider(const char* name) {
    const struct { const char* name; NativeAotGuest::Provider provider; } entries[] = {
        {"sprintf", Sprintf}, {"snprintf", Snprintf}, {"vsnprintf", Vsnprintf}, {"fprintf", Fprintf},
        {"sscanf", Sscanf}, {"fscanf", Fscanf}, {"qsort", Qsort}, {"pthread_create", CreateThread},
        {"lldiv", Lldiv}, {"exp2", ScalarBridge<Exp2>}, {"exp", ScalarBridge<Exp>},
        {"sin", ScalarBridge<Sin>}, {"log10", ScalarBridge<Log10>}, {"exp2f", ScalarBridge<Exp2f>}
    };
    for (const auto& entry : entries) if (std::strcmp(name, entry.name) == 0) return entry.provider;
    return nullptr;
}
}

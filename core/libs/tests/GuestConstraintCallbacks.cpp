#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
void* APS5_VABI set_constraint_handler_s_nid_postfix(void*);
void APS5_VABI ignore_handler_s_nid_postfix(const char*, void*, int);
int APS5_VABI wcscpy_s_nid_postfix(std::uint16_t*, std::size_t, const std::uint16_t*);
void* APS5_VABI sceAmprSetConstraintHandlerS(void*);
void APS5_VABI sceAmprIgnoreHandlerS(const char*, void*, int);
int APS5_VABI sceAmprWcscpyS(std::uint16_t*, std::size_t, const std::uint16_t*);
}

namespace {

using SetHandler = void* (APS5_VABI *)(void*);
using Handler = void (APS5_VABI *)(const char*, void*, int);
using Copy = int (APS5_VABI *)(std::uint16_t*, std::size_t, const std::uint16_t*);

struct Api {
    SetHandler set;
    Handler ignore;
    Copy copy;
};

constexpr Api Libc{set_constraint_handler_s_nid_postfix, ignore_handler_s_nid_postfix, wcscpy_s_nid_postfix};
constexpr Api Ampr{sceAmprSetConstraintHandlerS, sceAmprIgnoreHandlerS, sceAmprWcscpyS};
const char* receivedMessage = nullptr;
void* receivedPointer = nullptr;
int receivedError = 0;
int firstCalls = 0;
int secondCalls = 0;
SetHandler reentrantSet = nullptr;
void* previousInHandler = nullptr;

void Require(bool value, const char* message) {
    if (value) return;
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
}

void APS5_VABI Second(const char* message, void* pointer, int error) {
    receivedMessage = message;
    receivedPointer = pointer;
    receivedError = error;
    ++secondCalls;
}

void APS5_VABI First(const char* message, void* pointer, int error) {
    receivedMessage = message;
    receivedPointer = pointer;
    receivedError = error;
    ++firstCalls;
    if (reentrantSet) previousInHandler = reentrantSet(reinterpret_cast<void*>(&Second));
}

void CheckArguments(void* pointer, int error) {
    Require(receivedError == error, "guest constraint callback received the wrong error");
    Require(receivedPointer == pointer, "guest constraint callback received the wrong pointer");
    Require(receivedMessage && std::strcmp(receivedMessage, "wcscpy_s") == 0,
        "guest constraint callback received the wrong message");
}

void Check(const Api& api) {
    auto* first = reinterpret_cast<void*>(&First);
    auto* second = reinterpret_cast<void*>(&Second);
    auto* initial = api.set(first);
    Require(initial != nullptr, "initial constraint handler is null");
    Require(api.set(second) == first && api.set(first) == second, "previous constraint handler changed");
    constexpr std::uint16_t text[]{65, 66, 0};
    std::array<std::uint16_t, 5> output{0x1234, 0x5678, 0x9abc, 0xdef0, 0x1357};
    Require(api.copy(nullptr, 3, text) == 22 && firstCalls == 1, "null destination callback count or result");
    CheckArguments(nullptr, 22);
    Require(api.copy(output.data(), 3, nullptr) == 22 && firstCalls == 2, "null source callback count or result");
    CheckArguments(output.data(), 22);
    Require(api.copy(output.data(), 0, text) == 22 && firstCalls == 3, "zero size callback count or result");
    CheckArguments(output.data(), 22);
    Require(output[0] == 0x1234 && output[1] == 0x5678, "invalid input changed the output");
    Require(api.copy(output.data() + 1, 2, text) == 34 && firstCalls == 4, "short buffer callback count or result");
    CheckArguments(output.data() + 1, 34);
    Require(output[0] == 0x1234 && output[1] == 65 && output[2] == 0 && output[3] == 0xdef0,
        "short buffer output or guard changed");
    Require(api.copy(output.data() + 1, 3, text) == 0 && firstCalls == 4 && secondCalls == 0,
        "successful copy invoked a handler");
    Require(output[1] == 65 && output[2] == 66 && output[3] == 0 && output[4] == 0x1357,
        "successful copy output or guard changed");
    reentrantSet = api.set;
    Require(api.copy(nullptr, 3, text) == 22 && firstCalls == 5, "reentrant handler was not invoked");
    CheckArguments(nullptr, 22);
    Require(previousInHandler == first, "reentrant registration returned the wrong handler");
    reentrantSet = nullptr;
    Require(api.copy(output.data(), 0, text) == 22 && secondCalls == 1, "replacement handler was not invoked");
    CheckArguments(output.data(), 22);
    auto* ignore = reinterpret_cast<void*>(api.ignore);
    Require(api.set(ignore) == second, "ignore registration returned the wrong handler");
    Require(api.copy(nullptr, 3, text) == 22 && firstCalls == 5 && secondCalls == 1,
        "ignore handler changed the result or invoked an old handler");
    Require(api.set(nullptr) == ignore && api.set(first) == initial, "null registration did not restore the default");
    api.set(initial);
}

void CheckIsolation() {
    auto* first = reinterpret_cast<void*>(&First);
    auto* second = reinterpret_cast<void*>(&Second);
    auto* libcInitial = Libc.set(first);
    auto* amprInitial = Ampr.set(second);
    constexpr std::uint16_t text[]{65, 0};
    Require(Libc.copy(nullptr, 2, text) == 22 && firstCalls == 1 && secondCalls == 0,
        "libc handler state changed across libraries");
    CheckArguments(nullptr, 22);
    Require(Ampr.copy(nullptr, 2, text) == 22 && firstCalls == 1 && secondCalls == 1,
        "Ampr handler state changed across libraries");
    CheckArguments(nullptr, 22);
    Libc.set(libcInitial);
    Ampr.set(amprInitial);
}

}

int main(int argc, char** argv) {
    Require(argc == 2, "usage: guest_constraint_callbacks_tests <libc|ampr|isolation>");
    if (std::strcmp(argv[1], "libc") == 0) Check(Libc);
    else if (std::strcmp(argv[1], "ampr") == 0) Check(Ampr);
    else if (std::strcmp(argv[1], "isolation") == 0) CheckIsolation();
    else return 2;
    std::printf("constraint callbacks: %s passed\n", argv[1]);
    return 0;
}

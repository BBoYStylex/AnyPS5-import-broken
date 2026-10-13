#include "SceTypes.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <utility>

extern "C" {
using GuestRudpEventHandler = void(APS5_VABI*)(int, int, int, void*);
int APS5_VABI sceRudpInit_nid_postfix(void*, int);
int APS5_VABI sceRudpCreateContext(GuestRudpEventHandler, void*, int*);
int APS5_VABI sceRudpTerminate(int);
int APS5_VABI sceRudpEnd();
int APS5_VABI sceRudpSetOption(int, int, const void*, uint32_t);
int APS5_VABI sceRudpGetOption(int, int, void*, uint32_t);
int APS5_VABI sceRudpGetNumberOfPacketsToRead(int);
int APS5_VABI sceRudpGetSizeWritable(int);
int APS5_VABI sceRudpGetSizeReadable(int);
}

static void Require(bool value) {
    if (!value) std::abort();
}

static void APS5_VABI Handler(int, int, int, void*) {}

int main() {
    constexpr int notInitialized = static_cast<int>(0x80770001u);
    constexpr int invalidContext = static_cast<int>(0x80770003u);
    constexpr int invalidArgument = static_cast<int>(0x80770004u);
    constexpr int invalidOption = static_cast<int>(0x80770005u);
    constexpr int notAcceptable = static_cast<int>(0x80770011u);
    constexpr std::int32_t sentinel = 0x5a5a5a5a;
    constexpr std::array<std::pair<int, std::int32_t>, 17> defaultOptions = {{
        {1, 1346},
        {2, 65536},
        {3, 65536},
        {4, 0},
        {5, 1},
        {6, 1},
        {7, 1},
        {8, 0},
        {9, 60000},
        {10, 0},
        {11, 30},
        {14, 0},
        {15, 0},
        {16, 0},
        {17, 0},
        {18, 0},
        {19, 0},
    }};
    alignas(16) static std::array<unsigned char, 0x10000> pool{};

    std::int32_t optionValue = sentinel;
    Require(sceRudpGetNumberOfPacketsToRead(0) == notInitialized);
    Require(sceRudpGetSizeWritable(0) == notInitialized);
    Require(sceRudpGetSizeReadable(0) == notInitialized);
    Require(sceRudpSetOption(0, 1, &optionValue, sizeof(optionValue)) == notInitialized);
    Require(sceRudpSetOption(-1, 0, nullptr, 0) == notInitialized);
    Require(sceRudpGetOption(0, 1, &optionValue, sizeof(optionValue)) == notInitialized);
    Require(sceRudpGetOption(-1, 0, nullptr, 0) == notInitialized);
    Require(optionValue == sentinel);

    Require(sceRudpInit_nid_postfix(pool.data(), static_cast<int>(pool.size())) == 0);
    Require(sceRudpGetNumberOfPacketsToRead(0) == invalidContext);
    Require(sceRudpGetSizeWritable(0) == invalidContext);
    Require(sceRudpGetSizeReadable(0) == invalidContext);
    Require(sceRudpGetNumberOfPacketsToRead(12345) == invalidContext);
    Require(sceRudpGetSizeWritable(12345) == invalidContext);
    Require(sceRudpGetSizeReadable(12345) == invalidContext);
    Require(sceRudpGetNumberOfPacketsToRead(-1) == invalidArgument);
    Require(sceRudpGetSizeWritable(-1) == invalidArgument);
    Require(sceRudpGetSizeReadable(-1) == invalidArgument);
    Require(sceRudpSetOption(0, 1, &optionValue, sizeof(optionValue)) == invalidContext);
    Require(sceRudpSetOption(0, 0, nullptr, 0) == invalidContext);
    Require(sceRudpSetOption(-1, 1, &optionValue, sizeof(optionValue)) == invalidContext);
    Require(sceRudpGetOption(0, 1, &optionValue, sizeof(optionValue)) == invalidContext);
    Require(sceRudpGetOption(0, 0, nullptr, 0) == invalidContext);
    Require(sceRudpGetOption(-1, 1, &optionValue, sizeof(optionValue)) == invalidContext);
    Require(optionValue == sentinel);

    int context = -1;
    Require(sceRudpCreateContext(Handler, nullptr, &context) == 0);
    Require(sceRudpGetNumberOfPacketsToRead(context) == 0);
    Require(sceRudpGetSizeWritable(context) == notAcceptable);
    Require(sceRudpGetSizeReadable(context) == notAcceptable);

    for (int badOption : {-1, 0, 12, 13, 20}) {
        Require(sceRudpSetOption(context, badOption, &optionValue, sizeof(optionValue)) == invalidOption);
        Require(sceRudpSetOption(context, badOption, nullptr, 0) == invalidOption);
        Require(sceRudpGetOption(context, badOption, &optionValue, sizeof(optionValue)) == invalidOption);
        Require(sceRudpGetOption(context, badOption, nullptr, 0) == invalidOption);
        Require(optionValue == sentinel);
    }

    Require(sceRudpSetOption(context, 1, nullptr, sizeof(optionValue)) == invalidArgument);
    Require(sceRudpGetOption(context, 1, nullptr, sizeof(optionValue)) == invalidArgument);
    for (uint32_t badLen : {0u, 2u, 8u}) {
        Require(sceRudpSetOption(context, 1, &optionValue, badLen) == invalidArgument);
        Require(sceRudpGetOption(context, 1, &optionValue, badLen) == invalidArgument);
        Require(optionValue == sentinel);
    }

    for (const auto& [option, expected] : defaultOptions) {
        std::array<std::int32_t, 3> neighbors = {sentinel, sentinel, sentinel};
        Require(sceRudpGetOption(context, option, &neighbors[1], sizeof(std::int32_t)) == 0);
        Require(neighbors[0] == sentinel);
        Require(neighbors[1] == expected);
        Require(neighbors[2] == sentinel);
    }

    int secondContext = -1;
    Require(sceRudpCreateContext(Handler, nullptr, &secondContext) == 0);
    for (const auto& [option, expected] : defaultOptions) {
        const std::int32_t updated = expected + 100 + option;
        Require(sceRudpSetOption(context, option, &updated, sizeof(updated)) == 0);
        std::int32_t readBack = sentinel;
        Require(sceRudpGetOption(context, option, &readBack, sizeof(readBack)) == 0);
        Require(readBack == updated);
        std::int32_t secondReadBack = sentinel;
        Require(sceRudpGetOption(secondContext, option, &secondReadBack, sizeof(secondReadBack)) == 0);
        Require(secondReadBack == expected);
    }
    Require(sceRudpTerminate(secondContext) == 0);
    Require(sceRudpGetOption(secondContext, 1, &optionValue, sizeof(optionValue)) == invalidContext);
    Require(sceRudpSetOption(secondContext, 1, &optionValue, sizeof(optionValue)) == invalidContext);

    Require(sceRudpTerminate(context) == 0);
    Require(sceRudpGetNumberOfPacketsToRead(context) == invalidContext);
    Require(sceRudpGetSizeWritable(context) == invalidContext);
    Require(sceRudpGetSizeReadable(context) == invalidContext);
    Require(sceRudpGetOption(context, 1, &optionValue, sizeof(optionValue)) == invalidContext);
    Require(sceRudpSetOption(context, 1, &optionValue, sizeof(optionValue)) == invalidContext);
    Require(sceRudpEnd() == 0);
    Require(sceRudpGetOption(context, 1, &optionValue, sizeof(optionValue)) == notInitialized);
    Require(sceRudpSetOption(context, 1, &optionValue, sizeof(optionValue)) == notInitialized);
    return 0;
}

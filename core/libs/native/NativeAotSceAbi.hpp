#ifndef NATIVE_AOT_SCE_ABI_HPP
#define NATIVE_AOT_SCE_ABI_HPP

#include "native/NativeAotGuest.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

namespace NativeAotSce {

template<class T>
inline constexpr bool ScalarArgument = (std::is_integral_v<T> || std::is_same_v<T, float> || std::is_same_v<T, double> ||
    (std::is_pointer_v<T> && !std::is_function_v<std::remove_pointer_t<T>>)) && sizeof(T) <= 8;

template<class Value, std::size_t Index, class... Values>
Value Scalar(const NativeAotGuestState& state) {
    static_assert((ScalarArgument<Values> && ...));
    constexpr std::array<bool, sizeof...(Values)> floating{std::is_floating_point_v<Values>...};
    std::size_t integers = 0, floats = 0, spilled = 0;
    for (std::size_t index = 0; index < Index; ++index) {
        if (floating[index]) { if (floats++ >= 8) ++spilled; }
        else if (integers++ >= 6) ++spilled;
    }
    std::uint64_t raw;
    if constexpr (std::is_floating_point_v<Value>) {
        if (floats < 8) std::memcpy(&raw, state.Xmm[floats], sizeof(raw));
        else std::memcpy(&raw, reinterpret_cast<const void*>(state.Gpr[4] + 8 + spilled * 8), sizeof(raw));
        Value value;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    } else {
        if (integers < 6) raw = NativeAotGuest::Argument(state, integers);
        else std::memcpy(&raw, reinterpret_cast<const void*>(state.Gpr[4] + 8 + spilled * 8), sizeof(raw));
        if constexpr (std::is_pointer_v<Value>) return reinterpret_cast<Value>(raw);
        else return static_cast<Value>(raw);
    }
}

template<class Result, class... Values, std::size_t... Indices>
void CallScalar(NativeAotGuestState& state, Result (*function)(Values...), std::index_sequence<Indices...>) {
    if constexpr (std::is_void_v<Result>) function(Scalar<Values, Indices, Values...>(state)...);
    else {
        static_assert(ScalarArgument<Result>);
        const auto value = function(Scalar<Values, Indices, Values...>(state)...);
        if constexpr (std::is_floating_point_v<Result>) std::memcpy(state.Xmm[0], &value, sizeof(value));
        else if constexpr (std::is_pointer_v<Result>) NativeAotGuest::Return(state, reinterpret_cast<std::uint64_t>(value));
        else NativeAotGuest::Return(state, static_cast<std::uint64_t>(value));
    }
}

template<auto Function>
void ScalarBridge(NativeAotGuestState& state) {
    []<class Result, class... Values>(NativeAotGuestState& current, Result (*function)(Values...)) {
        CallScalar(current, function, std::index_sequence_for<Values...>{});
    }(state, Function);
}

}

#endif

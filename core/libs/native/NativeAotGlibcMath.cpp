#define _REENTRANT 1
#include "native/NativeAotGlibcSupport.hpp"
#include <cmath>
#include <cfenv>
#include <cstring>
#include <limits>

namespace {

using namespace NativeAotGlibcSupport;
using NativeAotGuest::DoubleArgument;
using NativeAotGuest::ReturnDouble;
int guestSigngam = 0;

struct MathError {
    int previous = errno;
    std::fenv_t environment{};
    MathError() {
        if (std::feholdexcept(&environment) != 0) Unsupported("cannot preserve math floating-point environment");
        errno = 0;
    }
    ~MathError() {
        const auto flags = std::fetestexcept(FE_ALL_EXCEPT);
        if (errno != 0) SetGuestErrno(GuestErrno(errno));
        else if (flags & FE_INVALID) SetGuestErrno(33);
        else if (flags & (FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW)) SetGuestErrno(34);
        errno = previous;
        if (std::feupdateenv(&environment) != 0) Unsupported("cannot restore math floating-point environment");
    }
};

template<double (*Function)(double)>
void Unary(State& state) {
    MathError error;
    ReturnDouble(state, Function(DoubleArgument(state, 0)));
}

template<double (*Function)(double)>
void IeeeUnary(State& state) {
    const auto previous = errno;
    ReturnDouble(state, Function(DoubleArgument(state, 0)));
    errno = previous;
}

template<double (*Function)(double, double)>
void Binary(State& state) {
    MathError error;
    ReturnDouble(state, Function(DoubleArgument(state, 0), DoubleArgument(state, 1)));
}

template<double (*Function)(int, double)>
void OrderedBessel(State& state) {
    MathError error;
    ReturnDouble(state, Function(Integer(state, 0), DoubleArgument(state, 0)));
}

void Fma(State& state) {
    MathError error;
    ReturnDouble(state, std::fma(DoubleArgument(state, 0), DoubleArgument(state, 1), DoubleArgument(state, 2)));
}

void Ldexp(State& state) {
    MathError error;
    ReturnDouble(state, std::ldexp(DoubleArgument(state, 0), Integer(state, 0)));
}

void Scalbln(State& state) {
    MathError error;
    ReturnDouble(state, std::scalbln(DoubleArgument(state, 0), Signed(state, 0)));
}

void Frexp(State& state) {
    ReturnDouble(state, std::frexp(DoubleArgument(state, 0), Pointer<int>(state, 0)));
}

void Modf(State& state) {
    ReturnDouble(state, std::modf(DoubleArgument(state, 0), Pointer<double>(state, 0)));
}

void LgammaR(State& state) {
    MathError error;
    ReturnDouble(state, ::lgamma_r(DoubleArgument(state, 0), Pointer<int>(state, 0)));
}

void Lgamma(State& state) {
    MathError error;
    ReturnDouble(state, ::lgamma_r(DoubleArgument(state, 0), &guestSigngam));
}

void Exp10(State& state) {
    MathError error;
    ReturnDouble(state, ::__exp10(DoubleArgument(state, 0)));
}

void Significand(State& state) {
    const auto value = DoubleArgument(state, 0);
    int exponent;
    const auto fraction = std::frexp(value, &exponent);
    ReturnDouble(state, std::isfinite(value) && value != 0 ? fraction * 2 : value);
}

void Scalb(State& state) {
    MathError error;
    const auto value = DoubleArgument(state, 0), power = DoubleArgument(state, 1);
    if (std::isnan(value) || std::isnan(power)) ReturnDouble(state, value * power);
    else if (std::isinf(power)) ReturnDouble(state, power > 0 ? value * power : value / -power);
    else if (power != std::trunc(power)) {
        errno = EDOM;
        ReturnDouble(state, std::numeric_limits<double>::quiet_NaN());
    } else {
        const auto exponent = power > 65000 ? 65000 : power < -65000 ? -65000 : static_cast<int>(power);
        ReturnDouble(state, std::scalbn(value, exponent));
    }
}

void Nexttoward(State& state) {
    const auto value = DoubleArgument(state, 0);
    std::uint64_t significand;
    std::uint16_t signExponent;
    std::memcpy(&significand, reinterpret_cast<const void*>(state.Gpr[4] + 8), 8);
    std::memcpy(&signExponent, reinterpret_cast<const void*>(state.Gpr[4] + 16), 2);
    const auto sign = signExponent >> 15;
    const auto exponent = signExponent & 0x7fff;
    if ((exponent != 0 && !(significand >> 63)) || (exponent == 0 && (significand >> 63)))
        Unsupported("nexttoward requires canonical x87 extended precision");
    if (std::isnan(value)) { ReturnDouble(state, value + value); return; }
    if (exponent == 0x7fff && significand != (std::uint64_t{1} << 63)) {
        ReturnDouble(state, std::numeric_limits<double>::quiet_NaN());
        return;
    }
    const bool negative = std::signbit(value);
    int binaryExponent = 0;
    const auto fraction = std::frexp(std::fabs(value), &binaryExponent);
    const std::uint64_t valueSignificand = std::isinf(value) ? (std::uint64_t{1} << 63)
        : value == 0 ? 0 : static_cast<std::uint64_t>(std::ldexp(fraction, 64));
    const int valueExponent = std::isinf(value) ? 0x7fff : value == 0 ? 0 : binaryExponent - 1 + 16383;
    const bool bothZero = value == 0 && significand == 0;
    const bool equal = bothZero || (sign == negative && exponent == valueExponent && significand == valueSignificand);
    if (equal) { ReturnDouble(state, std::copysign(value, sign ? -1.0 : 1.0)); return; }
    const bool magnitudeGreater = exponent > valueExponent || (exponent == valueExponent && significand > valueSignificand);
    const bool greater = sign != negative ? !sign : sign ? !magnitudeGreater : magnitudeGreater;
    MathError error;
    ReturnDouble(state, std::nextafter(value, greater ? std::numeric_limits<double>::infinity() : -std::numeric_limits<double>::infinity()));
}

const NativeAotGuest::ProviderBinding Bindings[] = {
    {"acos", Unary<std::acos>, nullptr},
    {"acosh", Unary<std::acosh>, nullptr},
    {"asin", Unary<std::asin>, nullptr},
    {"asinh", Unary<std::asinh>, nullptr},
    {"atan", Unary<std::atan>, nullptr},
    {"atanh", Unary<std::atanh>, nullptr},
    {"cbrt", Unary<std::cbrt>, nullptr},
    {"cos", Unary<std::cos>, nullptr},
    {"cosh", Unary<std::cosh>, nullptr},
    {"exp", Unary<std::exp>, nullptr},
    {"exp2", Unary<std::exp2>, nullptr},
    {"floor", IeeeUnary<std::floor>, nullptr},
    {"log", Unary<std::log>, nullptr},
    {"log10", Unary<std::log10>, nullptr},
    {"log2", Unary<std::log2>, nullptr},
    {"sin", Unary<std::sin>, nullptr},
    {"sinh", Unary<std::sinh>, nullptr},
    {"sqrt", Unary<std::sqrt>, nullptr},
    {"tan", Unary<std::tan>, nullptr},
    {"tanh", Unary<std::tanh>, nullptr},
    {"tgamma", Unary<std::tgamma>, nullptr},
    {"ceil", IeeeUnary<std::ceil>, nullptr},
    {"erf", Unary<std::erf>, nullptr},
    {"erfc", Unary<std::erfc>, nullptr},
    {"expm1", Unary<std::expm1>, nullptr},
    {"log1p", Unary<std::log1p>, nullptr},
    {"logb", IeeeUnary<std::logb>, nullptr},
    {"nearbyint", IeeeUnary<std::nearbyint>, nullptr},
    {"rint", IeeeUnary<std::rint>, nullptr},
    {"round", IeeeUnary<std::round>, nullptr},
    {"trunc", IeeeUnary<std::trunc>, nullptr},
    {"j0", Unary<::j0>, nullptr},
    {"j1", Unary<::j1>, nullptr},
    {"y0", Unary<::y0>, nullptr},
    {"y1", Unary<::y1>, nullptr},
    {"atan2", Binary<std::atan2>, nullptr},
    {"hypot", Binary<std::hypot>, nullptr},
    {"pow", Binary<std::pow>, nullptr},
    {"remainder", Binary<std::remainder>, nullptr},
    {"fdim", Binary<std::fdim>, nullptr},
    {"fmod", Binary<std::fmod>, nullptr},
    {"nextafter", Binary<std::nextafter>, nullptr},
    {"drem", Binary<std::remainder>, nullptr},
    {"jn", OrderedBessel<::jn>, nullptr},
    {"yn", OrderedBessel<::yn>, nullptr},
    {"fma", Fma, nullptr},
    {"ldexp", Ldexp, nullptr},
    {"scalbln", Scalbln, nullptr},
    {"frexp", Frexp, nullptr},
    {"modf", Modf, nullptr},
    {"lgamma_r", LgammaR, nullptr},
    {"lgamma", Lgamma, nullptr},
    {"gamma", Lgamma, nullptr},
    {"exp10", Exp10, nullptr},
    {"significand", Significand, nullptr},
    {"scalb", Scalb, nullptr},
    {"nexttoward", Nexttoward, nullptr},
    {"signgam", nullptr, &guestSigngam},
};

}

namespace NativeAotGlibcSupport {
void RegisterMath() {
    Register(Bindings, sizeof(Bindings) / sizeof(Bindings[0]));
}
}

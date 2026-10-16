#ifndef NATIVE_AOT_SCE_CRT_HPP
#define NATIVE_AOT_SCE_CRT_HPP

#include "native/NativeAotGuest.hpp"

namespace NativeAotSce {
NativeAotGuest::Provider CrtProvider(const char* name);
}

#endif

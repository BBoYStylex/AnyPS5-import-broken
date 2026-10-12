#include "prx/libc/include/general/VabiMacros.hpp"
#include <cerrno>
#include <cmath>
#include <cfenv>
extern "C" double APS5_VABI nearbyint_nid_postfix(double value) {
    const int savedError = errno;
    std::fenv_t environment;
    std::fegetenv(&environment);
    volatile double result = std::nearbyint(value);
    std::fesetenv(&environment);
    errno = savedError;
    return result;
}

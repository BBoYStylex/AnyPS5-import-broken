#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_SHADERS_REGISTRATIONPREPARATIONBUDGET_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_DRIVER_SHADERS_REGISTRATIONPREPARATIONBUDGET_HPP

#include <cstdint>

namespace AgcDriver::DriverDetail {

constexpr bool RegistrationPreparationAllowed(std::uint64_t processPrivateBytes, std::uint64_t commitBudgetBytes) noexcept {
    return processPrivateBytes < commitBudgetBytes;
}

}

#endif

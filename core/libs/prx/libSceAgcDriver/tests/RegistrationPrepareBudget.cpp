#include "prx/libSceAgcDriver/Execution/include/Driver/Shaders/RegistrationPreparationBudget.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}

int main() {
    try {
        using AgcDriver::DriverDetail::RegistrationPreparationAllowed;
        Require(RegistrationPreparationAllowed(0, 1), "preparation should be allowed below the budget");
        Require(RegistrationPreparationAllowed(1023, 1024), "preparation should be allowed immediately below the budget");
        Require(!RegistrationPreparationAllowed(1024, 1024), "preparation should defer at the budget");
        Require(!RegistrationPreparationAllowed(1025, 1024), "preparation should defer above the budget");
        Require(!RegistrationPreparationAllowed(0, 0), "a zero budget should defer every preparation");
        std::cout << "registration prepare budget tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

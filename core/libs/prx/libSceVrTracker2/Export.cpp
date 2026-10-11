#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceVrTracker2GetResult() {
    throw std::runtime_error(std::string(__func__) + ": not supported");
}

int APS5_VABI sceVrTracker2Initialize() {
    throw std::runtime_error(std::string(__func__) + ": not supported");
}

int APS5_VABI sceVrTracker2QueryMemory() {
    throw std::runtime_error(std::string(__func__) + ": not supported");
}

int APS5_VABI sceVrTracker2RegisterDevice() {
    throw std::runtime_error(std::string(__func__) + ": not supported");
}

int APS5_VABI sceVrTracker2SetCoordinateSystem() {
    throw std::runtime_error(std::string(__func__) + ": not supported");
}

int APS5_VABI sceVrTracker2UnregisterDevice() {
    throw std::runtime_error(std::string(__func__) + ": not supported");
}

}

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceVrTracker2GetResult() {
    throw std::runtime_error("libSceVrTracker2: VR tracking not supported");
    return 0;
}

int APS5_VABI sceVrTracker2Initialize() {
    throw std::runtime_error("libSceVrTracker2: VR tracking not supported");
    return 0;
}

int APS5_VABI sceVrTracker2QueryMemory() {
    throw std::runtime_error("libSceVrTracker2: VR tracking not supported");
    return 0;
}

int APS5_VABI sceVrTracker2RegisterDevice() {
    throw std::runtime_error("libSceVrTracker2: VR tracking not supported");
    return 0;
}

int APS5_VABI sceVrTracker2SetCoordinateSystem() {
    throw std::runtime_error("libSceVrTracker2: VR tracking not supported");
    return 0;
}

int APS5_VABI sceVrTracker2UnregisterDevice() {
    throw std::runtime_error("libSceVrTracker2: VR tracking not supported");
    return 0;
}

}

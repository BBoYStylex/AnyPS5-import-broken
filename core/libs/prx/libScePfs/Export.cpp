#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

std::size_t APS5_VABI scePfsGetWorkBufferSize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePfsValidate(void* pfsBuffer, int validationLevel) {
    (void)pfsBuffer;
    (void)validationLevel;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePfsGetUncompressedSize(void* pfsBuffer, std::size_t* size) {
    (void)pfsBuffer;
    (void)size;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePfsPread(void* outputBuffer, std::size_t length, std::size_t offset, void* pfsBuffer, void* workBuffer) {
    (void)outputBuffer;
    (void)length;
    (void)offset;
    (void)pfsBuffer;
    (void)workBuffer;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}

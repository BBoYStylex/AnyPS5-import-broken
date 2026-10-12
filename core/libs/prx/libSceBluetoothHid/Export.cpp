#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

#include <string>

extern "C" {

std::int32_t APS5_VABI sceBluetoothHidInit() {
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidRegisterCallback(void* callback, std::int32_t flags, void* param) {
    (void)flags;
    (void)param;
    if (callback == nullptr) throw std::invalid_argument(std::string(__func__) + ": null callback");
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidUnregisterCallback() {
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidRegisterDevice(std::uint16_t vendorId, std::uint16_t productId) {
    (void)vendorId;
    (void)productId;
    return 0;
}

std::int32_t APS5_VABI sceBluetoothHidUnregisterDevice(std::uint16_t vendorId, std::uint16_t productId) {
    (void)vendorId;
    (void)productId;
    return 0;
}

int APS5_VABI sceBluetoothHidGetDeviceName(std::uint16_t vendor_id, std::uint16_t product_id, char* name, std::size_t size) {
    if (!name || size == 0) APS5_INVALID_ARG_EX;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBluetoothHidGetInputReport(std::uint16_t vendor_id, std::uint16_t product_id, void* report, std::size_t* size) {
    if (!report || !size) APS5_INVALID_ARG_EX;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceBluetoothHidSetOutputReport(std::uint16_t vendor_id, std::uint16_t product_id, const void* report, std::size_t size) {
    if (!report || size == 0) APS5_INVALID_ARG_EX;
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}

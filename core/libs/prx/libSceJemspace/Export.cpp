#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <cstddef>
#include <cstring>

extern "C" {

int APS5_VABI sceJemspaceCalloc() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspaceCreate() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspaceDestroy() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspaceFree() {
    NotImplemented_nid_no_patch(__func__);
}

void APS5_VABI sceJemspaceGetStats(const void* space, void* stats) {
    if (space == nullptr || stats == nullptr) {
        return;
    }
    const auto* source = static_cast<const std::byte*>(space) + 104;
    auto* destination = static_cast<std::byte*>(stats);
    std::memmove(destination, source, 32);
    std::memmove(destination + 32, source + 32, 8);
}

int APS5_VABI sceJemspaceMalloc() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspaceMallocUsableSize() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspaceMemalign() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspacePosixMemalign() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspacePrintStats() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspacePurge() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspaceRealloc() {
    NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI sceJemspaceReallocalign() {
    NotImplemented_nid_no_patch(__func__);
}

}

#include "prx/libkernel/Pthread/include/Pthread.hpp"
#include "prx/libkernel/Pthread/Posix/Common.hpp"
#include <algorithm>
#include <cstring>

extern "C" {
int APS5_VABI scePthreadAttrSetstackaddr(PthreadAttr* attr, void* address);
int APS5_VABI scePthreadAttrGetsolosched(const PthreadAttr* attr, int* value);
bool IsInitialPthread_nid_no_patch();

int APS5_VABI pthread_attr_setstackaddr_nid_postfix(PthreadAttr* attr, void* address) {
    return PosixThread::ToErrno(scePthreadAttrSetstackaddr(attr, address));
}

int APS5_VABI pthread_attr_getsolosched_np_nid_postfix(const PthreadAttr* attr, int* value) {
    if (!attr || !*attr || !value) return PosixThread::GUEST_EINVAL;
    return PosixThread::ToErrno(scePthreadAttrGetsolosched(attr, value));
}

int APS5_VABI pthread_attr_getaffinity_np_nid_postfix(const PthreadAttr* attr, std::size_t size, void* mask) {
    constexpr std::size_t CpuSetSize = 32;
    if (!attr || !*attr) return PosixThread::GUEST_EINVAL;
    if (size < CpuSetSize) return 34;
    if (!mask) return PosixThread::GUEST_EFAULT;
    std::memset(mask, 0, size);
    if (!(*attr)->_affinityExplicit) {
        std::memset(mask, 0xff, CpuSetSize);
    } else {
        std::memcpy(mask, &(*attr)->_affinity, sizeof(KernelCpumask));
        std::memcpy(static_cast<unsigned char*>(mask) + sizeof(KernelCpumask), (*attr)->affinityTail.data(), (*attr)->affinityTail.size());
    }
    return 0;
}

int APS5_VABI pthread_attr_setaffinity_np_nid_postfix(PthreadAttr* attr, std::size_t size, const void* mask) {
    constexpr std::size_t CpuSetSize = 32;
    if (!attr || !*attr) return PosixThread::GUEST_EINVAL;
    if (!size || !mask) {
        (*attr)->_affinity = DEFAULT_THREAD_AFFINITY;
        (*attr)->_affinityExplicit = false;
        (*attr)->affinityTail.fill(0);
        return 0;
    }
    if (size < CpuSetSize) return 34;
    const auto* bytes = static_cast<const unsigned char*>(mask);
    if (std::any_of(bytes + CpuSetSize, bytes + size, [](unsigned char byte) { return byte != 0; }))
        return PosixThread::GUEST_EINVAL;
    std::memcpy(&(*attr)->_affinity, bytes, sizeof(KernelCpumask));
    std::memcpy((*attr)->affinityTail.data(), bytes + sizeof(KernelCpumask), (*attr)->affinityTail.size());
    (*attr)->_affinityExplicit = true;
    return 0;
}

int APS5_VABI pthread_main_np_nid_postfix() {
    return IsInitialPthread_nid_no_patch() ? 1 : 0;
}

}

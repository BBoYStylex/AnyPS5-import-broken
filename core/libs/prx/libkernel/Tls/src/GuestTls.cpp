#include "SceTypes.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include <pthread.h>

// The guest TLS contract produced by the relinker's Mach-O writer.
//
// A relinked image has no usable ELF PT_TLS, so the writer copies the segment's
// init image into a per-thread block and rewrites every %fs access into a
// call-out (core/relinker/elfpatcher/src/macos/MacOsImage.cpp). The stub:
//
//   * reads %fs:0, which on Darwin is pthread_self()->tsd, and
//   * calls Aps5GuestTlsAllocate with the image's TLS descriptor when it is
//     null, otherwise uses %fs:0 as the block base directly, and
//   * applies the original displacement against that base, so a negative
//     displacement reaches the header in front of the block.
//
// Aps5GuestTlsRegister is called from the module init stub so a thread has its
// block before any access runs.

namespace {

// Written by the relinker as MetaTlsDescriptor; stable for meta version 2.
struct GuestTlsDescriptor {
    std::uint64_t version;
    void* initImage;
    std::uint64_t initSize;
    std::uint64_t blockSize;
    std::uint64_t alignment;
};

static_assert(sizeof(GuestTlsDescriptor) == 40, "the descriptor is five 64-bit fields");

// %fs:-16 is the guest thread pointer, so the block needs a header in front of
// it. The stub reaches that header with a plain negative displacement from the
// block base, which is why the block starts TlsHeaderBytes into the allocation.
constexpr std::size_t TlsHeaderBytes = 16;

// The block lives in a thread-local variable rather than a heap allocation
// because the relinker binds Aps5GuestTlsGetAddr with an addend: a guest that
// imports __tls_get_addr takes the address of that symbol and adds its offset,
// so the symbol's address has to be the block and has to be per-thread.
constexpr std::size_t MaxBlockBytes = 256 * 1024;

// AnyPS5 emulates the guest's pthread keys in its own storage, so host key 0 is
// free to carry the guest TLS block for the %fs:0 lookup the stub performs.
pthread_key_t& TlsKey() {
    static pthread_key_t key = [] {
        pthread_key_t created = 0;
        if (pthread_key_create(&created, nullptr) != 0)
            throw std::runtime_error("guest TLS: cannot create the host thread key");
        return created;
    }();
    return key;
}

std::size_t AlignUp(std::size_t value, std::size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

// The block itself. It has to be a thread-local variable whose symbol carries
// no mangling: this toolchain emits a Mach-O __thread variable only outside an
// extern "C" block, so the name is forced with an asm label. The default
// visibility is needed because the whole build compiles with -fvisibility=hidden
// and the relinker binds this symbol by name.
__attribute__((visibility("default"))) __thread char GuestTlsBlock_nid_no_patch[MaxBlockBytes]
    __asm__("_Aps5GuestTlsGetAddr_nid_no_patch");

extern "C" {

#define Aps5GuestTlsGetAddr_nid_no_patch GuestTlsBlock_nid_no_patch

// Allocates this thread's TLS block from the image's descriptor, publishes it
// where the rewritten %fs accesses look, and returns the block base.
void* APS5_VABI Aps5GuestTlsAllocate_nid_no_patch(const GuestTlsDescriptor* descriptor) {
    if (descriptor == nullptr || descriptor->version != 1)
        throw std::runtime_error("guest TLS: unsupported descriptor version");

    const std::size_t blockSize = descriptor->blockSize != 0 ? descriptor->blockSize : 1;
    if (blockSize + TlsHeaderBytes > MaxBlockBytes)
        throw std::runtime_error("guest TLS: block of " + std::to_string(blockSize)
                                 + " bytes exceeds the " + std::to_string(MaxBlockBytes) + " byte limit");

    const std::size_t alignment = descriptor->alignment >= sizeof(void*) ? descriptor->alignment : sizeof(void*);
    auto* raw = Aps5GuestTlsGetAddr_nid_no_patch;
    std::memset(raw, 0, TlsHeaderBytes + AlignUp(blockSize, alignment));

    auto* block = raw + TlsHeaderBytes;
    if (descriptor->initImage != nullptr && descriptor->initSize != 0)
        std::memcpy(block, descriptor->initImage, descriptor->initSize);
    // %fs:-16 is the guest's thread pointer.
    std::memcpy(raw, &block, sizeof(block));

    pthread_setspecific(TlsKey(), block);
    return block;
}

// Called from the module init stub so the block exists before the first access.
void APS5_VABI Aps5GuestTlsRegister_nid_no_patch(const GuestTlsDescriptor* descriptor) {
    if (pthread_getspecific(TlsKey()) == nullptr) Aps5GuestTlsAllocate_nid_no_patch(descriptor);
}

} // extern "C"
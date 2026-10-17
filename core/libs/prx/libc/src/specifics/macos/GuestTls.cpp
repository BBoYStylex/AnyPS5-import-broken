#ifdef __APPLE__

// Guest TLS for relinked Mach-O modules. The macOS relinker (core/relinker/elfpatcher/src/macos/MacOsImage.cpp) gives
// each module a descriptor in its __meta section and rewrites every guest %fs access into a stub that reads the
// thread pointer from gs:[key*8], the pthread TSD slot of the module's key, and calls Allocate when it is still zero.
//
// Layout (FreeBSD/PS5 TLS variant II): the module's TLS block ends exactly at the thread pointer, and the thread
// pointer points at a small control block whose first word points back at it. Guest code reads TLS variables at
// negative offsets from the thread pointer, the self pointer at 0 and the stack guard at +0x28.

#include "prx/libc/include/general/AsmMacros.hpp"
#include <cpuid.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <pthread.h>

namespace {

struct GuestTlsDescriptor {
    std::uint64_t Active;
    std::uint64_t InitImage;
    std::uint64_t FileSize;
    std::uint64_t BlockSize;
    std::uint64_t Alignment;
    std::uint64_t Key;
};
static_assert(sizeof(GuestTlsDescriptor) == 0x30);

// Covers the self pointer at +0 and the stack guard at +0x28; guest code may not touch anything else above the thread
// pointer (the relinker rejects other non-negative displacements), so +8 holds the host allocation for freeing.
constexpr std::size_t ThreadControlBlockSize = 0x30;
constexpr std::size_t AllocationSlot = 0x08;
constexpr std::size_t StackGuardSlot = 0x28;

[[noreturn]] void Fatal(const char* message) {
    std::fprintf(stderr, "guest TLS: %s\n", message);
    std::abort();
}

std::mutex& RegistrationLock() {
    static std::mutex lock;
    return lock;
}

void FreeBlock(void* threadPointer) {
    if (threadPointer == nullptr) return;
    void* block = nullptr;
    std::memcpy(&block, static_cast<std::uint8_t*>(threadPointer) + AllocationSlot, sizeof(block));
    std::free(block);
}

std::uint64_t StackGuardValue() {
    static const std::uint64_t value = [] {
        std::uint64_t guard = 0;
        arc4random_buf(&guard, sizeof(guard));
        return guard & ~std::uint64_t{0xff};
    }();
    return value;
}

}

// Read by the Allocate trampoline: non-zero when the OS enables XSAVE, so AVX upper halves are saved as well.
extern "C" std::uint8_t Aps5GuestTlsUseXsave = [] {
    unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) return std::uint8_t{0};
    return static_cast<std::uint8_t>((ecx & bit_OSXSAVE) != 0);
}();

extern "C" void Aps5GuestTlsRegister_nid_no_patch(GuestTlsDescriptor* descriptor) {
    std::lock_guard lock(RegistrationLock());
    if (descriptor->Key != 0) return;
    pthread_key_t key{};
    if (pthread_key_create(&key, FreeBlock) != 0) Fatal("pthread_key_create failed");
    // macOS never hands out key 0, so 0 can mean "not registered yet".
    if (key == 0) Fatal("pthread_key_create returned key 0");
    descriptor->Key = key;
}

// Called through Aps5GuestTlsAllocate_nid_no_patch; returns the calling thread's thread pointer.
extern "C" void* Aps5GuestTlsAllocateBlock_nid_no_patch(GuestTlsDescriptor* descriptor) {
    if (descriptor->Key == 0) Aps5GuestTlsRegister_nid_no_patch(descriptor);
    const auto key = static_cast<pthread_key_t>(descriptor->Key);
    if (void* existing = pthread_getspecific(key)) return existing;
    const std::size_t alignment = descriptor->Alignment > 16 ? descriptor->Alignment : 16;
    const std::size_t tlsSize = (descriptor->BlockSize + alignment - 1) & ~(alignment - 1);
    const std::size_t total = tlsSize + ThreadControlBlockSize;
    void* block = nullptr;
    if (posix_memalign(&block, alignment, total) != 0 || block == nullptr) Fatal("out of memory for a thread block");
    auto* bytes = static_cast<std::uint8_t*>(block);
    std::memset(bytes, 0, total);
    auto* threadPointer = bytes + tlsSize;
    auto* image = threadPointer - descriptor->BlockSize;
    if (descriptor->FileSize != 0 && descriptor->InitImage != 0)
        std::memcpy(image, reinterpret_cast<const void*>(descriptor->InitImage), descriptor->FileSize);
    void* self = threadPointer;
    std::memcpy(threadPointer, &self, sizeof(self));
    std::memcpy(threadPointer + AllocationSlot, &block, sizeof(block));
    const auto guard = StackGuardValue();
    std::memcpy(threadPointer + StackGuardSlot, &guard, sizeof(guard));
    if (pthread_setspecific(key, threadPointer) != 0) Fatal("pthread_setspecific failed");
    return threadPointer;
}

// __tls_get_addr for modules whose relocations use the platform resolver: {module, offset}, where module is the
// address of the module's descriptor (written by the relinker into each DTPMOD slot) and offset is from the start
// of the module's block.
struct GuestTlsIndex {
    std::uint64_t Module;
    std::uint64_t Offset;
};

extern "C" void* Aps5GuestTlsGetAddr_nid_no_patch(const GuestTlsIndex* index) {
    auto* descriptor = reinterpret_cast<GuestTlsDescriptor*>(index->Module);
    if (descriptor == nullptr) Fatal("__tls_get_addr with no module");
    auto* threadPointer = static_cast<std::uint8_t*>(
        descriptor->Key != 0 ? pthread_getspecific(static_cast<pthread_key_t>(descriptor->Key)) : nullptr);
    if (threadPointer == nullptr) threadPointer = static_cast<std::uint8_t*>(Aps5GuestTlsAllocateBlock_nid_no_patch(descriptor));
    return threadPointer - descriptor->BlockSize + index->Offset;
}

// The relinker's TLS stubs call this with the descriptor in rax and expect the thread pointer back in rax. They save
// only the flags (and sometimes rax) themselves, so every other general register and all x87/SSE/AVX state must
// survive. The stub has already moved rsp down by 0x80, so the interrupted guest code's red zone is safe.
asm(
    ".text\n"
    ".p2align 4\n"
    APS5_ASM_FUNCTION("Aps5GuestTlsAllocate_nid_no_patch")
    "    pushq %rbp\n"
    "    movq %rsp, %rbp\n"
    "    pushq %rcx\n"
    "    pushq %rdx\n"
    "    pushq %rsi\n"
    "    pushq %rdi\n"
    "    pushq %r8\n"
    "    pushq %r9\n"
    "    pushq %r10\n"
    "    pushq %r11\n"
    "    movq %rax, %rdi\n"
    "    andq $-64, %rsp\n"
    "    subq $4096, %rsp\n"
    "    cmpb $0, " APS5_ASM_SYMBOL("Aps5GuestTlsUseXsave") "(%rip)\n"
    "    je 1f\n"
    "    movq %rsp, %rcx\n"
    "    movl $512, %edx\n"
    "2:  movq $0, (%rcx,%rdx)\n"
    "    addq $8, %rdx\n"
    "    cmpq $576, %rdx\n"
    "    jb 2b\n"
    "    movl $-1, %eax\n"
    "    movl $-1, %edx\n"
    "    xsave64 (%rsp)\n"
    "    call " APS5_ASM_SYMBOL("Aps5GuestTlsAllocateBlock_nid_no_patch") "\n"
    "    movq %rax, %rsi\n"
    "    movl $-1, %eax\n"
    "    movl $-1, %edx\n"
    "    xrstor64 (%rsp)\n"
    "    movq %rsi, %rax\n"
    "    jmp 3f\n"
    "1:  fxsave64 (%rsp)\n"
    "    call " APS5_ASM_SYMBOL("Aps5GuestTlsAllocateBlock_nid_no_patch") "\n"
    "    fxrstor64 (%rsp)\n"
    "3:  leaq -64(%rbp), %rsp\n"
    "    popq %r11\n"
    "    popq %r10\n"
    "    popq %r9\n"
    "    popq %r8\n"
    "    popq %rdi\n"
    "    popq %rsi\n"
    "    popq %rdx\n"
    "    popq %rcx\n"
    "    popq %rbp\n"
    "    ret\n"
    APS5_ASM_FUNCTION_END("Aps5GuestTlsAllocate_nid_no_patch"));

#endif

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cwchar>
#include <cwctype>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/HeapDiagnostics.hpp"

uint32_t Need_sceLibc = 1;

extern "C" {

    int APS5_VABI _ZSt13_Execute_onceRSt9once_flagPFiPvS1_PS1_ES1__nid_postfix(int*, int (APS5_VABI *)(void*, void*, void**), void*);

    int APS5_VABI std_execute_once_nid_postfix(int* flag, int (APS5_VABI *func)(void*, void*, void**), void* arg) {
        return _ZSt13_Execute_onceRSt9once_flagPFiPvS1_PS1_ES1__nid_postfix(flag, func, arg);
    }

    void APS5_VABI LibcHeapGetTraceInfo_nid_postfix(LibcHeapInfo* info) {
        LibcHeapTraceInfo_nid_no_patch(info);
    }

// Dead import of Cyberpunk 2077 (PPSA04029): no call sites, but the
// Windows loader resolves imports strictly, so it must be present.
int APS5_VABI _ZSt14_Atomic_assertPKcS0__nid_postfix() {
 NotImplemented_nid_no_patch(__func__);
 return 0;
}


APS5_EXPORT("H2e8t5ScQGc", libc___cxa_finalize);
void APS5_VABI libc___cxa_finalize(void* dsoHandle) {
    CxaFinalize_nid_no_patch(dsoHandle);
}

// Cyberpunk 2077 (PPSA04029): vsnprintf_s used for string formatting.
int APS5_VABI vsnprintf_s(char* buf, std::size_t bufSize, std::size_t count, const char* fmt, ...) {
 if (buf == nullptr || bufSize == 0 || fmt == nullptr) return -1;
 std::size_t limit = (count == static_cast<std::size_t>(-1) || count >= bufSize) ? bufSize - 1 : count;
 va_list ap;
 va_start(ap, fmt);
 int result = std::vsnprintf(buf, limit + 1, fmt, ap);
 va_end(ap);
 return result;
}

// Cyberpunk 2077 (PPSA04029): vsscanf_s used for string parsing.
int APS5_VABI vsscanf_s(const char* buf, const char* fmt, ...) {
 if (buf == nullptr || fmt == nullptr) return -1;
 va_list ap;
 va_start(ap, fmt);
 int result = std::vsscanf(buf, fmt, ap);
 va_end(ap);
 return result;
}

// Cyberpunk 2077 (PPSA04029): standard abs for long long values.
long long APS5_VABI abs(long long x) {
 return x < 0 ? -x : x;
}

std::uint64_t APS5_VABI _Mtx_destroy(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Cnd_destroy(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Iswctype(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI __powidf2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _WStoul(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Cnd_init(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Cnd_broadcast(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Mtx_init(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Mtx_unlock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Mtx_lock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::uint64_t APS5_VABI _Cnd_wait(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}
}

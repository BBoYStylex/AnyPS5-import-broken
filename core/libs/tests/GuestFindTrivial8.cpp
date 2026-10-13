#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

extern "C" const void* APS5_VABI __std_find_trivial_8_nid_postfix(const void*, const void*, std::uint64_t) noexcept;
#if defined(__x86_64__)
extern "C" std::uintptr_t APS5_VABI Find8RegisterProbe(const void*, const void*, std::uint64_t) noexcept;

asm(".text\n.globl Find8RegisterProbe\nFind8RegisterProbe:\n"
    "subq $8, %rsp\nxorl %ecx, %ecx\nxorl %r8d, %r8d\nxorl %r9d, %r9d\n"
    "call __std_find_trivial_8_nid_postfix\naddq $8, %rsp\nret\n");
#endif
static void Check(bool value, int line) {
    if (!value) { std::fprintf(stderr,"Find8 check failed at %d\n",line); std::abort(); }
}
#define Require(value) Check((value),__LINE__)
static const void* Find(const void* first, const void* last, std::uint64_t value) {
    const void* (APS5_VABI *volatile function)(const void*,const void*,std::uint64_t) noexcept = __std_find_trivial_8_nid_postfix;
    const auto result = function(first,last,value);
#if defined(__x86_64__)
    Require(Find8RegisterProbe(first,last,value) == reinterpret_cast<std::uintptr_t>(result));
#endif
    return result;
}
static void Store(void* address, std::uint64_t value) { std::memcpy(address,&value,sizeof(value)); }
struct GuardedPages {
    std::size_t page;
    unsigned char* base;
    GuardedPages() {
#ifdef _WIN32
        SYSTEM_INFO info; GetSystemInfo(&info); page = info.dwPageSize;
        base = static_cast<unsigned char*>(VirtualAlloc(nullptr,page*3,MEM_RESERVE,PAGE_NOACCESS));
        Require(base && VirtualAlloc(base+page,page,MEM_COMMIT,PAGE_READWRITE) == base+page);
#else
        page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
        base = static_cast<unsigned char*>(mmap(nullptr,page*3,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
        Require(base != MAP_FAILED && mprotect(base+page,page,PROT_READ|PROT_WRITE) == 0);
#endif
    }
    ~GuardedPages() {
#ifdef _WIN32
        VirtualFree(base,0,MEM_RELEASE);
#else
        munmap(base,page*3);
#endif
    }
};
#ifdef _WIN32
static void RequireChildFailure(const wchar_t* mode) {
    std::wstring executable(32768,L'\0');
    const auto length = GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
    Require(length > 0 && length < executable.size()); executable.resize(length);
    std::wstring command = L"\"" + executable + L"\" " + mode;
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    Require(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process));
    Require(WaitForSingleObject(process.hProcess,10000) == WAIT_OBJECT_0);
    DWORD code = 0; Require(GetExitCodeProcess(process.hProcess,&code));
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    Require(code != 0 && code != STILL_ACTIVE);
}
#endif
int main(int argc, char** argv) {
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    if (argc == 2) {
        std::array<std::uint64_t,2> values{1,2};
        if (std::strcmp(argv[1],"null") == 0) Find(nullptr,values.data(),3);
        else if (std::strcmp(argv[1],"reversed") == 0) Find(values.data()+1,values.data(),3);
        else if (std::strcmp(argv[1],"partial") == 0) Find(values.data(),reinterpret_cast<unsigned char*>(values.data())+7,3);
        else if (std::strcmp(argv[1],"unreadable") == 0) {
            GuardedPages pages; Find(pages.base,pages.base+8,3);
        }
        return 0;
    }
    std::array<std::uint64_t,6> values{0,7,7,0x8000000000000000ULL,0xffffffffffffffffULL,42};
    const auto end = values.data()+values.size();
    Require(Find(values.data(),end,0) == values.data());
    Require(Find(values.data(),end,42) == end-1);
    Require(Find(values.data(),end,7) == values.data()+1);
    Require(Find(values.data(),end,99) == end);
    Require(Find(values.data(),end,0x8000000000000000ULL) == values.data()+3);
    Require(Find(values.data(),end,std::numeric_limits<std::uint64_t>::max()) == values.data()+4);
    Require(Find(nullptr,nullptr,0) == nullptr);
    Require(Find(end,end,42) == end);
    std::array<unsigned char,64> unaligned{};
    for (std::size_t offset = 0; offset < 8; ++offset) {
        auto* first = unaligned.data()+offset;
        Store(first,3); Store(first+8,0xffffffffffffffffULL); Store(first+16,3);
        Require(Find(first,first+24,3) == first);
        Require(Find(first,first+24,0xffffffffffffffffULL) == first+8);
        Require(Find(first,first+24,4) == first+24);
    }
    GuardedPages pages;
    auto* last = pages.base+2*pages.page;
    Require(Find(last,last,1) == last);
    for (std::size_t count = 1; count <= 65; ++count) {
        auto* first = last-count*8;
        for (std::size_t index = 0; index < count; ++index) Store(first+index*8,index);
        Require(Find(first,last,count-1) == last-8);
        Require(Find(first,last,999) == last);
    }

    Store(last-8,123);
    Require(Find(last-8,last+8,123) == last-8);
#ifdef _WIN32
    for (const auto mode : {L"null",L"reversed",L"partial",L"unreadable"}) RequireChildFailure(mode);
#endif
    std::puts("find8: first/last/duplicate/absent/empty/unaligned/64-bit/RAX/guarded bounds passed");
}

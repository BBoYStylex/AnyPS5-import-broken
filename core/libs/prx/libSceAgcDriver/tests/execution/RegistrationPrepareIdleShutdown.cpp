#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>
#else
#include <filesystem>
#endif

namespace {

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("idle registration preparation shutdown: " + message);
}

std::set<std::uint64_t> ThreadIds() {
    std::set<std::uint64_t> ids;
#ifdef _WIN32
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    Require(snapshot != INVALID_HANDLE_VALUE, "cannot list the process threads");
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    for (BOOL more = Thread32First(snapshot, &entry); more; more = Thread32Next(snapshot, &entry)) {
        if (entry.th32OwnerProcessID == GetCurrentProcessId()) ids.insert(entry.th32ThreadID);
    }
    CloseHandle(snapshot);
#else
    for (const auto& task : std::filesystem::directory_iterator("/proc/self/task")) ids.insert(std::stoull(task.path().filename().string()));
#endif
    return ids;
}

bool ProgramThread(std::uint64_t id) {
#ifdef _WIN32
    using Query = NTSTATUS(NTAPI*)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto query = reinterpret_cast<Query>(reinterpret_cast<void*>(GetProcAddress(ntdll, "NtQueryInformationThread")));
    Require(query != nullptr, "cannot find NtQueryInformationThread");
    const HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, static_cast<DWORD>(id));
    if (thread == nullptr) return false;
    void* start = nullptr;
    const NTSTATUS status = query(thread, ThreadQuerySetWin32StartAddress, &start, sizeof(start), nullptr);
    CloseHandle(thread);
    Require(status >= 0, "cannot read the start address of a new thread");
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCWSTR>(start), &module);
    return module != ntdll;
#else
    static_cast<void>(id);
    return true;
#endif
}

}

int main() {
    try {
        AgcDriver::DriverDetail::Driver::Get();
        const auto before = ThreadIds();
        LibcRunShutdown_nid_postfix();
        for (const auto id : ThreadIds()) Require(before.contains(id) || !ProgramThread(id), "shutdown left a thread running that it started");
        std::puts("shutdown without registration preparations started no threads");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

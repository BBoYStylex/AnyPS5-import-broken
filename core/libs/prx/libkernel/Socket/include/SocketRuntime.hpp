#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "prx/libkernel/Socket/include/SocketPoll.hpp"
namespace GuestSockets {
struct Iovec {
    void* base;
    std::size_t length;
};
int Duplicate(int descriptor);
int DuplicateTo(int descriptor, int target);
int Close(int descriptor);
void Forget(int descriptor);
bool IsOpen(int descriptor);
int Family(int descriptor);
std::int64_t Read(int descriptor, void* buffer, std::size_t length);
std::int64_t Write(int descriptor, const void* buffer, std::size_t length);
bool Ready(int descriptor, bool write, std::int64_t* data, bool* eof);
int Poll(KernelSocketPoll::Entry* entries, int count, int timeoutMilliseconds);
// what a guest descriptor is, for the calls whose filters FreeBSD answers with EINVAL: a socket and
// a pipe have read and write filters, a file and a directory only have vnode ones, the rest has none
enum class DescriptorKind { Socket, Pipe, File, Other };
DescriptorKind Kind(int descriptor);
// polls a socket or a pipe: data is the bytes available for a read or the free space for a write, and
// eof is set when the other end is closed, as filt_soread, filt_sowrite, filt_piperead and
// filt_pipewrite report them
bool FilterReady(int descriptor, bool write, std::int64_t* data, bool* eof);
struct Interest {
    int descriptor;
    bool write;
    // a low water mark poll cannot express (NOTE_LOWAT, the size of sceKernelAddReadEvent and
    // sceKernelAddWriteEvent): the wait returns to its caller every millisecond while one is pending
    std::int64_t minimum = 0;
};
std::uintptr_t CurrentWaker();
void Wake(std::uintptr_t waker);
bool WaitAny(const std::vector<Interest>& interests, std::uint64_t deadlineNanos);
std::int64_t Readv(int descriptor, const Iovec* iov, int iovcnt);
std::int64_t Writev(int descriptor, const Iovec* iov, int iovcnt);
}

extern "C" bool GuestSocketIsOpen_nid_no_patch(int descriptor);

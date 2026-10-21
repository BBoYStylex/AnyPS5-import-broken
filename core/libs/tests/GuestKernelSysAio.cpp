#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/RemoveTree.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <source_location>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

struct GuestSigevent {
    int notify;
    int signo;
    void* value;
    union {
        std::int32_t threadId;
        std::uint16_t keventFlags;
        std::int64_t spare[8];
    };
};

struct GuestAiocb {
    int fildes;
    std::int64_t offset;
    void* buf;
    std::size_t nbytes;
    int spare[2];
    void* spare2;
    int lioOpcode;
    int reqprio;
    std::int64_t status;
    std::int64_t error;
    void* kernelinfo;
    GuestSigevent sigevent;
};
static_assert(sizeof(GuestAiocb) == 160 && offsetof(GuestAiocb, status) == 56 && offsetof(GuestAiocb, sigevent) == 80);

using Handler = void (APS5_VABI *)(int);

extern "C" {
int APS5_VABI aio_read_nid_postfix(GuestAiocb*);
int APS5_VABI aio_write_nid_postfix(GuestAiocb*);
int APS5_VABI aio_fsync_nid_postfix(int, GuestAiocb*);
int APS5_VABI aio_error_nid_postfix(const GuestAiocb*);
std::int64_t APS5_VABI aio_return_nid_postfix(GuestAiocb*);
int APS5_VABI aio_suspend_nid_postfix(const GuestAiocb* const*, int, const KernelTimespec*);
int APS5_VABI aio_cancel_nid_postfix(int, GuestAiocb*);
std::int64_t APS5_VABI aio_waitcomplete_nid_postfix(GuestAiocb**, const KernelTimespec*);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI close_nid_postfix(int);
int APS5_VABI pipe_nid_postfix(int*);
Handler APS5_VABI signal_nid_postfix(int, Handler);
int APS5_VABI pthread_getthreadid_np_nid_postfix(void);
int* APS5_VABI __error_nid_postfix();
}

namespace {

constexpr int Einval = 22;
constexpr int Ebadf = 9;
constexpr int Efault = 14;
constexpr int Eagain = 35;
constexpr int Einprogress = 36;
constexpr int Eopnotsupp = 45;
constexpr int Sync = 0x80;
constexpr int AllDone = 3;
constexpr int SigevSignal = 1;
constexpr int SigevKevent = 3;
constexpr int SigevThreadId = 4;
constexpr int Sigusr1 = 30;
void* const NoJob = reinterpret_cast<void*>(std::intptr_t{-1});

volatile std::sig_atomic_t received = 0;
void APS5_VABI Callback(int value) { received = value; }

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "AIO check failed at line %u (guest errno %d)\n", location.line(), *__error_nid_postfix());
        std::abort();
    }
}

bool Fails(std::int64_t result, int error) {
    return result == -1 && *__error_nid_postfix() == error;
}

GuestAiocb Request(int descriptor, std::int64_t offset, void* buffer, std::size_t length) {
    GuestAiocb request{};
    request.fildes = descriptor;
    request.offset = offset;
    request.buf = buffer;
    request.nbytes = length;
    return request;
}

bool ReadThrows(GuestAiocb& request) {
    try {
        aio_read_nid_postfix(&request);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void BeforeFirstRequest(int descriptor, GuestAiocb& request) {
    const GuestAiocb* list[] = {&request};
    const KernelTimespec invalid{0, 1000000000};
    GuestAiocb* completed = &request;
    Require(aio_error_nid_postfix(&request) == Einval);
    Require(Fails(aio_suspend_nid_postfix(list, 1, nullptr), Eagain));
    Require(Fails(aio_return_nid_postfix(&request), Einval));
    Require(aio_cancel_nid_postfix(descriptor, nullptr) == AllDone);
    Require(Fails(aio_cancel_nid_postfix(-1, nullptr), Ebadf));
    Require(Fails(aio_fsync_nid_postfix(0, &request), Einval));
    Require(Fails(aio_waitcomplete_nid_postfix(&completed, &invalid), Einval) && completed == nullptr);
    Require(Fails(aio_suspend_nid_postfix(list, 1, nullptr), Eagain));
}

void Transfers(int descriptor) {
    char buffer[16] = {};
    GuestAiocb request = Request(descriptor, 2, buffer, 4);
    const GuestAiocb* list[] = {&request};
    request.lioOpcode = 99;
    request.error = 7;
    Require(aio_read_nid_postfix(&request) == 0);
    Require(request.status == -1 && request.error == Einprogress && request.kernelinfo != NoJob && request.lioOpcode == 99);
    Require(aio_error_nid_postfix(&request) == 0 && aio_suspend_nid_postfix(list, 1, nullptr) == 0);
    Require(aio_cancel_nid_postfix(descriptor, &request) == AllDone && aio_cancel_nid_postfix(descriptor, nullptr) == AllDone);
    Require(aio_return_nid_postfix(&request) == 4 && std::memcmp(buffer, "cdef", 4) == 0);
    Require(request.status == 4 && request.error == 0);
    Require(Fails(aio_return_nid_postfix(&request), Einval) && aio_error_nid_postfix(&request) == Einval);

    char patch[] = "XY";
    GuestAiocb write = Request(descriptor, 6, patch, 2);
    Require(aio_write_nid_postfix(&write) == 0 && write.kernelinfo != request.kernelinfo && aio_return_nid_postfix(&write) == 2);
    GuestAiocb whole = Request(descriptor, 0, buffer, sizeof(buffer));
    Require(aio_read_nid_postfix(&whole) == 0 && aio_return_nid_postfix(&whole) == 8 && std::memcmp(buffer, "abcdefXY", 8) == 0);
    GuestAiocb past = Request(descriptor, 100, buffer, 4);
    Require(aio_read_nid_postfix(&past) == 0 && aio_return_nid_postfix(&past) == 0);

    GuestAiocb negative = Request(descriptor, -5, buffer, 1);
    Require(aio_read_nid_postfix(&negative) == 0 && aio_error_nid_postfix(&negative) == Einval);
    Require(Fails(aio_return_nid_postfix(&negative), Einval) && negative.status == -1 && negative.error == Einval);
    Require(aio_error_nid_postfix(&negative) == Einval);
    negative.nbytes = 0;
    Require(aio_read_nid_postfix(&negative) == 0 && aio_return_nid_postfix(&negative) == 0);

    GuestAiocb flush = Request(descriptor, -1, nullptr, 0);
    Require(Fails(aio_fsync_nid_postfix(0, &flush), Einval) && flush.error == 0 && flush.kernelinfo == nullptr);
    Require(aio_fsync_nid_postfix(Sync, &flush) == 0 && aio_return_nid_postfix(&flush) == 0);
}

void Rejections(int descriptor, const std::string& path) {
    char buffer[4] = {};
    GuestAiocb rejected = Request(descriptor, -1, buffer, 1);
    Require(Fails(aio_read_nid_postfix(&rejected), Einval) && rejected.status == -1 && rejected.error == Einval);
    Require(rejected.kernelinfo == NoJob && aio_error_nid_postfix(&rejected) == Einval);
    Require(Fails(aio_return_nid_postfix(&rejected), Einval));
    Require(Fails(aio_read_nid_postfix(nullptr), Efault) && Fails(aio_fsync_nid_postfix(Sync, nullptr), Efault));

    GuestAiocb closed = Request(-1, 0, buffer, 1);
    Require(Fails(aio_read_nid_postfix(&closed), Ebadf) && closed.error == Ebadf && aio_error_nid_postfix(&closed) == Ebadf);
    Require(Fails(aio_fsync_nid_postfix(Sync, &closed), Ebadf));

    GuestAiocb huge = Request(descriptor, 0, buffer, std::numeric_limits<std::size_t>::max());
    Require(Fails(aio_read_nid_postfix(&huge), Einval) && huge.error == 0 && huge.status == -1);

    GuestAiocb notified = Request(descriptor, 0, buffer, 1);
    for (int notify : {2, 5, -1}) {
        notified.sigevent.notify = notify;
        Require(Fails(aio_read_nid_postfix(&notified), Einval) && notified.error == Einval);
    }
    for (int notify : {SigevSignal, SigevThreadId}) {
        for (int signo : {0, 129}) {
            notified.sigevent.notify = notify;
            notified.sigevent.signo = signo;
            Require(Fails(aio_write_nid_postfix(&notified), Einval) && notified.error == 0);
        }
    }
    notified.sigevent = {};
    notified.sigevent.notify = SigevKevent;
    notified.sigevent.keventFlags = 0x0001;
    Require(Fails(aio_read_nid_postfix(&notified), Einval) && notified.error == Einval && notified.kernelinfo != NoJob);
    notified.sigevent.keventFlags = 0x0020;
    Require(ReadThrows(notified));

    const int readOnly = open_nid_postfix(path.c_str(), SCE_KERNEL_O_RDONLY, 0);
    const int writeOnly = open_nid_postfix(path.c_str(), SCE_KERNEL_O_WRONLY, 0);
    Require(readOnly >= 0 && writeOnly >= 0);
    GuestAiocb wrongWay = Request(writeOnly, 0, buffer, 1);
    Require(Fails(aio_read_nid_postfix(&wrongWay), Ebadf) && wrongWay.error == Ebadf);
    wrongWay.fildes = readOnly;
    Require(Fails(aio_write_nid_postfix(&wrongWay), Ebadf) && wrongWay.error == Ebadf);
    Require(aio_fsync_nid_postfix(Sync, &wrongWay) == 0 && aio_return_nid_postfix(&wrongWay) == 0);
    Require(close_nid_postfix(readOnly) == 0 && close_nid_postfix(writeOnly) == 0);

    int pipes[2] = {};
    Require(pipe_nid_postfix(pipes) == 0);
    GuestAiocb piped = Request(pipes[0], 0, buffer, 1);
    Require(Fails(aio_read_nid_postfix(&piped), Eopnotsupp) && piped.error == Eopnotsupp && piped.kernelinfo != NoJob);
    Require(aio_error_nid_postfix(&piped) == Eopnotsupp && aio_cancel_nid_postfix(pipes[0], nullptr) == AllDone);
    piped.fildes = pipes[1];
    Require(Fails(aio_write_nid_postfix(&piped), Eopnotsupp));
    Require(Fails(aio_fsync_nid_postfix(Sync, &piped), Einval) && piped.kernelinfo == NoJob);
    Require(close_nid_postfix(pipes[0]) == 0 && close_nid_postfix(pipes[1]) == 0);
}

void Waiting(int descriptor) {
    char buffer[8] = {};
    const KernelTimespec poll{0, 0};
    const KernelTimespec brief{0, 1000000};
    const KernelTimespec negative{-1, 0};
    const KernelTimespec overlong{0, 1000000000};
    GuestAiocb* completed = nullptr;
    Require(Fails(aio_waitcomplete_nid_postfix(&completed, &poll), Eagain) && completed == nullptr);
    Require(Fails(aio_waitcomplete_nid_postfix(&completed, &brief), Eagain));
    Require(Fails(aio_waitcomplete_nid_postfix(&completed, &negative), Einval));

    GuestAiocb idle{};
    const GuestAiocb* idleList[] = {&idle};
    Require(Fails(aio_suspend_nid_postfix(idleList, 17, nullptr), Einval) && Fails(aio_suspend_nid_postfix(idleList, -1, nullptr), Einval));
    Require(Fails(aio_suspend_nid_postfix(nullptr, 1, nullptr), Efault) && aio_suspend_nid_postfix(nullptr, 0, nullptr) == 0);
    Require(Fails(aio_suspend_nid_postfix(idleList, 1, &negative), Einval) && Fails(aio_suspend_nid_postfix(idleList, 1, &overlong), Einval));
    Require(aio_suspend_nid_postfix(idleList, 1, nullptr) == 0);

    GuestAiocb first = Request(descriptor, 0, buffer, 2);
    GuestAiocb second = Request(descriptor, 2, buffer + 2, 3);
    Require(aio_read_nid_postfix(&first) == 0 && aio_read_nid_postfix(&second) == 0);
    const GuestAiocb* both[] = {&idle, &second};
    Require(aio_suspend_nid_postfix(both, 2, &poll) == 0);
    Require(aio_waitcomplete_nid_postfix(&completed, nullptr) == 2 && completed == &first && first.status == 2 && first.error == 0);
    Require(aio_waitcomplete_nid_postfix(&completed, &poll) == 3 && completed == &second && second.status == 3);
    Require(Fails(aio_return_nid_postfix(&first), Einval));

    GuestAiocb failing = Request(descriptor, -2, buffer, 1);
    Require(aio_read_nid_postfix(&failing) == 0);
    Require(Fails(aio_waitcomplete_nid_postfix(&completed, nullptr), Einval) && completed == &failing);
    Require(failing.status == -1 && failing.error == Einval);

    GuestAiocb late = Request(descriptor, 1, buffer, 1);
    std::thread submitter([&late] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Require(aio_read_nid_postfix(&late) == 0);
    });
    Require(aio_waitcomplete_nid_postfix(&completed, nullptr) == 1 && completed == &late);
    submitter.join();
}

void QueueLimit(int descriptor) {
    char buffer[1] = {};
    std::vector<GuestAiocb> requests(256, Request(descriptor, 0, buffer, 1));
    for (auto& request : requests) Require(aio_read_nid_postfix(&request) == 0);
    GuestAiocb overflow = Request(descriptor, 0, buffer, 1);
    Require(Fails(aio_read_nid_postfix(&overflow), Eagain) && overflow.error == Eagain && overflow.status == -1);
    Require(Fails(aio_fsync_nid_postfix(Sync, &overflow), Eagain));
    for (auto& request : requests) Require(aio_return_nid_postfix(&request) == 1);
    Require(aio_read_nid_postfix(&overflow) == 0 && aio_return_nid_postfix(&overflow) == 1);
}

void Signals(int descriptor) {
    char buffer[1] = {};
    Require(signal_nid_postfix(Sigusr1, Callback) != reinterpret_cast<Handler>(std::intptr_t{-1}));
    GuestAiocb signalled = Request(descriptor, 0, buffer, 1);
    signalled.sigevent.notify = SigevSignal;
    signalled.sigevent.signo = Sigusr1;
    received = 0;
    Require(aio_read_nid_postfix(&signalled) == 0 && received == Sigusr1 && aio_return_nid_postfix(&signalled) == 1);
    signalled.sigevent.notify = SigevThreadId;
    signalled.sigevent.threadId = pthread_getthreadid_np_nid_postfix();
    received = 0;
    Require(aio_read_nid_postfix(&signalled) == 0 && received == Sigusr1 && aio_return_nid_postfix(&signalled) == 1);
    signalled.sigevent.threadId += 1;
    received = 0;
    Require(ReadThrows(signalled) && received == 0);
}

}

int main() {
    const auto root = std::filesystem::path("anyps5-kernel-sys-aio-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Require(std::filesystem::create_directory(root));
    const auto path = (root / "data.bin").string();
    std::ofstream(path, std::ios::binary) << "abcdefgh";
    const int descriptor = open_nid_postfix(path.c_str(), SCE_KERNEL_O_RDWR, 0);
    Require(descriptor >= 0);
    GuestAiocb untouched = Request(descriptor, 0, nullptr, 0);
    BeforeFirstRequest(descriptor, untouched);
    Transfers(descriptor);
    Rejections(descriptor, path);
    Waiting(descriptor);
    QueueLimit(descriptor);
    Signals(descriptor);
    const int directory = open_nid_postfix(root.string().c_str(), SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, 0);
    Require(directory >= 0);
    GuestAiocb flush = Request(directory, 0, nullptr, 0);
    Require(aio_fsync_nid_postfix(Sync, &flush) == 0 && aio_return_nid_postfix(&flush) == 0);
    Require(close_nid_postfix(directory) == 0 && close_nid_postfix(descriptor) == 0);
    GuestAiocb* completed = nullptr;
    const KernelTimespec poll{0, 0};
    Require(Fails(aio_waitcomplete_nid_postfix(&completed, &poll), Eagain));
    std::error_code error;
    RemoveTree(root, error);
    return 0;
}

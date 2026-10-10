#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>

#ifndef _WIN32
#include <signal.h>
#include <ucontext.h>
#endif

extern "C" int* APS5_VABI __error_nid_postfix();
extern "C" int APS5_VABI getpid_nid_postfix(void);

struct GuestSignalSet {
    std::uint32_t bits[4];
};

struct GuestSigaction {
    std::uintptr_t handler;
    int flags;
    GuestSignalSet mask;
};
static_assert(sizeof(GuestSigaction) == 32 && offsetof(GuestSigaction, flags) == 8 && offsetof(GuestSigaction, mask) == 12);

struct GuestMcontext {
    std::uint64_t onstack;
    std::uint64_t rdi;
    std::uint64_t rsi;
    std::uint64_t rdx;
    std::uint64_t rcx;
    std::uint64_t r8;
    std::uint64_t r9;
    std::uint64_t rax;
    std::uint64_t rbx;
    std::uint64_t rbp;
    std::uint64_t r10;
    std::uint64_t r11;
    std::uint64_t r12;
    std::uint64_t r13;
    std::uint64_t r14;
    std::uint64_t r15;
    std::uint32_t trapno;
    std::uint16_t fs;
    std::uint16_t gs;
    std::uint64_t addr;
    std::uint32_t flags;
    std::uint16_t es;
    std::uint16_t ds;
    std::uint64_t err;
    std::uint64_t rip;
    std::uint64_t cs;
    std::uint64_t rflags;
    std::uint64_t rsp;
    std::uint64_t ss;
    std::uint64_t len;
    std::uint64_t fpformat;
    std::uint64_t ownedfp;
    std::uint64_t lbrfrom;
    std::uint64_t lbrto;
    std::uint64_t aux1;
    std::uint64_t aux2;
    std::uint64_t fpstate[104];
    std::uint64_t fsbase;
    std::uint64_t gsbase;
    std::uint64_t spare[6];
};

struct GuestUcontext {
    std::uint32_t sigmask[4];
    std::int32_t reserved[12];
    GuestMcontext mcontext;
    GuestUcontext* link;
    void* stackPointer;
    std::uint64_t stackSize;
    std::int32_t stackFlags;
    std::int32_t stackAlign;
    std::int32_t flags;
    std::int32_t spare[4];
    std::int32_t tail[3];
};

static_assert(offsetof(GuestUcontext, mcontext) == 0x40);
static_assert(offsetof(GuestUcontext, mcontext) + offsetof(GuestMcontext, rsp) == 0xf8);

struct GuestSiginfo {
    int signo;
    int err_no;
    int code;
    int pid;
    unsigned int uid;
    int status;
    void* addr;
    void* value;
    union {
        struct {
            int _trapno;
        } _fault;
        struct {
            int _timerid;
            int _overrun;
        } _timer;
        struct {
            int _mqd;
        } _mesgq;
        struct {
            long _band;
        } _poll;
        struct {
            long __spare1__;
            int __spare2__[7];
        } __spare__;
    } _reason;
};
static_assert(sizeof(GuestSiginfo) == 80);
static_assert(offsetof(GuestSiginfo, addr) == 24);

namespace {
using GuestHandler = void (APS5_VABI *)(int);
using GuestSiginfoHandler = void (APS5_VABI *)(int, GuestSiginfo*, void*);

constexpr int MaxSignal = 128;
constexpr int GuestSigkill = 9;
constexpr int GuestSigstop = 17;

constexpr int SaOnstack = 0x1;
constexpr int SaRestart = 0x2;
constexpr int SaResethand = 0x4;
constexpr int SaNocldstop = 0x8;
constexpr int SaNodefer = 0x10;
constexpr int SaNocldwait = 0x20;
constexpr int SaSiginfo = 0x40;

std::atomic<std::uintptr_t> handlers[MaxSignal + 1]{};
static_assert(std::atomic<std::uintptr_t>::is_always_lock_free);

std::atomic<int> handlerFlags[MaxSignal + 1]{};
static_assert(std::atomic<int>::is_always_lock_free);

std::atomic<std::uint32_t> blockedMask{0};
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);

std::mutex registration;
GuestSigaction dispositions[MaxSignal + 1]{};

int NativeSignal(int guest) {
    switch (guest) {
        case 2: return SIGINT;
        case 4: return SIGILL;
        case 6: return SIGABRT;
        case 8: return SIGFPE;
        case 10: return SIGBUS;
        case 11: return SIGSEGV;
        case 13: return SIGPIPE;
        case 14: return SIGALRM;
        case 15: return SIGTERM;
#ifndef _WIN32
        case 30: return SIGUSR1;
        case 31: return SIGUSR2;
#endif
        default: return 0;
    }
}

int GuestSignal(int native) {
    switch (native) {
        case SIGINT: return 2;
        case SIGILL: return 4;
        case SIGABRT: return 6;
        case SIGFPE: return 8;
        case SIGBUS: return 10;
        case SIGSEGV: return 11;
        case SIGPIPE: return 13;
        case SIGALRM: return 14;
        case SIGTERM: return 15;
#ifndef _WIN32
        case SIGUSR1: return 30;
        case SIGUSR2: return 31;
#endif
        default: return 0;
    }
}

#ifndef _WIN32
void HostDispatch(int native, siginfo_t* info, void* ucontext_raw) {
    const int guest = GuestSignal(native);
    if (!guest || guest > MaxSignal) return;

    if ((blockedMask.load(std::memory_order_acquire) & (1u << (guest - 1))) != 0) return;

    const auto handler = handlers[guest].load(std::memory_order_acquire);
    if (handler <= 1) {
        if (handler == 1) return; // SIG_IGN
        struct sigaction sa{};
        sa.sa_handler = SIG_DFL;
        sigaction(native, &sa, nullptr);
        raise(native);
        return;
    }

    const int flags = handlerFlags[guest].load(std::memory_order_acquire);

    if ((flags & SaSiginfo) == 0) {
        auto callback = reinterpret_cast<GuestHandler>(handler);
        callback(guest);
    } else {
        auto callback = reinterpret_cast<GuestSiginfoHandler>(handler);

        alignas(16) GuestSiginfo ginfo{};
        ginfo.signo = guest;
        if (info != nullptr) {
            ginfo.err_no = info->si_errno;
            ginfo.code = info->si_code;
            ginfo.pid = info->si_pid;
            ginfo.uid = info->si_uid;
            ginfo.status = info->si_status;
            ginfo.addr = info->si_addr;
        }

        ucontext_t* luc = reinterpret_cast<ucontext_t*>(ucontext_raw);
        alignas(16) GuestUcontext gucontext{};
        auto& m = gucontext.mcontext;
        if (luc != nullptr) {
            m.r8  = luc->uc_mcontext.gregs[REG_R8];
            m.r9  = luc->uc_mcontext.gregs[REG_R9];
            m.r10 = luc->uc_mcontext.gregs[REG_R10];
            m.r11 = luc->uc_mcontext.gregs[REG_R11];
            m.r12 = luc->uc_mcontext.gregs[REG_R12];
            m.r13 = luc->uc_mcontext.gregs[REG_R13];
            m.r14 = luc->uc_mcontext.gregs[REG_R14];
            m.r15 = luc->uc_mcontext.gregs[REG_R15];
            m.rdi = luc->uc_mcontext.gregs[REG_RDI];
            m.rsi = luc->uc_mcontext.gregs[REG_RSI];
            m.rbp = luc->uc_mcontext.gregs[REG_RBP];
            m.rbx = luc->uc_mcontext.gregs[REG_RBX];
            m.rdx = luc->uc_mcontext.gregs[REG_RDX];
            m.rax = luc->uc_mcontext.gregs[REG_RAX];
            m.rcx = luc->uc_mcontext.gregs[REG_RCX];
            m.rsp = luc->uc_mcontext.gregs[REG_RSP];
            m.rip = luc->uc_mcontext.gregs[REG_RIP];
            m.rflags = luc->uc_mcontext.gregs[REG_EFL];
            m.cs  = luc->uc_mcontext.gregs[REG_CSGSFS] & 0xffff;
            m.trapno = luc->uc_mcontext.gregs[REG_TRAPNO];
            m.err = luc->uc_mcontext.gregs[REG_ERR];
            m.addr = reinterpret_cast<std::uint64_t>(info ? info->si_addr : nullptr);
            m.len = sizeof(GuestMcontext);

            if (luc->uc_mcontext.fpregs != nullptr) {
                std::memcpy(m.fpstate, luc->uc_mcontext.fpregs, std::min(sizeof(m.fpstate), sizeof(struct _fpstate)));
            }
        }

        callback(guest, &ginfo, &gucontext);

        if (luc != nullptr) {
            luc->uc_mcontext.gregs[REG_R8]  = m.r8;
            luc->uc_mcontext.gregs[REG_R9]  = m.r9;
            luc->uc_mcontext.gregs[REG_R10] = m.r10;
            luc->uc_mcontext.gregs[REG_R11] = m.r11;
            luc->uc_mcontext.gregs[REG_R12] = m.r12;
            luc->uc_mcontext.gregs[REG_R13] = m.r13;
            luc->uc_mcontext.gregs[REG_R14] = m.r14;
            luc->uc_mcontext.gregs[REG_R15] = m.r15;
            luc->uc_mcontext.gregs[REG_RDI] = m.rdi;
            luc->uc_mcontext.gregs[REG_RSI] = m.rsi;
            luc->uc_mcontext.gregs[REG_RBP] = m.rbp;
            luc->uc_mcontext.gregs[REG_RBX] = m.rbx;
            luc->uc_mcontext.gregs[REG_RDX] = m.rdx;
            luc->uc_mcontext.gregs[REG_RAX] = m.rax;
            luc->uc_mcontext.gregs[REG_RCX] = m.rcx;
            luc->uc_mcontext.gregs[REG_RSP] = m.rsp;
            luc->uc_mcontext.gregs[REG_RIP] = m.rip;
            luc->uc_mcontext.gregs[REG_EFL] = m.rflags;
        }
    }
}
#else
void Dispatch(int native) {
    int guest = 0;
    for (int candidate : {2, 4, 6, 8, 11, 15})
        if (NativeSignal(candidate) == native) { guest = candidate; break; }
    if (!guest) return;
    std::signal(native, Dispatch);
    if ((blockedMask.load() & (1u << (guest - 1))) != 0) return;
    const auto callback = reinterpret_cast<GuestHandler>(handlers[guest].load());
    if (reinterpret_cast<std::uintptr_t>(callback) > 1) callback(guest);
}
#endif

bool Delivered(const GuestSigaction& action) {
#ifndef _WIN32
    return action.handler > 1;
#else
    return action.handler > 1 && (action.flags & SaSiginfo) == 0;
#endif
}

bool Install(int guest, const GuestSigaction& action) {
    const int native = NativeSignal(guest);
    if (!native) return true;

    handlers[guest].store(action.handler, std::memory_order_release);
    handlerFlags[guest].store(action.flags, std::memory_order_release);

#ifndef _WIN32
    struct sigaction sa{};
    sigemptyset(&sa.sa_mask);

    if (action.handler == 0) {
        sa.sa_handler = SIG_DFL;
    } else if (action.handler == 1) {
        sa.sa_handler = SIG_IGN;
    } else {
        sa.sa_sigaction = HostDispatch;
        sa.sa_flags = SA_SIGINFO;
        if (action.flags & SaOnstack) sa.sa_flags |= SA_ONSTACK;
        if (action.flags & SaRestart) sa.sa_flags |= SA_RESTART;
        if (action.flags & SaResethand) sa.sa_flags |= SA_RESETHAND;
        if (action.flags & SaNocldstop) sa.sa_flags |= SA_NOCLDSTOP;
        if (action.flags & SaNodefer) sa.sa_flags |= SA_NODEFER;
        if (action.flags & SaNocldwait) sa.sa_flags |= SA_NOCLDWAIT;
    }

    if (sigaction(native, &sa, nullptr) != 0) {
        return false;
    }
    return true;
#else
    auto hostHandler = action.handler == 1 ? SIG_IGN : Delivered(action) ? Dispatch : SIG_DFL;
    if (std::signal(native, hostHandler) == SIG_ERR) {
        return false;
    }
    return true;
#endif
}
}

struct GuestStack {
    void* sp;
    std::size_t size;
    int flags;
};
static_assert(sizeof(GuestStack) == 24 && offsetof(GuestStack, flags) == 16);
namespace {
constexpr int SsDisable = 4;
constexpr std::size_t MinSignalStackSize = 2048;
thread_local GuestStack alternateStack{nullptr, 0, SsDisable};
}

extern "C" {
GuestHandler APS5_VABI signal_nid_postfix(int guest, GuestHandler handler) {
    const auto invalid = reinterpret_cast<GuestHandler>(static_cast<std::uintptr_t>(-1));
    const int native = NativeSignal(guest);
    if (!native || handler == invalid) { *__error_nid_postfix() = 22; return invalid; }
    std::lock_guard lock(registration);
    const GuestSigaction action{reinterpret_cast<std::uintptr_t>(handler), SaRestart, {}};
    if (!Install(guest, action)) { *__error_nid_postfix() = 22; return invalid; }
    const auto previous = reinterpret_cast<GuestHandler>(dispositions[guest].handler);
    dispositions[guest] = action;
    return previous;
}
int APS5_VABI sigaction_nid_postfix(int guest, const GuestSigaction* action, GuestSigaction* previous) {
    if (guest < 1 || guest > MaxSignal) { *__error_nid_postfix() = 22; return -1; }
    if (action && (guest == GuestSigkill || guest == GuestSigstop) && action->handler != 0) { *__error_nid_postfix() = 22; return -1; }
    std::lock_guard lock(registration);
    const GuestSigaction old = dispositions[guest];
    if (action) {
        if (!Install(guest, *action)) { *__error_nid_postfix() = 22; return -1; }
        dispositions[guest] = *action;
    }
    if (previous) *previous = old;
    return 0;
}
int APS5_VABI raise_nid_postfix(int guest) {
    const int native = NativeSignal(guest);
    if (!native) { *__error_nid_postfix() = 22; return -1; }
    {
        std::lock_guard lock(registration);
        if (dispositions[guest].handler > 1 && !Delivered(dispositions[guest])) throw std::runtime_error("raise: SA_SIGINFO handlers are not delivered");
    }
    const int result = std::raise(native);
    if (result) *__error_nid_postfix() = 22;
    return result ? -1 : 0;
}
int APS5_VABI kill_nid_postfix(int pid, int guest) {
    if (guest < 0 || guest > MaxSignal) { *__error_nid_postfix() = 22; return -1; }
    const int self = getpid_nid_postfix();
    if (pid != self && pid != 0 && pid != -self) { *__error_nid_postfix() = 3; return -1; }
    return guest == 0 ? 0 : raise_nid_postfix(guest);
}
int APS5_VABI sigaltstack_nid_postfix(const GuestStack* stack, GuestStack* previous) {
    GuestStack replacement = alternateStack;
    if (stack != nullptr) {
        if ((stack->flags & ~SsDisable) != 0) { *__error_nid_postfix() = 22; return -1; }
        if (stack->flags & SsDisable) replacement.flags = SsDisable;
        else if (stack->size < MinSignalStackSize) { *__error_nid_postfix() = 12; return -1; }
        else replacement = {stack->sp, stack->size, 0};
    }
    if (previous != nullptr) *previous = alternateStack;
    alternateStack = replacement;

#ifndef _WIN32
    if (stack != nullptr) {
        stack_t host_ss{};
        host_ss.ss_sp = stack->sp;
        host_ss.ss_size = stack->size;
        host_ss.ss_flags = (stack->flags & SsDisable) ? SS_DISABLE : 0;
        sigaltstack(&host_ss, nullptr);
    }
#endif
    return 0;
}
int APS5_VABI _sigprocmask_nid_postfix(int how, const GuestSignalSet* set, GuestSignalSet* previousSet) {
    std::lock_guard lock(registration);
    if (previousSet != nullptr) {
        previousSet->bits[0] = blockedMask.load();
        previousSet->bits[1] = 0;
        previousSet->bits[2] = 0;
        previousSet->bits[3] = 0;
    }
    if (set != nullptr) {
        switch (how) {
            case 1: blockedMask.fetch_or(set->bits[0]); break;
            case 2: blockedMask.fetch_and(~set->bits[0]); break;
            case 3: blockedMask.store(set->bits[0]); break;
            default: throw std::invalid_argument("_sigprocmask: invalid how");
        }
    }
    return 0;
}

int APS5_VABI sigprocmask_nid_postfix(int how, const void* set, void* previousSet) {
    return _sigprocmask_nid_postfix(how, static_cast<const GuestSignalSet*>(set),
                                    static_cast<GuestSignalSet*>(previousSet));
}
}

extern "C" {

int APS5_VABI _is_signal_return_nid_postfix(std::uint64_t programCounter) {
    (void)programCounter;
    return 0;
}

}

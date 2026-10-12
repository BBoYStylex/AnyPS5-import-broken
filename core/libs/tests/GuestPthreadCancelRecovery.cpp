#include "SceTypes.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#ifdef _WIN32
#include "prx/libkernel/Pthread/include/Pthread.hpp"
#include <chrono>
#include <thread>
#include <windows.h>
#include <wct.h>
#endif

extern "C" {
int APS5_VABI scePthreadCreate(Pthread* thread, const PthreadAttr* attr, PthreadEntry entry, void* arg, const char* name);
int APS5_VABI scePthreadJoin(Pthread thread, void** retval);
int APS5_VABI scePthreadCancel(Pthread thread);
Pthread APS5_VABI scePthreadSelf();
int APS5_VABI scePthreadSetcancelstate(int state, int* oldState);
int APS5_VABI scePthreadSetcanceltype(int type, int* oldType);
void APS5_VABI scePthreadTestcancel();
int APS5_VABI scePthreadKeyCreate(PthreadKey* key, pthread_key_destructor_func_t destructor);
int APS5_VABI scePthreadKeyDelete(PthreadKey key);
int APS5_VABI scePthreadSetspecific(PthreadKey key, void* value);
int APS5_VABI scePthreadMutexInit(PthreadMutex* mutex, const PthreadMutexattr* attr, const char* name);
int APS5_VABI scePthreadMutexDestroy(PthreadMutex* mutex);
int APS5_VABI scePthreadMutexLock(PthreadMutex* mutex);
int APS5_VABI scePthreadMutexUnlock(PthreadMutex* mutex);
int APS5_VABI scePthreadCondInit(PthreadCond* cond, const PthreadCondattr* attr, const char* name);
int APS5_VABI scePthreadCondDestroy(PthreadCond* cond);
int APS5_VABI scePthreadCondSignal(PthreadCond* cond);
int APS5_VABI scePthreadCondTimedwait(PthreadCond* cond, PthreadMutex* mutex, KernelUseconds usec);
int APS5_VABI scePthreadSemInit(PthreadSem* sem, int flag, unsigned int value, const char* name);
int APS5_VABI scePthreadSemDestroy(PthreadSem* sem);
int APS5_VABI scePthreadSemWait(PthreadSem* sem);
int APS5_VABI scePthreadSemTimedwait(PthreadSem* sem, KernelUseconds usec);
int APS5_VABI scePthreadSemGetvalue(PthreadSem* sem, int* value);
int APS5_VABI sem_init_nid_postfix(void* sem, int pshared, unsigned int value);
int APS5_VABI sem_destroy_nid_postfix(void* sem);
int APS5_VABI sem_wait_nid_postfix(void* sem);
int APS5_VABI sem_reltimedwait_np_nid_postfix(void* sem, std::uint32_t usec);
int APS5_VABI sem_getvalue_nid_postfix(void* sem, int* value);
int APS5_VABI sceKernelUsleep_nid_postfix(KernelUseconds microseconds);
void APS5_VABI __pthread_cleanup_push_imp_nid_postfix(void (APS5_VABI* routine)(void*), void* argument, void* info);
void APS5_VABI __pthread_cleanup_pop_imp_nid_postfix(int execute);
}

static constexpr int CANCEL_ENABLE = 0;
static constexpr int CANCEL_DISABLE = 1;
static constexpr int CANCEL_DEFERRED = 0;
static constexpr int KERNEL_EINVAL = static_cast<int>(0x80020016);
static constexpr KernelUseconds WAIT_MICROSECONDS = 60000000;
static void* const CANCELED = reinterpret_cast<void*>(std::uintptr_t{1});

static void Require(bool value, std::source_location location = std::source_location::current()) {
    if (value) return;
    std::fprintf(stderr, "%s:%u: requirement failed\n", location.file_name(), location.line());
    std::abort();
}

static PthreadSem sem = nullptr;
static void* posixSem = nullptr;
static PthreadMutex mutex = nullptr;
static PthreadCond cond = nullptr;
static PthreadKey key = -1;
static Pthread target = nullptr;
static std::atomic<bool> ready{false}, release{false}, targetRelease{false};
static std::atomic<int> events{0};
static std::array<int, 4> order{};
static bool usePosix = false, useTimed = false, disabled = false;
static bool conditionReleased = false, returned = false;
static int stage = 0, normalResult = 0, sentinel = 0;
static void* joinResult = &sentinel;

static void Signal(std::atomic<bool>& flag) {
    flag.store(true);
    flag.notify_all();
}

static void Gate() {
    Signal(ready);
    release.wait(false);
}

static Pthread Start(PthreadEntry entry) {
    ready = false;
    release = false;
    returned = false;
    events = 0;
    order = {};
    Pthread thread = nullptr;
    Require(scePthreadCreate(&thread, nullptr, entry, nullptr, nullptr) == 0);
    ready.wait(false);
    return thread;
}

static void Join(Pthread thread, void* expected) {
    void* result = &sentinel;
    Require(scePthreadJoin(thread, &result) == 0);
    Require(result == expected);
}

static void APS5_VABI Record(void* value) {
    const auto index = events.fetch_add(1);
    Require(index < static_cast<int>(order.size()));
    order[index] = static_cast<int>(reinterpret_cast<std::uintptr_t>(value));
}

static void APS5_VABI CleanupWithPoints(void*) {
    Record(reinterpret_cast<void*>(std::uintptr_t{2}));
    scePthreadTestcancel();
    Require(sceKernelUsleep_nid_postfix(1) == 0);
    Record(reinterpret_cast<void*>(std::uintptr_t{3}));
}

static void APS5_VABI DestroySpecific(void* value) {
    Require(value == &normalResult);
    Record(reinterpret_cast<void*>(std::uintptr_t{4}));
}

static void WaitForToken() {
    if (usePosix) {
        Require((useTimed ? sem_reltimedwait_np_nid_postfix(&posixSem, WAIT_MICROSECONDS)
                          : sem_wait_nid_postfix(&posixSem)) == 0);
    } else {
        Require((useTimed ? scePthreadSemTimedwait(&sem, WAIT_MICROSECONDS) : scePthreadSemWait(&sem)) == 0);
    }
}

static int TokenCount() {
    int count = -1;
    Require((usePosix ? sem_getvalue_nid_postfix(&posixSem, &count) : scePthreadSemGetvalue(&sem, &count)) == 0);
    return count;
}

static void* APS5_VABI PendingToken(void*) {
    std::uintptr_t first[8], second[8];
    Require(scePthreadSetspecific(key, &normalResult) == 0);
    __pthread_cleanup_push_imp_nid_postfix(Record, reinterpret_cast<void*>(std::uintptr_t{1}), first);
    __pthread_cleanup_push_imp_nid_postfix(CleanupWithPoints, nullptr, second);
    Gate();
    WaitForToken();
    returned = true;
    return nullptr;
}

static void CheckDefaults() {
    int old = -1;
    Require(scePthreadSetcancelstate(CANCEL_ENABLE, &old) == 0 && old == CANCEL_ENABLE);
    Require(scePthreadSetcanceltype(CANCEL_DEFERRED, &old) == 0 && old == CANCEL_DEFERRED);
    scePthreadTestcancel();
}

static void* APS5_VABI ConsumeToken(void*) {
    CheckDefaults();
    WaitForToken();
    return &normalResult;
}

static void PendingTokenRecovery() {
    Require((usePosix ? sem_init_nid_postfix(&posixSem, 0, 1) : scePthreadSemInit(&sem, 0, 1, nullptr)) == 0);
    const auto thread = Start(PendingToken);
    Require(scePthreadCancel(thread) == 0);
    Require(scePthreadCancel(thread) == 0);
    Require(events == 0 && TokenCount() == 1);
    Signal(release);
    Join(thread, CANCELED);
    Require(!returned && events == 4);
    Require((order == std::array<int, 4>{2, 3, 1, 4}));
    Require(TokenCount() == 1);
    Pthread consumer = nullptr;
    Require(scePthreadCreate(&consumer, nullptr, ConsumeToken, nullptr, nullptr) == 0);
    Join(consumer, &normalResult);
    Require(TokenCount() == 0);
    Require((usePosix ? sem_destroy_nid_postfix(&posixSem) : scePthreadSemDestroy(&sem)) == 0);
}

static void APS5_VABI Unlock(void*) {
    Require(scePthreadMutexUnlock(&mutex) == 0);
    Record(reinterpret_cast<void*>(std::uintptr_t{1}));
}

static void* APS5_VABI ConditionWaiter(void*) {
    CheckDefaults();
    if (disabled) Require(scePthreadSetcancelstate(CANCEL_DISABLE, nullptr) == 0);
    Require(scePthreadMutexLock(&mutex) == 0);
    std::uintptr_t info[8];
    __pthread_cleanup_push_imp_nid_postfix(Unlock, nullptr, info);
    Signal(ready);
    while (!conditionReleased) Require(scePthreadCondTimedwait(&cond, &mutex, WAIT_MICROSECONDS) == 0);
    Require(scePthreadMutexUnlock(&mutex) == 0);
    __pthread_cleanup_pop_imp_nid_postfix(0);
    if (disabled) {
        stage = 1;
        scePthreadTestcancel();
        int old = -1;
        Require(scePthreadSetcancelstate(CANCEL_ENABLE, &old) == 0 && old == CANCEL_DISABLE);
        stage = 2;
        scePthreadTestcancel();
    }
    returned = true;
    return &normalResult;
}

static void ConditionRecovery() {
    Require(scePthreadMutexInit(&mutex, nullptr, nullptr) == 0);
    Require(scePthreadCondInit(&cond, nullptr, nullptr) == 0);
    conditionReleased = false;
    stage = 0;
    const auto thread = Start(ConditionWaiter);
    Require(scePthreadMutexLock(&mutex) == 0);
    Require(scePthreadCancel(thread) == 0);
    Require(events == 0);
    if (disabled) {
        conditionReleased = true;
        Require(scePthreadCondSignal(&cond) == 0);
    }
    Require(scePthreadMutexUnlock(&mutex) == 0);
    Join(thread, CANCELED);
    Require(!returned);
    Require(disabled ? stage == 2 && events == 0 : events == 1 && order[0] == 1);
    disabled = false;
    conditionReleased = false;
    const auto next = Start(ConditionWaiter);
    Require(scePthreadMutexLock(&mutex) == 0);
    conditionReleased = true;
    Require(scePthreadCondSignal(&cond) == 0);
    Require(scePthreadMutexUnlock(&mutex) == 0);
    Join(next, &normalResult);
    Require(returned && events == 0);
    Require(scePthreadCondDestroy(&cond) == 0);
    Require(scePthreadMutexDestroy(&mutex) == 0);
}

static void* APS5_VABI HeldTarget(void*) {
    CheckDefaults();
    targetRelease.wait(false);
    return &normalResult;
}

static void* APS5_VABI PendingJoin(void*) {
    std::uintptr_t info[8];
    __pthread_cleanup_push_imp_nid_postfix(Record, reinterpret_cast<void*>(std::uintptr_t{1}), info);
    Gate();
    Require(scePthreadJoin(target, &joinResult) == 0);
    returned = true;
    return nullptr;
}

static void JoinRecovery() {
    targetRelease = false;
    joinResult = &sentinel;
    Require(scePthreadCreate(&target, nullptr, HeldTarget, nullptr, nullptr) == 0);
    const auto joiner = Start(PendingJoin);
    Require(scePthreadCancel(joiner) == 0);
    Signal(release);
    Join(joiner, CANCELED);
    Require(!returned && joinResult == &sentinel && events == 1 && order[0] == 1);
    Signal(targetRelease);
    Join(target, &normalResult);
    Require(scePthreadCreate(&target, nullptr, HeldTarget, nullptr, nullptr) == 0);
    Join(target, &normalResult);
}

#ifdef _WIN32

static DWORD nativeExitSlot = FLS_OUT_OF_INDEXES;

struct NativeExitGate {
    HANDLE entered = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE release = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::atomic<int> completed{0};

    NativeExitGate() { Require(entered != nullptr && release != nullptr); }
    ~NativeExitGate() { ::CloseHandle(entered); ::CloseHandle(release); }
};

static void WINAPI NativeExitDestructor(void* opaque) {
    auto& gate = *static_cast<NativeExitGate*>(opaque);
    Require(::FlsSetValue(nativeExitSlot, nullptr) != 0);
    Require(::SetEvent(gate.entered) != 0);
    Require(::WaitForSingleObject(gate.release, 10000) == WAIT_OBJECT_0);
    gate.completed.fetch_add(1);
}

static void* APS5_VABI NativeExitTarget(void* opaque) {
    Require(::FlsSetValue(nativeExitSlot, opaque) != 0);
    return &normalResult;
}

static Pthread StartNativeExitTarget(NativeExitGate& gate) {
    Pthread thread = nullptr;
    Require(scePthreadCreate(&thread, nullptr, NativeExitTarget, &gate, nullptr) == 0);
    Require(::WaitForSingleObject(gate.entered, 3000) == WAIT_OBJECT_0);
    Require(thread->_finished.load());
    return thread;
}

static bool NativeJoinWaitIsPending(Pthread thread, bool multiple = false) {
    const HWCT session = ::OpenThreadWaitChainSession(0, nullptr);
    Require(session != nullptr);
    const DWORD id = ::GetThreadId(static_cast<HANDLE>(thread->nativeHandle));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool waiting = false;
    while (!waiting && std::chrono::steady_clock::now() < deadline) {
        WAITCHAIN_NODE_INFO nodes[16]{};
        DWORD count = 16;
        BOOL cycle = FALSE;
        if (!::GetThreadWaitChain(session, 0, 0, id, &count, nodes, &cycle)) break;
        for (DWORD index = 0; index < count; ++index)
            if (nodes[index].ObjectType == WctThreadWaitType) waiting = true;
        if (!waiting && multiple && count == 1 && nodes[0].ObjectType == WctThreadType &&
                nodes[0].ObjectStatus == WctStatusBlocked) {
            std::lock_guard lock(thread->cancelLock);
            waiting = thread->wakeEvent != nullptr;
        }
        std::this_thread::yield();
    }
    ::CloseThreadWaitChainSession(session);
    return waiting;
}

struct NativeJoinState {
    Pthread target = nullptr;
    Pthread cleanupTarget = nullptr;
    void* result = &sentinel;
    void* cleanupResult = &sentinel;
    bool disabled = false;
    std::atomic<bool> entered{false};
    std::atomic<bool> returned{false};
    std::atomic<bool> cleanupEntered{false};
    std::atomic<int> cleanups{0};
};

static void APS5_VABI NativeJoinCleanup(void* opaque) {
    auto& state = *static_cast<NativeJoinState*>(opaque);
    state.cleanups.fetch_add(1);
    Signal(state.cleanupEntered);
    if (state.cleanupTarget)
        Require(scePthreadJoin(state.cleanupTarget, &state.cleanupResult) == 0);
}

static void* APS5_VABI NativeExitJoiner(void* opaque) {
    auto& state = *static_cast<NativeJoinState*>(opaque);
    CheckDefaults();
    if (state.disabled) Require(scePthreadSetcancelstate(CANCEL_DISABLE, nullptr) == 0);
    std::uintptr_t info[8];
    __pthread_cleanup_push_imp_nid_postfix(NativeJoinCleanup, &state, info);
    Signal(state.entered);
    Require(scePthreadJoin(state.target, &state.result) == 0);
    Signal(state.returned);
    __pthread_cleanup_pop_imp_nid_postfix(0);
    if (state.disabled) {
        Require(scePthreadSetcancelstate(CANCEL_ENABLE, nullptr) == 0);
        scePthreadTestcancel();
    }
    return &normalResult;
}

static void NativeJoinRecovery(bool cancel, bool disable) {
    NativeExitGate gate;
    NativeJoinState state;
    state.disabled = disable;
    state.target = StartNativeExitTarget(gate);
    Pthread joiner = nullptr;
    Require(scePthreadCreate(&joiner, nullptr, NativeExitJoiner, &state, nullptr) == 0);
    state.entered.wait(false);
    const bool enteredNativeWait = NativeJoinWaitIsPending(joiner, !disable);
    if (cancel) Require(scePthreadCancel(joiner) == 0);
    const bool canceledBeforeRelease = cancel && !disable &&
        ::WaitForSingleObject(static_cast<HANDLE>(joiner->nativeHandle), 2000) == WAIT_OBJECT_0;
    if (disable) Require(::WaitForSingleObject(static_cast<HANDLE>(joiner->nativeHandle), 0) == WAIT_TIMEOUT);
    Require(::SetEvent(gate.release) != 0);
    void* result = &sentinel;
    Require(scePthreadJoin(joiner, &result) == 0);
    const bool returnedNormally = state.returned.load();
    if (!returnedNormally) Join(state.target, &normalResult);
    const bool correct = enteredNativeWait && gate.completed.load() == 1 &&
        (cancel ? result == CANCELED : result == &normalResult) &&
        (cancel && !disable ? canceledBeforeRelease && !returnedNormally && state.result == &sentinel && state.cleanups == 1
                           : returnedNormally && state.result == &normalResult && state.cleanups == 0);
    if (!correct)
        std::fprintf(stderr, "Native join: cancel=%d disabled=%d entered=%d canceledBeforeRelease=%d returned=%d outputUntouched=%d\n",
            cancel, disable, enteredNativeWait, canceledBeforeRelease, returnedNormally, state.result == &sentinel);
    Require(correct);
}

static void NativeJoinCleanupRecovery() {
    NativeExitGate targetGate, cleanupGate;
    NativeJoinState state;
    state.target = StartNativeExitTarget(targetGate);
    state.cleanupTarget = StartNativeExitTarget(cleanupGate);
    Pthread joiner = nullptr;
    Require(scePthreadCreate(&joiner, nullptr, NativeExitJoiner, &state, nullptr) == 0);
    state.entered.wait(false);
    Require(NativeJoinWaitIsPending(joiner, true));
    Require(scePthreadCancel(joiner) == 0);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!state.cleanupEntered.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    const bool cleanupWait = state.cleanupEntered.load() && NativeJoinWaitIsPending(joiner);
    Require(::SetEvent(cleanupGate.release) != 0 && ::SetEvent(targetGate.release) != 0);
    Join(joiner, CANCELED);
    Join(state.target, &normalResult);
    Require(cleanupWait && state.cleanups == 1 && !state.returned && state.result == &sentinel &&
        state.cleanupResult == &normalResult && targetGate.completed == 1 && cleanupGate.completed == 1);
}

static void AdoptedNativeJoinRecovery() {
    NativeExitGate gate;
    const Pthread target = StartNativeExitTarget(gate);
    Pthread self = nullptr;
    HANDLE event = nullptr;
    std::atomic<bool> entered{false}, joined{false}, hostReturn{false};
    std::thread host([&] {
        self = scePthreadSelf();
        Signal(entered);
        Join(target, &normalResult);
        {
            std::lock_guard lock(self->cancelLock);
            event = static_cast<HANDLE>(self->wakeEvent);
        }
        Signal(joined);
        hostReturn.wait(false);
    });
    entered.wait(false);
    const bool waiting = NativeJoinWaitIsPending(self, true);
    Require(::SetEvent(gate.release) != 0);
    joined.wait(false);
    DWORD flags = 0;
    const bool eventWasValid = event != nullptr && ::GetHandleInformation(event, &flags) != 0;
    Signal(hostReturn);
    host.join();
    Require(::WaitForSingleObject(static_cast<HANDLE>(self->nativeHandle), 3000) == WAIT_OBJECT_0);
    bool cleared = false;
    {
        std::lock_guard lock(self->cancelLock);
        cleared = self->wakeEvent == nullptr;
    }
    ::SetLastError(0);
    const bool eventWasClosed = event != nullptr && ::GetHandleInformation(event, &flags) == 0 &&
        ::GetLastError() == ERROR_INVALID_HANDLE;
    if (!cleared || !eventWasClosed)
        std::fprintf(stderr, "Adopted native join: eventValidBeforeReturn=%d clearedAfterReturn=%d closedAfterReturn=%d\n",
            eventWasValid, cleared, eventWasClosed);
    Require(waiting && eventWasValid && cleared && eventWasClosed && gate.completed == 1);
}

struct AdoptedCancelState {
    NativeJoinState join;
    Pthread self = nullptr;
    std::atomic<bool> adopted{false};
};

static DWORD WINAPI AdoptedCancelableJoiner(void* opaque) {
    auto& state = *static_cast<AdoptedCancelState*>(opaque);
    state.self = scePthreadSelf();
    Signal(state.adopted);
    NativeExitJoiner(&state.join);
    return 0;
}

static void AdoptedNativeJoinCancellation() {
    NativeExitGate gate;
    AdoptedCancelState state;
    state.join.target = StartNativeExitTarget(gate);
    const HANDLE caller = ::CreateThread(nullptr, 0, AdoptedCancelableJoiner, &state, 0, nullptr);
    Require(caller != nullptr);
    state.adopted.wait(false);
    state.join.entered.wait(false);
    const bool waiting = NativeJoinWaitIsPending(state.self, true);
    HANDLE event = nullptr;
    {
        std::lock_guard lock(state.self->cancelLock);
        event = static_cast<HANDLE>(state.self->wakeEvent);
    }
    DWORD flags = 0;
    const bool eventWasValid = event != nullptr && ::GetHandleInformation(event, &flags) != 0;
    Require(scePthreadCancel(state.self) == 0);
    const bool canceledBeforeRelease = ::WaitForSingleObject(caller, 2000) == WAIT_OBJECT_0;
    Require(::SetEvent(gate.release) != 0);
    Require(::WaitForSingleObject(caller, 3000) == WAIT_OBJECT_0);
    const bool returnedNormally = state.join.returned.load();
    if (!returnedNormally) Join(state.join.target, &normalResult);
    ::SetLastError(0);
    const bool eventWasClosed = event != nullptr && ::GetHandleInformation(event, &flags) == 0 &&
        ::GetLastError() == ERROR_INVALID_HANDLE;
    Require(::CloseHandle(caller) != 0);
    const bool correct = waiting && eventWasValid && canceledBeforeRelease && !returnedNormally &&
        state.join.result == &sentinel && state.join.cleanups == 1 && eventWasClosed && gate.completed == 1;
    if (!correct)
        std::fprintf(stderr, "Adopted canceled native join: entered=%d eventValid=%d canceledBeforeRelease=%d returned=%d outputUntouched=%d cleanups=%d eventClosed=%d\n",
            waiting, eventWasValid, canceledBeforeRelease, returnedNormally, state.join.result == &sentinel,
            state.join.cleanups.load(), eventWasClosed);
    Require(correct);
}

static void NativeJoinRecoveryCases() {
    nativeExitSlot = ::FlsAlloc(NativeExitDestructor);
    Require(nativeExitSlot != FLS_OUT_OF_INDEXES);
    NativeJoinRecovery(false, false);
    NativeJoinRecovery(true, true);
    NativeJoinRecovery(true, false);
    NativeJoinCleanupRecovery();
    AdoptedNativeJoinRecovery();
    AdoptedNativeJoinCancellation();
    DWORD before = 0, after = 0;
    Require(::GetProcessHandleCount(::GetCurrentProcess(), &before) != 0);
    for (int round = 0; round < 16; ++round) {
        NativeJoinRecovery(false, false);
        NativeJoinRecovery(true, true);
        NativeJoinRecovery(true, false);
        NativeJoinCleanupRecovery();
        AdoptedNativeJoinCancellation();
    }
    Require(::GetProcessHandleCount(::GetCurrentProcess(), &after) != 0);
    if (after != before) std::fprintf(stderr, "Native join process handles: before=%lu after=%lu\n", before, after);
    Require(after == before);
    Require(::FlsFree(nativeExitSlot) != 0);
    nativeExitSlot = FLS_OUT_OF_INDEXES;
}

#endif

int main() {
    int old = 17;
    Require(scePthreadSetcancelstate(CANCEL_DISABLE, nullptr) == 0);
    Require(scePthreadSetcancelstate(7, &old) == KERNEL_EINVAL && old == 17);
    Require(scePthreadSetcancelstate(CANCEL_ENABLE, &old) == 0 && old == CANCEL_DISABLE);
    old = 17;
    Require(scePthreadSetcanceltype(1, &old) == KERNEL_EINVAL && old == 17);
    Require(scePthreadSetcanceltype(CANCEL_DEFERRED, &old) == 0 && old == CANCEL_DEFERRED);
    Require(scePthreadKeyCreate(&key, reinterpret_cast<pthread_key_destructor_func_t>(DestroySpecific)) == 0);
    for (bool posix : {false, true}) {
        usePosix = posix;
        for (bool timed : {false, true}) {
            useTimed = timed;
            PendingTokenRecovery();
        }
    }
    Require(scePthreadKeyDelete(key) == 0);
    ConditionRecovery();
    disabled = true;
    ConditionRecovery();
    JoinRecovery();
#ifdef _WIN32
    NativeJoinRecoveryCases();
#endif
}

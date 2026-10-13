#include "prx/libkernel/Semaphore/include/Semaphore.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include <chrono>

#include <stdexcept>
#include <string>
#include <utility>

KernelSemaPrivate::KernelSemaPrivate(std::int32_t initCount, std::int32_t maxCount, std::string name, bool isFifo)
 : name(std::move(name)), tokenCount(initCount), initCount(initCount), maxCount(maxCount), isFifo(isFifo) {
}

extern "C" {

int APS5_VABI sceKernelCreateSema(KernelSema* sem, const char* name, uint32_t attr, int init, int max, void* opt) {
 (void)opt;
 if (sem == nullptr || name == nullptr || attr > 2 || init < 0 || max <= 0 || init > max) {
  APS5_INVALID_ARG_EX;
 }

 *sem = new KernelSemaPrivate(init, max, std::string(name), attr == 1);
 return KERNEL_SEMA_OK;
}

int APS5_VABI sceKernelPollSema(KernelSema sem, int need) {
 if (sem == nullptr || need <= 0) {
  APS5_INVALID_ARG_EX;
 }

 std::lock_guard<std::mutex> lock(sem->mutex);
 if (sem->tokenCount < need) {
  return SCE_KERNEL_ERROR_EBUSY;
 }
 sem->tokenCount -= need;
 return KERNEL_SEMA_OK;
}

int APS5_VABI sceKernelSignalSema(KernelSema sem, int count) {
 if (sem == nullptr || count < 0) {
  APS5_INVALID_ARG_EX;
 }

 std::lock_guard<std::mutex> lock(sem->mutex);
 if (count > sem->maxCount - sem->tokenCount) {
  return SCE_KERNEL_ERROR_EINVAL;
 }
 sem->tokenCount += count;
 for (auto it = sem->queue.begin(); it != sem->queue.end();) {
  KernelSemaWaiter* waiter = *it;
  if (waiter->need > sem->tokenCount) {
   ++it;
   continue;
  }
  sem->tokenCount -= waiter->need;
  waiter->released = true;
  it = sem->queue.erase(it);
 }
 sem->condition.NotifyAll();
 return KERNEL_SEMA_OK;
}

int APS5_VABI sceKernelWaitSema(KernelSema sem, int need, KernelUseconds* time) {
 if (sem == nullptr || need <= 0) {
  APS5_INVALID_ARG_EX;
 }

 std::unique_lock<std::mutex> lock(sem->mutex);
 if (sem->tokenCount >= need) {
  sem->tokenCount -= need;
  return KERNEL_SEMA_OK;
 }
 const std::uint64_t generation = sem->cancelGeneration;
 KernelSemaWaiter waiter{need};
 sem->queue.push_back(&waiter);
 ++sem->waiterCount;
 struct WaiterGuard {
  KernelSemaPrivate* sem;
  KernelSemaWaiter* waiter;
  ~WaiterGuard() {
   --sem->waiterCount;
   if (!waiter->released) {
    sem->queue.remove(waiter);
   }
   sem->condition.NotifyAll();
  }
 } waiterGuard{sem, &waiter};

 const auto waitStart = std::chrono::steady_clock::now();
 const auto traceWait = [&](bool timedOut) {
  KernelTraceWait_nid_postfix("sema", __builtin_return_address(0), static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - waitStart).count()), timedOut);
 };
 const auto released = [&] { return waiter.released || sem->deleted || sem->cancelGeneration != generation; };
 bool acquired = true;
 if (time == nullptr) {
  sem->condition.Wait(lock, released);
 } else {
  const std::uint64_t deadline = TimedWait::DeadlineNanos(*time);
  acquired = sem->condition.WaitUntil(lock, deadline, released);
  *time = static_cast<KernelUseconds>(TimedWait::RemainingMicros(deadline));
 }
 traceWait(!acquired);
 if (waiter.released) {
  return KERNEL_SEMA_OK;
 }
 if (sem->deleted) {
  return SCE_KERNEL_ERROR_EACCES;
 }
 if (sem->cancelGeneration != generation) {
  return SCE_KERNEL_ERROR_ECANCELED;
 }
 return SCE_KERNEL_ERROR_ETIMEDOUT;
}

int APS5_VABI sceKernelCancelSema(KernelSema sem, int count, int* threads) {
 if (sem == nullptr) {
  APS5_INVALID_ARG_EX;
 }

 std::lock_guard<std::mutex> lock(sem->mutex);
 if (count > sem->maxCount) {
  return SCE_KERNEL_ERROR_EINVAL;
 }
 if (threads != nullptr) {
  *threads = static_cast<int>(sem->queue.size());
 }
 sem->tokenCount = count < 0 ? sem->initCount : count;
 sem->queue.clear();
 ++sem->cancelGeneration;
 sem->condition.NotifyAll();
 return KERNEL_SEMA_OK;
}

int APS5_VABI sceKernelDeleteSema(KernelSema sem) {
 if (sem == nullptr) {
  APS5_INVALID_ARG_EX;
 }

 std::unique_lock<std::mutex> lock(sem->mutex);
 sem->deleted = true;
 sem->condition.NotifyAll();
 sem->condition.Wait(lock, [&] { return sem->waiterCount == 0; });
 lock.unlock();
 delete sem;
 return KERNEL_SEMA_OK;
}

}

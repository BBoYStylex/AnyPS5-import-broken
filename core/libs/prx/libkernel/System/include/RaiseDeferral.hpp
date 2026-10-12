#ifndef CORE_LIBS_PRX_LIBKERNEL_SYSTEM_INCLUDE_RAISEDEFERRAL_HPP
#define CORE_LIBS_PRX_LIBKERNEL_SYSTEM_INCLUDE_RAISEDEFERRAL_HPP

#include <atomic>
#include <mutex>

namespace RaiseDeferral {

void BindThread(std::atomic<int>* locks, std::atomic<int>* pending);
void Enter();
void Leave();

class Mutex {
public:
    void lock() {
        Enter();
        mutex.lock();
    }

    void unlock() {
        mutex.unlock();
        Leave();
    }

private:
    std::mutex mutex;
};

}

#endif

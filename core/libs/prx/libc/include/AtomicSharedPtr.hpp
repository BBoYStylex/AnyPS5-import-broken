#ifndef PRX_LIBC_INCLUDE_ATOMICSHAREDPTR_HPP
#define PRX_LIBC_INCLUDE_ATOMICSHAREDPTR_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

// libstdc++ ships std::atomic<std::shared_ptr<T>> as an extension; libc++ does
// not, because the standard only guarantees the atomic_load/atomic_store free
// functions for shared_ptr. This reproduces the member API the code uses on
// top of a mutex, which gives the same guarantee: every reader sees a whole
// pointer, never a torn one.
#if defined(_LIBCPP_VERSION)

namespace ApcDetail {

template <typename T>
class AtomicSharedPtr {
public:
    AtomicSharedPtr() = default;
    AtomicSharedPtr(std::nullptr_t) noexcept {}

    std::shared_ptr<T> load(std::memory_order = std::memory_order_seq_cst) const {
        std::lock_guard lock(mutex);
        return pointer;
    }

    void store(std::shared_ptr<T> value, std::memory_order = std::memory_order_seq_cst) {
        std::lock_guard lock(mutex);
        pointer = std::move(value);
    }

    std::shared_ptr<T> exchange(std::shared_ptr<T> value, std::memory_order = std::memory_order_seq_cst) {
        std::lock_guard lock(mutex);
        std::shared_ptr<T> previous = std::move(pointer);
        pointer = std::move(value);
        return previous;
    }

    /// Replaces the pointer only when it still equals `expected`.
    bool compare_exchange_strong(
        std::shared_ptr<T>& expected,
        std::shared_ptr<T> desired,
        std::memory_order = std::memory_order_seq_cst) {
        std::lock_guard lock(mutex);
        if (pointer != expected) {
            expected = pointer;
            return false;
        }
        pointer = std::move(desired);
        return true;
    }

    operator std::shared_ptr<T>() const { return load(); }

private:
    mutable std::mutex mutex;
    std::shared_ptr<T> pointer;
};

}

template <typename T>
using AtomicSharedPtr = ApcDetail::AtomicSharedPtr<T>;

#else

template <typename T>
using AtomicSharedPtr = std::atomic<std::shared_ptr<T>>;

#endif

#endif
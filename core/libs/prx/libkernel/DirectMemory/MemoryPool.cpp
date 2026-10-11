
#include "prx/libkernel/DirectMemory/MemoryPool.hpp"
#include "prx/libkernel/DirectMemory/DirectMemory.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <mutex>
#include <new>
#include <utility>


#ifndef SCE_KERNEL_ERROR_EINVAL
#define SCE_KERNEL_ERROR_EINVAL static_cast<int>(0x80020016u)
#endif

#ifndef SCE_KERNEL_ERROR_EFAULT
#define SCE_KERNEL_ERROR_EFAULT static_cast<int>(0x8002000Eu)
#endif

#ifndef SCE_KERNEL_ERROR_EAGAIN
#define SCE_KERNEL_ERROR_EAGAIN static_cast<int>(0x80020023u)
#endif

#ifndef SCE_KERNEL_ERROR_ENOMEM
#define SCE_KERNEL_ERROR_ENOMEM static_cast<int>(0x8002000Cu)
#endif

namespace {

constexpr uint64_t kPage = static_cast<uint64_t>(PS5_PAGE_SIZE);
constexpr uint64_t kPoolBytes = static_cast<uint64_t>(DIRECT_MEMORY_SIZE);
constexpr size_t kNumPages = static_cast<size_t>(kPoolBytes / kPage);

static_assert(kPage != 0 && (kPage & (kPage - 1)) == 0, "PS5_PAGE_SIZE must be a power of two");
static_assert(kPoolBytes % kPage == 0, "DIRECT_MEMORY_SIZE must be a multiple of PS5_PAGE_SIZE");


bool AlignUp(uint64_t v, uint64_t a, uint64_t& out) {
    const uint64_t m = a - 1;
    if (v > UINT64_MAX - m) return false;
    out = (v + m) & ~m;
    return true;
}


bool NormalizeAlignment(size_t alignment, uint64_t& out) {
    const uint64_t a = (alignment == 0) ? kPage : static_cast<uint64_t>(alignment);
    if ((a & (a - 1)) != 0) return false;
    out = std::max<uint64_t>(a, kPage);
    return true;
}


bool NormalizeLength(size_t len, uint64_t& out) {
    if (len == 0) return false;
    uint64_t rounded;
    if (!AlignUp(static_cast<uint64_t>(len), kPage, rounded)) return false;
    if (rounded > kPoolBytes) return false;
    out = rounded;
    return true;
}

bool NormalizeSearchRange(int64_t searchStart, int64_t searchEnd, uint64_t& start, uint64_t& end) {
    if (searchStart < 0) return false;
    start = static_cast<uint64_t>(searchStart);
    end = (searchEnd < 0) ? kPoolBytes : std::min<uint64_t>(static_cast<uint64_t>(searchEnd), kPoolBytes);
    return start < end;
}


bool NormalizeRegion(uint64_t start, size_t len, uint64_t& alignedLen) {
    if (start >= kPoolBytes || (start & (kPage - 1)) != 0) return false;
    if (!NormalizeLength(len, alignedLen)) return false;
    return alignedLen <= kPoolBytes - start;
}

struct PhysicalMemoryPool {
    static PhysicalMemoryPool& Instance() {
        static PhysicalMemoryPool inst;
        return inst;
    }

    int Alloc(int64_t searchStart, int64_t searchEnd, size_t len, size_t alignment, int memoryType, int64_t* physOut) {
        if (!physOut) return SCE_KERNEL_ERROR_EFAULT;

        uint64_t start, end, align, alignedLen;
        if (!NormalizeSearchRange(searchStart, searchEnd, start, end)) return SCE_KERNEL_ERROR_EINVAL;
        if (!NormalizeAlignment(alignment, align)) return SCE_KERNEL_ERROR_EINVAL;
        if (!NormalizeLength(len, alignedLen)) return SCE_KERNEL_ERROR_EINVAL;

        uint64_t cur;
        if (!AlignUp(start, align, cur)) return SCE_KERNEL_ERROR_EINVAL;

        std::lock_guard<std::mutex> lock(_mutex);
        const size_t pageCount = static_cast<size_t>(alignedLen / kPage);

        while (cur <= end && alignedLen <= end - cur) {
            size_t lastUsed;
            if (!_lastUsedPage(static_cast<size_t>(cur / kPage), pageCount, lastUsed)) {
                const int rc = _commit(cur, alignedLen, memoryType);
                if (rc == 0) *physOut = static_cast<int64_t>(cur);
                return rc;
            }

            if (!AlignUp((static_cast<uint64_t>(lastUsed) + 1) * kPage, align, cur)) break;
        }
        return SCE_KERNEL_ERROR_EAGAIN;
    }


    bool Free(uint64_t start, size_t len) { return _release(start, len, /*requireAllocated=*/false); }

    bool CheckedFree(uint64_t start, size_t len) {
        try {
            return _release(start, len, /*requireAllocated=*/true);
        } catch (...) {
            return false;
        }
    }

    bool Find(uint64_t offset, bool findNext, int64_t* start, int64_t* end, int* memoryType) {
        if (!start || !end || !memoryType || offset >= kPoolBytes) return false;
        std::lock_guard<std::mutex> lock(_mutex);
        BlockMap::const_iterator it = _blocks.upper_bound(offset);
        if (it != _blocks.begin() && std::prev(it)->second.end > offset) --it;
        else if (!findNext || it == _blocks.end()) return false;

        *start = static_cast<int64_t>(it->first);
        *end = static_cast<int64_t>(it->second.end);
        *memoryType = it->second.type;
        return true;
    }

    bool Retype(uint64_t start, size_t len, int memoryType) {
        uint64_t alignedLen;
        if (!NormalizeRegion(start, len, alignedLen)) return false;
        const uint64_t end = start + alignedLen;

        std::lock_guard<std::mutex> lock(_mutex);
        if (!_anyUsed(static_cast<size_t>(start / kPage), static_cast<size_t>(alignedLen / kPage))) return false;

        const bool splitStart = _splitAt(start);
        try {
            _splitAt(end);
        } catch (...) {
            if (splitStart) _unsplitAt(start);
            throw;
        }
        for (BlockMap::iterator it = _blocks.lower_bound(start); it != _blocks.end() && it->first < end; ++it) {
            it->second.type = memoryType;
        }
        return true;
    }

    bool LargestFree(int64_t searchStart, int64_t searchEnd, size_t alignment, int64_t* startOut, size_t* sizeOut) {
        if (!startOut || !sizeOut) return false;
        uint64_t start, end, align;
        if (!NormalizeSearchRange(searchStart, searchEnd, start, end)) return false;
        if (!NormalizeAlignment(alignment, align)) return false;

        uint64_t cur;
        if (!AlignUp(start, kPage, cur)) return false;
        const uint64_t limit = end & ~(kPage - 1);

        std::lock_guard<std::mutex> lock(_mutex);
        uint64_t bestStart = 0;
        uint64_t bestSize = 0;

        while (cur < limit) {
            if (_used[cur / kPage]) {
                cur += kPage;
                continue;
            }
            uint64_t runEnd = cur;
            while (runEnd < limit && !_used[runEnd / kPage]) runEnd += kPage;

            uint64_t alignedStart;
            if (!AlignUp(cur, align, alignedStart)) break;
            if (alignedStart < runEnd && runEnd - alignedStart > bestSize) {
                bestStart = alignedStart;
                bestSize = runEnd - alignedStart;
            }
            cur = runEnd;
        }

        if (bestSize == 0) return false;
        *startOut = static_cast<int64_t>(bestStart);
        *sizeOut = static_cast<size_t>(bestSize);
        return true;
    }

    size_t FreeRun(uint64_t offset, uint64_t limit) {
        const uint64_t lim = std::min<uint64_t>(limit, kPoolBytes);
        if (offset >= lim) return 0;

        std::lock_guard<std::mutex> lock(_mutex);
        uint64_t cur = offset & ~(kPage - 1);
        size_t run = 0;
        while (cur <= lim && kPage <= lim - cur && !_used[cur / kPage]) {
            cur += kPage;
            run += static_cast<size_t>(kPage);
        }
        return run;
    }

private:
    struct Block {
        uint64_t end;
        int type;
    };
    typedef std::map<uint64_t, Block> BlockMap;

    bool _lastUsedPage(size_t first, size_t count, size_t& out) const {
        for (size_t i = count; i-- > 0;) {
            if (_used[first + i]) {
                out = first + i;
                return true;
            }
        }
        return false;
    }

    bool _anyUsed(size_t first, size_t count) const {
        for (size_t i = 0; i < count; ++i) {
            if (_used[first + i]) return true;
        }
        return false;
    }

    bool _allUsed(size_t first, size_t count) const {
        for (size_t i = 0; i < count; ++i) {
            if (!_used[first + i]) return false;
        }
        return true;
    }

    void _mark(size_t first, size_t count, bool used) {
        for (size_t i = 0; i < count; ++i) _used[first + i] = used;
    }

    int _commit(uint64_t start, uint64_t len, int type) {
        const uint64_t end = start + len;

        std::pair<BlockMap::iterator, bool> ins = _blocks.emplace(start, Block{end, type});
        if (!ins.second) return SCE_KERNEL_ERROR_EFAULT;

        try {
            CreateDirectMemoryBacking(static_cast<int64_t>(start), static_cast<size_t>(len), type);
        } catch (...) {
            _blocks.erase(ins.first);
            return SCE_KERNEL_ERROR_ENOMEM;
        }

        _mark(static_cast<size_t>(start / kPage), static_cast<size_t>(len / kPage), true);
        return 0;
    }

    bool _release(uint64_t start, size_t len, bool requireAllocated) {
        uint64_t alignedLen;
        if (!NormalizeRegion(start, len, alignedLen)) return false;
        const uint64_t end = start + alignedLen;
        const size_t first = static_cast<size_t>(start / kPage);
        const size_t count = static_cast<size_t>(alignedLen / kPage);

        std::lock_guard<std::mutex> lock(_mutex);
        if (requireAllocated ? !_allUsed(first, count) : !_anyUsed(first, count)) return false;


        const bool splitStart = _splitAt(start);
        bool splitEnd = false;
        try {
            splitEnd = _splitAt(end);
            ForgetDirectMemory(static_cast<int64_t>(start), static_cast<size_t>(alignedLen));
        } catch (...) {
            if (splitEnd) _unsplitAt(end);
            if (splitStart) _unsplitAt(start);
            throw;
        }

        _blocks.erase(_blocks.lower_bound(start), _blocks.lower_bound(end));
        _mark(first, count, false);
        return true;
    }


    bool _splitAt(uint64_t at) {
        BlockMap::iterator it = _blocks.upper_bound(at);
        if (it == _blocks.begin()) return false;
        --it;
        if (it->first == at || it->second.end <= at) return false;

        const Block tail = {it->second.end, it->second.type};
        _blocks.emplace(at, tail);
        it->second.end = at;
        return true;
    }

    void _unsplitAt(uint64_t at) noexcept {
        BlockMap::iterator right = _blocks.find(at);
        BlockMap::iterator left = std::prev(right);
        left->second.end = right->second.end;
        _blocks.erase(right);
    }

    std::mutex _mutex;
    bool _used[kNumPages] = {};
    BlockMap _blocks;
};

}
int DirectMemoryAlloc(int64_t searchStart, int64_t searchEnd, size_t len, size_t alignment, int memoryType, int64_t* physOut) {
    if (!physOut) return SCE_KERNEL_ERROR_EFAULT;
    try {
        return PhysicalMemoryPool::Instance().Alloc(searchStart, searchEnd, len, alignment, memoryType, physOut);
    } catch (const std::bad_alloc&) {
        return SCE_KERNEL_ERROR_ENOMEM;
    } catch (...) {
        return SCE_KERNEL_ERROR_EFAULT;
    }
}

void DirectMemoryFree(int64_t start, size_t len) {
    if (start < 0 || len == 0) return;
    // Exceptions intentionally propagate to surface physical backing cleanup failures.
    (void)PhysicalMemoryPool::Instance().Free(static_cast<uint64_t>(start), len);
}

bool DirectMemoryCheckedFree(int64_t start, size_t len) {
    if (start < 0 || len == 0) return false;
    try {
        return PhysicalMemoryPool::Instance().CheckedFree(static_cast<uint64_t>(start), len);
    } catch (...) {
        return false;
    }
}

bool DirectMemoryFind(int64_t offset, bool findNext, int64_t* start, int64_t* end, int* memoryType) {
    if (offset < 0 || !start || !end || !memoryType) return false;
    try {
        return PhysicalMemoryPool::Instance().Find(static_cast<uint64_t>(offset), findNext, start, end, memoryType);
    } catch (...) {
        return false;
    }
}

void DirectMemoryRetype(int64_t start, size_t len, int memoryType) {
    if (start < 0 || len == 0) return;
    // Exceptions intentionally propagate to surface map node allocation failures.
    (void)PhysicalMemoryPool::Instance().Retype(static_cast<uint64_t>(start), len, memoryType);
}

bool DirectMemoryLargestFree(int64_t searchStart, int64_t searchEnd, size_t alignment, int64_t* startOut, size_t* sizeOut) {
    if (!startOut || !sizeOut) return false;
    try {
        return PhysicalMemoryPool::Instance().LargestFree(searchStart, searchEnd, alignment, startOut, sizeOut);
    } catch (...) {
        return false;
    }
}

size_t DirectMemoryFreeRun(uint64_t offset, uint64_t limit) {
    if (offset >= kPoolBytes) return 0;
    try {
        return PhysicalMemoryPool::Instance().FreeRun(offset, limit);
    } catch (...) {
        return 0;
    }
}

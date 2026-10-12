#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_WINDOWSMAPPINGS_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_WINDOWSMAPPINGS_HPP

#ifdef _WIN32
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <stdexcept>
#include <string>
#include <system_error>

namespace GuestArena {

class WindowsMappings {
public:
    enum class Collection { All, Armed, Fresh };

    static WindowsMappings& Get() {
        static WindowsMappings mappings;
        return mappings;
    }

    void* Reserve(void* address, std::size_t bytes) {
        return allocate(GetCurrentProcess(), address, bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
    }

    std::vector<std::pair<std::uintptr_t, std::size_t>> Commit(void* address, std::size_t bytes, DWORD protection, std::size_t granule, bool watched) {
        std::vector<std::pair<std::uintptr_t, std::size_t>> created;
        std::lock_guard lock(mutex);
        const auto end = reinterpret_cast<std::uintptr_t>(address) + bytes;
        for (auto cursor = reinterpret_cast<std::uintptr_t>(address); cursor < end;) {
            const auto memory = query(cursor);
            const auto stop = std::min(end, reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize);
            if (memory.State == MEM_RESERVE) {
                const auto limit = std::min(end, cursor + granule);
                auto placeholderEnd = stop;
                while (placeholderEnd < limit) {
                    const auto next = query(placeholderEnd);
                    if (next.State != MEM_RESERVE) break;
                    placeholderEnd = reinterpret_cast<std::uintptr_t>(next.BaseAddress) + next.RegionSize;
                }
                const auto size = std::min(limit, placeholderEnd) - cursor;
                reset(cursor, size);
                const DWORD flags = MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER | (watched ? MEM_WRITE_WATCH : 0);
                if (!allocate(GetCurrentProcess(), reinterpret_cast<void*>(cursor), size, flags, protection, nullptr, 0)) fail("replace guest placeholder with private memory");
                if (!created.empty() && created.back().first + created.back().second == cursor) created.back().second += size;
                else created.emplace_back(cursor, size);
                cursor += size;
            } else {
                if (memory.State != MEM_COMMIT) throw std::runtime_error("guest memory is not committed");
                const auto mapped = views.find(cursor & ~(pageBytes - 1));
                if (mapped != views.end()) {
                    mapped->second.protection = protection;
                    mapped->second.armed = 0;
                    invalidate(*mapped->second.page);
                }
                if (!protect(cursor, stop - cursor, protection)) fail("protect guest memory");
                cursor = stop;
            }
        }
        return created;
    }

    void Reset(void* address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        reset(reinterpret_cast<std::uintptr_t>(address), bytes);
    }

    void Map(void* address, std::size_t bytes, HANDLE section, std::uint64_t offset, DWORD protection) {
        std::lock_guard lock(mutex);
        auto cursor = reinterpret_cast<std::uintptr_t>(address);
        reset(cursor, bytes);
        HANDLE duplicate = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), section, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) fail("keep shared guest section");
        const auto owned = std::make_shared<Section>(duplicate);
        for (std::size_t done = 0; done < bytes; done += pageBytes) {
            split(cursor + done, pageBytes);
            void* page = reinterpret_cast<void*>(cursor + done);
            if (!map(section, GetCurrentProcess(), page, offset + done, pageBytes, MEM_REPLACE_PLACEHOLDER, PAGE_EXECUTE_READWRITE, nullptr, 0)) fail("map shared guest page");
            MEMORY_BASIC_INFORMATION memory{};
            if (VirtualQuery(page, &memory, sizeof(memory)) != sizeof(memory)) fail("query shared guest page");
            if (memory.State == MEM_RESERVE && !VirtualAlloc(page, pageBytes, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) fail("commit shared guest page");
            DWORD previous;
            if (!VirtualProtect(page, pageBytes, protection, &previous)) fail("protect shared guest page");
            const auto key = std::make_pair(reinterpret_cast<std::uintptr_t>(section), offset + done);
            auto shared = physical[key].lock();
            if (!shared) {
                shared = std::make_shared<SharedPage>();
                physical[key] = shared;
            }
            const auto base = cursor + done;
            shared->aliases.push_back(base);
            views.emplace(base, View{shared, protection, {}, 0, owned, offset + done, 0});
            remember(freshRanges, base, base + pageBytes);
            invalidate(*shared);
        }
    }

    void SetProtection(std::uintptr_t address, std::size_t bytes, DWORD protection) {
        std::lock_guard lock(mutex);
        for (auto it = views.lower_bound(address); it != views.end() && it->first < address + bytes; ++it) {
            it->second.protection = protection;
            it->second.armed = 0;
            invalidate(*it->second.page);
        }
    }

    void Pin(std::uintptr_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        for (auto it = views.lower_bound(address & ~(pageBytes - 1)); it != views.end() && it->first < address + bytes; ++it) {
            auto& page = *it->second.page;
            ++page.pins;
            for (const auto alias : page.aliases) {
                auto& view = views.at(alias);
                if (view.armed == 0) continue;
                protectSubpages(alias, view.armed, view.protection, "pin shared guest page writable");
                view.armed = 0;
            }
            invalidate(page);
        }
    }

    void Unpin(std::uintptr_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        for (auto it = views.lower_bound(address & ~(pageBytes - 1)); it != views.end() && it->first < address + bytes; ++it) {
            auto& page = *it->second.page;
            if (page.pins != 0) --page.pins;
            invalidate(page);
        }
    }

    bool HandleWrite(std::uintptr_t address) {
        std::lock_guard lock(mutex);
        if (guarded(address)) return false;
        const auto base = address & ~(pageBytes - 1);
        const auto found = views.find(base);
        if (found == views.end() || !writable(found->second.protection)) return false;
        auto& view = found->second;
        const auto written = subpagesOf(base, address, address + 1);
        invalidate(*view.page, written);
        protectSubpages(base, written, view.protection, "resume shared memory write");
        view.armed &= static_cast<std::uint8_t>(~written);
        return true;
    }

    bool BeginHostWrite(std::uintptr_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        const auto first = views.lower_bound(address & ~(pageBytes - 1));
        const auto end = address + bytes;
        for (auto it = first; it != views.end() && it->first < end; ++it) {
            if (!writable(it->second.protection)) return false;
        }
        for (auto it = first; it != views.end() && it->first < end; ++it) {
            auto& view = it->second;
            ++view.hostWrites;
            const auto written = subpagesOf(it->first, address, end);
            invalidate(*view.page, written);
            const auto opened = static_cast<std::uint8_t>(view.armed & written);
            if (opened == 0) continue;
            protectSubpages(it->first, opened, view.protection, "open shared memory to a host write");
            view.armed &= static_cast<std::uint8_t>(~opened);
        }
        return true;
    }

    void EndHostWrite(std::uintptr_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        const auto end = address + bytes;
        for (auto it = views.lower_bound(address & ~(pageBytes - 1)); it != views.end() && it->first < end; ++it) {
            --it->second.hostWrites;
            invalidate(*it->second.page, subpagesOf(it->first, address, end));
        }
    }

    void* MapAlias(std::uintptr_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        const auto refuse = [&](const char* reason) {
            char text[192];
            std::snprintf(text, sizeof(text), "read-write alias of shared guest memory 0x%llx+0x%llx: %s", static_cast<unsigned long long>(address), static_cast<unsigned long long>(bytes), reason);
            return std::runtime_error(text);
        };
        if (address % pageBytes != 0 || bytes % pageBytes != 0 || bytes == 0) throw refuse("the range is not made of whole shared pages");
        auto view = views.find(address);
        if (view == views.end()) throw refuse("the range does not start at a shared view");
        const auto section = view->second.section;
        const auto offset = view->second.offset;
        SYSTEM_INFO system{};
        GetSystemInfo(&system);
        for (std::size_t done = 0; done < bytes; done += pageBytes, ++view) {
            if (view == views.end() || view->first != address + done || view->second.offset != offset + done) throw refuse("the range is not one contiguous run of views of a section");
            if (view->second.section != section && !sameSection(view->second.section->handle, section->handle)) throw refuse("the range spans several sections");
        }
        const auto lead = offset % system.dwAllocationGranularity;
        void* alias = map(section->handle, GetCurrentProcess(), nullptr, offset - lead, lead + bytes, 0, PAGE_READWRITE, nullptr, 0);
        if (alias == nullptr) {
            char text[160];
            std::snprintf(text, sizeof(text), "MapViewOfFile3 of a read-write alias of shared guest memory 0x%llx+0x%llx", static_cast<unsigned long long>(address), static_cast<unsigned long long>(bytes));
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), text);
        }
        return static_cast<char*>(alias) + lead;
    }

    void UnmapAlias(void* alias) {
        if (alias == nullptr) return;
        SYSTEM_INFO system{};
        GetSystemInfo(&system);
        const auto base = reinterpret_cast<std::uintptr_t>(alias) & ~(static_cast<std::uintptr_t>(system.dwAllocationGranularity) - 1);
        if (!unmap(GetCurrentProcess(), reinterpret_cast<void*>(base), 0)) fail("unmap shared guest alias");
    }

    std::uint64_t Guard(std::uintptr_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        if (bytes == 0 || ((address | bytes) & (subpageBytes - 1)) != 0) {
            char text[128];
            std::snprintf(text, sizeof(text), "guard page 0x%llx+0x%zx is not made of whole 4 KiB pages", static_cast<unsigned long long>(address), bytes);
            throw std::logic_error(text);
        }
        const auto end = address + bytes;
        const auto ticket = nextGuardTicket++;
        const auto overlap = firstGuard(address);
        if (overlap != guards.end() && overlap->first == address && overlap->second.end == end) {
            requireGuardIntact(address, bytes);
            overlap->second.owners.push_back(ticket);
            return ticket;
        }
        if (overlap != guards.end() && overlap->first < end) {
            char text[128];
            std::snprintf(text, sizeof(text), "guard page 0x%llx+0x%zx overlaps another guard page", static_cast<unsigned long long>(address), bytes);
            throw std::runtime_error(text);
        }
        DWORD previous;
        if (!VirtualProtect(reinterpret_cast<void*>(address), bytes, PAGE_NOACCESS, &previous)) fail("guard page");
        guards.emplace(address, Guarded{end, previous, {ticket}});
        forget(freshRanges, address, end);
        return ticket;
    }

    void Protect(std::uintptr_t address, std::size_t bytes, DWORD protection) {
        std::lock_guard lock(mutex);
        if (!protect(address, bytes, protection)) fail("protect guest memory");
    }

    void Decommit(std::uintptr_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        if (!VirtualFree(reinterpret_cast<void*>(address), bytes, MEM_DECOMMIT)) fail("VirtualFree decommit failed");
        dropGuards(address, address + bytes);
    }

    void Release(std::uintptr_t allocation) {
        std::lock_guard lock(mutex);
        auto end = allocation;
        for (auto memory = query(end); memory.State != MEM_FREE && reinterpret_cast<std::uintptr_t>(memory.AllocationBase) == allocation; memory = query(end)) {
            end = reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize;
        }
        if (!VirtualFree(reinterpret_cast<void*>(allocation), 0, MEM_RELEASE)) fail("VirtualFree release failed");
        dropGuards(allocation, end);
    }

    void Unguard(std::uintptr_t address, std::size_t bytes, std::uint64_t ticket) {
        std::lock_guard lock(mutex);
        if (const auto found = guards.find(address); found != guards.end()) {
            auto& owners = found->second.owners;
            if (const auto owner = std::find(owners.begin(), owners.end(), ticket); owner != owners.end()) {
                if (found->second.end != address + bytes) {
                    char text[128];
                    std::snprintf(text, sizeof(text), "guard page 0x%llx+0x%zx has another size", static_cast<unsigned long long>(address), bytes);
                    throw std::logic_error(text);
                }
                if (owners.size() == 1) requireGuardIntact(address, bytes);
                owners.erase(owner);
                if (!owners.empty()) return;
                DWORD previous;
                if (!VirtualProtect(reinterpret_cast<void*>(address), bytes, found->second.protection, &previous)) fail("remove guard page");
                guards.erase(found);
                refresh(address, address + bytes);
                return;
            }
        }
        if (droppedGuards.erase(ticket) != 0) return;
        char text[160];
        std::snprintf(text, sizeof(text), "guard page 0x%llx+0x%zx with ticket %llu is not registered", static_cast<unsigned long long>(address), bytes, static_cast<unsigned long long>(ticket));
        throw std::logic_error(text);
    }

    bool Protection(std::uintptr_t address, std::uint32_t* protection) {
        std::lock_guard lock(mutex);
        const auto found = views.find(address & ~(pageBytes - 1));
        if (found == views.end()) return false;
        *protection = found->second.protection;
        return true;
    }

    bool Collect(std::uintptr_t address, std::size_t bytes, void** pages, std::size_t* count, bool clear, Collection collection = Collection::All) {
        std::lock_guard lock(mutex);
        const auto capacity = *count;
        *count = 0;
        const auto end = address + bytes;
        if (collection == Collection::Fresh) {
            const auto fresh = freshRanges.upper_bound(address);
            if ((fresh == freshRanges.begin() || std::prev(fresh)->second <= address) && (fresh == freshRanges.end() || fresh->first >= end)) return true;
            for (auto it = views.lower_bound(address & ~(pageBytes - 1)); it != views.end() && it->first < end; ++it) {
                if (it->second.protection != PAGE_NOACCESS) continue;
                const auto guardedHere = guardedSubpages(it->first);
                for (auto at = std::max(address, it->first); at < std::min(end, it->first + pageBytes); at += 4096) {
                    const auto bit = 1u << ((at - it->first) / 4096);
                    if ((it->second.collected & bit) == 0 && (guardedHere & bit) == 0) return false;
                }
            }
        } else if (clear && !collectable(address, end)) return false;
        for (auto cursor = address; cursor < end;) {
            const auto nextClean = cleanRanges.upper_bound(cursor);
            if (nextClean != cleanRanges.begin()) {
                const auto clean = std::prev(nextClean);
                if (cursor < clean->second) {
                    cursor = std::min(end, clean->second);
                    continue;
                }
            }
            const auto base = cursor & ~(pageBytes - 1);
            const auto found = views.find(base);
            if (found != views.end()) {
                auto& view = found->second;
                auto stop = std::min(std::min(end, base + pageBytes), nextClean == cleanRanges.end() ? end : nextClean->first);
                const auto guardedHere = guardedSubpages(base);
                if (collection != Collection::All) {
                    const auto selected = [&](std::uintptr_t at) {
                        const auto bit = 1u << ((at - base) / 4096);
                        const bool collected = (view.collected & bit) != 0;
                        return collection == Collection::Armed ? collected : !collected && (guardedHere & bit) == 0;
                    };
                    while (cursor < stop && !selected(cursor)) cursor = std::min(stop, cursor + 4096);
                    if (cursor == stop) continue;
                    for (auto at = cursor + 4096; at < stop; at += 4096) {
                        if (!selected(at)) {
                            stop = at;
                            break;
                        }
                    }
                }
                if (view.protection == PAGE_NOACCESS) return false;
                const bool pinned = view.page->pins != 0;
                std::uint8_t settled = 0;
                auto settledEnd = cursor;
                bool full = false;
                for (std::size_t sub = 0; sub < subpages; ++sub) {
                    const auto from = std::max(cursor, base + sub * subpageBytes);
                    const auto to = std::min(stop, base + (sub + 1) * subpageBytes);
                    if (from >= to || (guardedHere & (1u << sub)) != 0) continue;
                    if (collection == Collection::Fresh || pinned || view.seen[sub] != view.page->generation[sub]) {
                        if (*count == capacity) {
                            full = true;
                            break;
                        }
                        pages[(*count)++] = reinterpret_cast<void*>(from);
                    }
                    settled |= static_cast<std::uint8_t>(1u << sub);
                    settledEnd = to;
                }
                if (clear) {
                    for (std::size_t sub = 0; sub < subpages; ++sub) {
                        const auto bit = static_cast<std::uint8_t>(1u << sub);
                        if ((settled & bit) == 0 || (view.collected & bit) != 0) continue;
                        view.collected |= bit;
                        forget(freshRanges, base + sub * subpageBytes, base + (sub + 1) * subpageBytes);
                    }
                }
                if (clear && !pinned && settled != 0) {
                    std::uint8_t untracked = 0;
                    for (const auto alias : view.page->aliases) {
                        auto& other = views.at(alias);
                        const auto arming = static_cast<std::uint8_t>(settled & ~other.armed);
                        if (!writable(other.protection) || arming == 0) continue;
                        if (other.hostWrites != 0) {
                            untracked |= arming;
                            continue;
                        }
                        const DWORD protection = other.protection == PAGE_EXECUTE_READWRITE ? PAGE_EXECUTE_READ : PAGE_READONLY;
                        protectSubpages(alias, arming, protection, "arm shared memory write tracking");
                        other.armed |= arming;
                    }
                    const auto tracked = static_cast<std::uint8_t>(settled & ~untracked);
                    auto cleanFrom = cursor;
                    auto cleanTo = cursor;
                    for (std::size_t sub = 0; sub < subpages; ++sub) {
                        if ((tracked & (1u << sub)) == 0) continue;
                        const auto from = std::max(cursor, base + sub * subpageBytes);
                        const auto to = std::min(settledEnd, base + (sub + 1) * subpageBytes);
                        if (from == base + sub * subpageBytes && to == base + (sub + 1) * subpageBytes) view.seen[sub] = view.page->generation[sub];
                        if (from != cleanTo) {
                            if (cleanFrom != cleanTo) rememberClean(cleanFrom, cleanTo);
                            cleanFrom = from;
                        }
                        cleanTo = to;
                    }
                    if (cleanFrom != cleanTo) rememberClean(cleanFrom, cleanTo);
                }
                if (full) return true;
                cursor = stop;
            } else {
                if (collection == Collection::Fresh) {
                    const auto next = views.lower_bound(cursor);
                    cursor = next == views.end() ? end : std::min(end, next->first);
                    continue;
                }
                const auto memory = query(cursor);
                if (memory.State != MEM_COMMIT || memory.Type != MEM_PRIVATE) return false;
                const auto stop = std::min(end, reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize);
                ULONG_PTR available = capacity - *count;
                if (available == 0) return true;
                DWORD granularity = 0;
                if (GetWriteWatch(clear ? WRITE_WATCH_FLAG_RESET : 0, reinterpret_cast<void*>(cursor), stop - cursor, pages + *count, &available, &granularity) != 0) fail("collect private guest writes");
                *count += available;
                if (*count == capacity) return true;
                cursor = stop;
            }
        }
        return true;
    }

private:
    bool collectable(std::uintptr_t address, std::uintptr_t end) {
        for (auto cursor = address; cursor < end;) {
            const auto base = cursor & ~(pageBytes - 1);
            if (const auto found = views.find(base); found != views.end()) {
                if (found->second.protection == PAGE_NOACCESS) return false;
                cursor = std::min(end, base + pageBytes);
                continue;
            }
            const auto memory = query(cursor);
            if (memory.State != MEM_COMMIT || memory.Type != MEM_PRIVATE) return false;
            cursor = std::min(end, reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize);
        }
        return true;
    }

    struct Guarded {
        std::uintptr_t end;
        DWORD protection;
        std::vector<std::uint64_t> owners;
    };

    std::map<std::uintptr_t, Guarded>::iterator firstGuard(std::uintptr_t address) {
        auto it = guards.upper_bound(address);
        if (it != guards.begin() && std::prev(it)->second.end > address) --it;
        return it;
    }

    static void requireGuardIntact(std::uintptr_t address, std::size_t bytes) {
        const auto memory = query(address);
        if (memory.State == MEM_COMMIT && memory.Protect == PAGE_NOACCESS && reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize >= address + bytes) return;
        char text[192];
        std::snprintf(text, sizeof(text), "guard page 0x%llx+0x%zx was changed outside the guard registry (state 0x%lx, protection 0x%lx)", static_cast<unsigned long long>(address), bytes, static_cast<unsigned long>(memory.State), static_cast<unsigned long>(memory.Protect));
        throw std::runtime_error(text);
    }

    void dropGuards(std::uintptr_t address, std::uintptr_t end) {
        for (auto it = firstGuard(address); it != guards.end() && it->first < end;) {
            droppedGuards.insert(it->second.owners.begin(), it->second.owners.end());
            const auto first = it->first;
            const auto last = it->second.end;
            it = guards.erase(it);
            refresh(first, last);
        }
    }

    void refresh(std::uintptr_t address, std::uintptr_t end) {
        for (auto it = views.lower_bound(address & ~(pageBytes - 1)); it != views.end() && it->first < end; ++it) {
            const auto guardedHere = guardedSubpages(it->first);
            for (std::size_t sub = 0; sub < subpages; ++sub) {
                const auto from = std::max(address, it->first + sub * subpageBytes);
                const auto to = std::min(end, it->first + (sub + 1) * subpageBytes);
                const auto bit = static_cast<std::uint8_t>(1u << sub);
                if (from < to && (it->second.collected & bit) == 0 && (guardedHere & bit) == 0) remember(freshRanges, from, to);
            }
        }
    }

    bool guarded(std::uintptr_t address) {
        const auto it = firstGuard(address);
        return it != guards.end() && it->first <= address;
    }

    bool protect(std::uintptr_t address, std::size_t bytes, DWORD protection) {
        const auto end = address + bytes;
        auto cursor = address;
        DWORD previous;
        for (auto it = firstGuard(address); it != guards.end() && it->first < end; ++it) {
            it->second.protection = protection;
            if (cursor < it->first && !VirtualProtect(reinterpret_cast<void*>(cursor), it->first - cursor, protection, &previous)) return false;
            cursor = std::max(cursor, it->second.end);
        }
        return cursor >= end || VirtualProtect(reinterpret_cast<void*>(cursor), end - cursor, protection, &previous);
    }

    static constexpr std::size_t pageBytes = 0x4000;
    static constexpr std::size_t subpageBytes = 0x1000;
    static_assert(subpageBytes == 4096, "Collect reports one page per subpage");
    static constexpr std::size_t subpages = pageBytes / subpageBytes;
    static constexpr std::uint8_t allSubpages = (1u << subpages) - 1;
    struct SharedPage {
        std::array<std::uint64_t, subpages> generation = [] {
            std::array<std::uint64_t, subpages> initial{};
            initial.fill(1);
            return initial;
        }();
        std::vector<std::uintptr_t> aliases;
        std::uint32_t pins = 0;
    };
    struct Section {
        HANDLE handle;
        explicit Section(HANDLE handle) : handle(handle) {}
        Section(const Section&) = delete;
        Section& operator=(const Section&) = delete;
        ~Section() { CloseHandle(handle); }
    };
    struct View {
        std::shared_ptr<SharedPage> page;
        DWORD protection;
        std::array<std::uint64_t, subpages> seen;
        std::uint8_t armed;
        std::shared_ptr<Section> section;
        std::uint64_t offset;
        std::uint32_t hostWrites;
        std::uint8_t collected = 0;
    };
    using Ranges = std::map<std::uintptr_t, std::uintptr_t>;

    static void forget(Ranges& ranges, std::uintptr_t start, std::uintptr_t end) {
        auto it = ranges.lower_bound(start);
        if (it != ranges.begin() && std::prev(it)->second > start) --it;
        while (it != ranges.end() && it->first < end) {
            const auto first = it->first;
            const auto last = it->second;
            it = ranges.erase(it);
            if (first < start) ranges.emplace(first, start);
            if (last > end) it = ranges.emplace(end, last).first;
        }
    }

    static void remember(Ranges& ranges, std::uintptr_t start, std::uintptr_t end) {
        auto it = ranges.lower_bound(start);
        if (it != ranges.begin() && std::prev(it)->second >= start) --it;
        while (it != ranges.end() && it->first <= end) {
            start = std::min(start, it->first);
            end = std::max(end, it->second);
            it = ranges.erase(it);
        }
        ranges.emplace(start, end);
    }

    void forgetClean(std::uintptr_t start, std::uintptr_t end) {
        forget(cleanRanges, start, end);
    }

    void rememberClean(std::uintptr_t start, std::uintptr_t end) {
        remember(cleanRanges, start, end);
    }

    void invalidate(SharedPage& page, std::uint8_t mask = allSubpages) {
        for (std::size_t sub = 0; sub < subpages; ++sub) {
            if ((mask & (1u << sub)) == 0) continue;
            ++page.generation[sub];
            for (const auto alias : page.aliases) forgetClean(alias + sub * subpageBytes, alias + (sub + 1) * subpageBytes);
        }
    }

    static std::uint8_t subpagesOf(std::uintptr_t base, std::uintptr_t address, std::uintptr_t end) {
        const auto from = std::max(address, base);
        const auto to = std::min(end, base + pageBytes);
        if (to <= from) return 0;
        const auto first = (from - base) / subpageBytes;
        const auto last = (to - 1 - base) / subpageBytes;
        return static_cast<std::uint8_t>(((1u << (last + 1)) - 1) & ~((1u << first) - 1));
    }

    void protectSubpages(std::uintptr_t base, std::uint8_t mask, DWORD protection, const char* operation) {
        for (std::size_t sub = 0; sub < subpages;) {
            if ((mask & (1u << sub)) == 0) {
                ++sub;
                continue;
            }
            auto stop = sub;
            while (stop < subpages && (mask & (1u << stop)) != 0) ++stop;
            if (!protect(base + sub * subpageBytes, (stop - sub) * subpageBytes, protection)) fail(operation);
            sub = stop;
        }
    }

    std::uint8_t guardedSubpages(std::uintptr_t base) {
        std::uint8_t mask = 0;
        for (auto it = firstGuard(base); it != guards.end() && it->first < base + pageBytes; ++it) mask |= subpagesOf(base, it->first, it->second.end);
        return mask;
    }

    bool sameSection(HANDLE first, HANDLE second) const {
        return compare != nullptr && compare(first, second);
    }

    static bool writable(DWORD protection) {
        return protection == PAGE_READWRITE || protection == PAGE_EXECUTE_READWRITE;
    }
    using AllocateFunction = PVOID (WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
    using MapFunction = PVOID (WINAPI*)(HANDLE, HANDLE, PVOID, ULONG64, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
    using UnmapFunction = BOOL (WINAPI*)(HANDLE, PVOID, ULONG);
    using CompareFunction = BOOL (WINAPI*)(HANDLE, HANDLE);

    WindowsMappings() {
        const auto module = GetModuleHandleW(L"KernelBase.dll");
        if (!module) fail("load Windows memory API");
        allocate = reinterpret_cast<AllocateFunction>(GetProcAddress(module, "VirtualAlloc2"));
        map = reinterpret_cast<MapFunction>(GetProcAddress(module, "MapViewOfFile3"));
        unmap = reinterpret_cast<UnmapFunction>(GetProcAddress(module, "UnmapViewOfFile2"));
        if (!allocate || !map || !unmap) throw std::runtime_error("Windows placeholder memory APIs are required");
        compare = reinterpret_cast<CompareFunction>(GetProcAddress(module, "CompareObjectHandles"));
    }

    [[noreturn]] static void fail(const char* operation) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
    }

    static MEMORY_BASIC_INFORMATION query(std::uintptr_t address) {
        MEMORY_BASIC_INFORMATION memory{};
        if (VirtualQuery(reinterpret_cast<void*>(address), &memory, sizeof(memory)) != sizeof(memory)) fail("query guest memory");
        return memory;
    }

    static void split(std::uintptr_t address, std::size_t bytes) {
        auto memory = query(address);
        memory = query(reinterpret_cast<std::uintptr_t>(memory.AllocationBase));
        if (memory.State != MEM_RESERVE) throw std::runtime_error("guest mapping requires a placeholder");
        const auto base = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        if (address != base) {
            if (!VirtualFree(reinterpret_cast<void*>(base), address - base, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) fail("split guest placeholder prefix");
            memory = query(address);
        }
        if (memory.RegionSize < bytes) throw std::runtime_error("guest placeholder is too small");
        if (memory.RegionSize != bytes && !VirtualFree(reinterpret_cast<void*>(address), bytes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) fail("split guest placeholder suffix");
    }

    void reset(std::uintptr_t address, std::size_t bytes) {
        const auto end = address + bytes;
        forgetClean(address, end);
        dropGuards(address, end);
        for (auto cursor = address; cursor < end;) {
            const auto memory = query(cursor);
            if (memory.State == MEM_RESERVE) {
                cursor = std::min(end, reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize);
                continue;
            }
            if (reinterpret_cast<std::uintptr_t>(memory.AllocationBase) != cursor) throw std::runtime_error("cannot release part of a host allocation");
            auto allocationEnd = cursor;
            do {
                const auto part = query(allocationEnd);
                if (part.AllocationBase != memory.AllocationBase) break;
                allocationEnd = reinterpret_cast<std::uintptr_t>(part.BaseAddress) + part.RegionSize;
            } while (allocationEnd < end);
            if (allocationEnd > end || query(allocationEnd).AllocationBase == memory.AllocationBase) throw std::runtime_error("guest release truncates a host allocation");
            if (memory.Type == MEM_MAPPED) {
                if (!unmap(GetCurrentProcess(), reinterpret_cast<void*>(cursor), MEM_PRESERVE_PLACEHOLDER)) fail("unmap shared guest page");
                const auto found = views.find(cursor);
                if (found != views.end()) {
                    std::erase(found->second.page->aliases, cursor);
                    views.erase(found);
                    forget(freshRanges, cursor, cursor + pageBytes);
                }
            } else if (memory.Type == MEM_PRIVATE) {
                if (!VirtualFree(reinterpret_cast<void*>(cursor), allocationEnd - cursor, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) fail("release private guest memory");
            } else {
                throw std::runtime_error("unsupported guest mapping type");
            }
            cursor = allocationEnd;
        }
        const auto last = query(reinterpret_cast<std::uintptr_t>(query(end - 1).AllocationBase));
        const auto lastBase = reinterpret_cast<std::uintptr_t>(last.BaseAddress);
        if (lastBase + last.RegionSize > end) split(lastBase, end - lastBase);
        const auto first = query(address);
        split(address, std::min(bytes, reinterpret_cast<std::uintptr_t>(first.BaseAddress) + first.RegionSize - address));
        if (query(address).RegionSize != bytes && !VirtualFree(reinterpret_cast<void*>(address), bytes, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS)) fail("coalesce guest placeholders");
    }

    Ranges cleanRanges;
    Ranges freshRanges;
    std::map<std::uintptr_t, Guarded> guards;
    std::set<std::uint64_t> droppedGuards;
    std::uint64_t nextGuardTicket = 1;
    std::map<std::uintptr_t, View> views;
    std::map<std::pair<std::uintptr_t, std::uint64_t>, std::weak_ptr<SharedPage>> physical;
    std::mutex mutex;
    AllocateFunction allocate = nullptr;
    MapFunction map = nullptr;
    UnmapFunction unmap = nullptr;
    CompareFunction compare = nullptr;
};

}
#endif

#endif

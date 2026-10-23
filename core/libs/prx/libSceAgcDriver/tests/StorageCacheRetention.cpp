#include "prx/libSceAgcDriver/Graphics/include/ShaderResources.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <list>
#include <memory>
#include <vector>

using namespace AgcDriver::Graphics;

namespace {

int failures = 0;

void Expect(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

struct MockStorageTexture {
    int id = 0;
};

struct MockEntry {
    std::shared_ptr<MockStorageTexture> texture;
    std::uint64_t accounted = 0;
};

void testHeldEntrySurvivesUnderCeiling() {
    Expect(KeepStorageOnBudgetEvict(100, 200, 40, 2), "a held entry under the ceiling survives eviction");
    Expect(KeepStorageOnBudgetEvict(200, 200, 40, 2), "a held entry exactly at the ceiling survives eviction");
}

void testUnheldEntryIsEvictableUnderCeiling() {
    Expect(!KeepStorageOnBudgetEvict(100, 200, 40, 1), "an unheld entry under the ceiling is evictable");
    Expect(!KeepStorageOnBudgetEvict(199, 200, 40, 1), "an unheld entry just under the ceiling is evictable");
}

void testEverythingIsEvictableOverCeiling() {
    Expect(!KeepStorageOnBudgetEvict(201, 200, 40, 2), "a held entry over the ceiling is evictable");
    Expect(!KeepStorageOnBudgetEvict(500, 200, 40, 3), "a held entry far over the ceiling is evictable");
    Expect(!KeepStorageOnBudgetEvict(201, 200, 40, 1), "an unheld entry over the ceiling is evictable");
}

void testUnaccountedEntryAlwaysSurvives() {
    Expect(KeepStorageOnBudgetEvict(100, 200, 0, 1), "an unaccounted entry under the ceiling survives");
    Expect(KeepStorageOnBudgetEvict(500, 200, 0, 1), "an unaccounted entry over the ceiling survives");
    Expect(KeepStorageOnBudgetEvict(500, 200, 0, 0), "an unaccounted unheld entry over the ceiling survives");
}

void testLoopHeldEntrySurvivesUnderCeiling() {
    std::list<MockEntry> entries;
    auto hold = std::make_shared<MockStorageTexture>(MockStorageTexture{1});
    entries.push_back({hold, 40});
    entries.push_back({std::make_shared<MockStorageTexture>(MockStorageTexture{2}), 40});
    std::uint64_t cacheBytes = 80;
    std::vector<MockEntry> evicted;
    EvictStorageOnBudget(entries, cacheBytes, 40, 100, 200, [&](auto it) {
        cacheBytes -= it->accounted;
        evicted.push_back(*it);
        entries.erase(it);
    });
    Expect(entries.size() == 1, "held entry remains in cache");
    Expect(!entries.empty() && entries.front().texture->id == 1, "held entry is the surviving entry");
    Expect(evicted.size() == 1, "idle entry was evicted");
    Expect(!evicted.empty() && evicted[0].texture->id == 2, "idle entry is the evicted entry");
    Expect(cacheBytes == 40, "cache bytes reflects eviction");
}

void testLoopAllHeldEntriesRetainedUpToCeiling() {
    std::list<MockEntry> entries;
    auto hold1 = std::make_shared<MockStorageTexture>(MockStorageTexture{1});
    auto hold2 = std::make_shared<MockStorageTexture>(MockStorageTexture{2});
    entries.push_back({hold1, 50});
    entries.push_back({hold2, 50});
    std::uint64_t cacheBytes = 100;
    std::vector<MockEntry> evicted;
    EvictStorageOnBudget(entries, cacheBytes, 20, 100, 200, [&](auto it) {
        cacheBytes -= it->accounted;
        evicted.push_back(*it);
        entries.erase(it);
    });
    Expect(entries.size() == 2, "all held entries survive under ceiling");
    Expect(evicted.empty(), "no entries evicted when all are held under ceiling");
    Expect(cacheBytes == 100, "cache bytes unchanged when held entries retained");
    Expect(entries.front().texture->id == 1, "original front entry remains at front");
    Expect(entries.back().texture->id == 2, "original back entry remains at back");
}

void testLoopIdleEntryEvictedWhileHeldSurvives() {
    std::list<MockEntry> entries;
    entries.push_back({std::make_shared<MockStorageTexture>(MockStorageTexture{1}), 30});
    auto hold2 = std::make_shared<MockStorageTexture>(MockStorageTexture{2});
    entries.push_back({hold2, 40});
    std::uint64_t cacheBytes = 70;
    std::vector<MockEntry> evicted;
    EvictStorageOnBudget(entries, cacheBytes, 40, 60, 150, [&](auto it) {
        cacheBytes -= it->accounted;
        evicted.push_back(*it);
        entries.erase(it);
    });
    Expect(entries.size() == 1, "only held entry survives");
    Expect(!entries.empty() && entries.front().texture->id == 2, "held entry is spliced to front and kept");
    Expect(evicted.size() == 1, "idle entry was evicted");
    Expect(!evicted.empty() && evicted[0].texture->id == 1, "idle entry 1 was evicted");
    Expect(cacheBytes == 40, "cache bytes updated after idle eviction");
}

void testLoopOverCeilingEvictsHeldEntries() {
    std::list<MockEntry> entries;
    auto hold1 = std::make_shared<MockStorageTexture>(MockStorageTexture{1});
    auto hold2 = std::make_shared<MockStorageTexture>(MockStorageTexture{2});
    entries.push_back({hold1, 50});
    entries.push_back({hold2, 50});
    std::uint64_t cacheBytes = 100;
    std::vector<MockEntry> evicted;
    EvictStorageOnBudget(entries, cacheBytes, 100, 80, 120, [&](auto it) {
        cacheBytes -= it->accounted;
        evicted.push_back(*it);
        entries.erase(it);
    });
    Expect(entries.empty(), "all entries evicted when over ceiling");
    Expect(evicted.size() == 2, "both held entries were evicted over ceiling");
    Expect(cacheBytes == 0, "cache bytes cleared");
}

void testLoopUnaccountedEntrySurvives() {
    std::list<MockEntry> entries;
    entries.push_back({std::make_shared<MockStorageTexture>(MockStorageTexture{1}), 0});
    entries.push_back({std::make_shared<MockStorageTexture>(MockStorageTexture{2}), 50});
    std::uint64_t cacheBytes = 50;
    std::vector<MockEntry> evicted;
    EvictStorageOnBudget(entries, cacheBytes, 60, 50, 100, [&](auto it) {
        cacheBytes -= it->accounted;
        evicted.push_back(*it);
        entries.erase(it);
    });
    Expect(entries.size() == 1, "unaccounted entry survives");
    Expect(!entries.empty() && entries.front().texture->id == 1, "unaccounted entry remains in cache");
    Expect(evicted.size() == 1, "accounted idle entry evicted");
    Expect(!evicted.empty() && evicted[0].texture->id == 2, "entry 2 was evicted");
    Expect(cacheBytes == 0, "cache bytes zeroed after accounted entry evicted");
}

}

int main() {
    testHeldEntrySurvivesUnderCeiling();
    testUnheldEntryIsEvictableUnderCeiling();
    testEverythingIsEvictableOverCeiling();
    testUnaccountedEntryAlwaysSurvives();
    testLoopHeldEntrySurvivesUnderCeiling();
    testLoopAllHeldEntriesRetainedUpToCeiling();
    testLoopIdleEntryEvictedWhileHeldSurvives();
    testLoopOverCeilingEvictsHeldEntries();
    testLoopUnaccountedEntrySurvives();
    return failures == 0 ? 0 : 1;
}

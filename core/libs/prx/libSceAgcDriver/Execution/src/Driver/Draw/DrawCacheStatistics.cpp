#include "prx/libSceAgcDriver/Execution/include/ShaderMemory.hpp"
#include "prx/libSceAgcDriver/ShaderRecompiler/include/Recompiler.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"

namespace AgcDriver::DriverDetail {

void Driver::reportDrawCache(DrawEntryCounters& counters) {
    const auto count = [](std::uint64_t value) { return static_cast<unsigned long long>(value); };
    const auto validated = counters.lookups - counters.absent;
    const auto miss = [&](DrawMiss reason) { return count(counters.misses[static_cast<std::size_t>(reason)]); };
    std::string ranks;
    for (std::size_t rank = 0; rank < dispatchVariants(); ++rank) {
        char text[32];
        std::snprintf(text, sizeof(text), "%s%llu", rank == 0 ? "" : " / ", count(counters.variantHitsByRank[rank]));
        ranks += text;
    }
    AgcDriver::ProfilePrint_nid_no_patch( "[draw-cache] %llu lookups (10 s): %llu no entry, %llu validated in %.1f us each (key, lookup and every stage): %llu hits (every stage equal; stage hits by variant rank 1..k %s), misses by reason: front stage differing %llu, fragment differing %llu, other stage differing %llu, layout (push offset) %llu, gate %llu, stage count %llu; %llu stage validations (%llu equal, %.2f variants compared each); %llu inserts (%llu variants inserted, %llu evicted beyond k, %llu already present, %llu unstable), %zu entries (%llu variants, ~%.1f MiB), %llu evictions in total, %llu LRU moves; verify: %llu hits captured again, %llu stages differed\n", count(counters.lookups), count(counters.absent), count(validated), validated != 0 ? counters.validateUs / static_cast<double>(validated) : 0.0, count(counters.hits), ranks.c_str(), miss(DrawMiss::FrontDiffering), miss(DrawMiss::FragmentDiffering), miss(DrawMiss::OtherDiffering), miss(DrawMiss::Layout), miss(DrawMiss::Gate), miss(DrawMiss::Stages), count(counters.stageValidations), count(counters.stageEqual), counters.stageValidations != 0 ? static_cast<double>(counters.variantsCompared) / static_cast<double>(counters.stageValidations) : 0.0, count(counters.inserts), count(counters.variantsInserted), count(counters.variantsEvicted), count(counters.present), count(counters.unstable), drawCache.size(), count(drawCacheVariants), static_cast<double>(drawCacheVariantBytes) / (1024.0 * 1024.0), count(drawCacheEvictions), count(counters.touches), count(counters.verifyHits), count(counters.verifyMismatches));
    AgcDriver::ProfilePrint_nid_no_patch( "[draw-cache] register key (10 s): %llu lookups, %llu hits, key build %.1f us each; decode skipped %llu, partial (state/pixel/programs from the entry) %llu; facade log != table %llu; verify: %llu decodes compared, %llu differed\n", count(counters.registerKeyLookups), count(counters.registerKeyHits), counters.registerKeyLookups != 0 ? counters.keyUs / static_cast<double>(counters.registerKeyLookups) : 0.0, count(counters.decodeSkipped), count(counters.decodePartial), count(counters.facadeMismatches), count(counters.verifyDecodes), count(counters.verifyDecodeMismatches));
    // Candidate 1: Capture/Materialize memo
    auto& captureTotals = AgcDriver::ShaderMemory::CaptureTotals();
    const auto captureHandleHits = captureTotals.handleHits.exchange(0, std::memory_order_relaxed);
    const auto captureHandleMisses = captureTotals.handleMisses.exchange(0, std::memory_order_relaxed);
    AgcDriver::ProfilePrint_nid_no_patch("[capture-memo] %llu calls (10 s): %llu handle hits, %llu handle misses (%.1f%% hits)\n", count(captureHandleHits + captureHandleMisses), count(captureHandleHits), count(captureHandleMisses), captureHandleHits + captureHandleMisses != 0 ? 100.0 * static_cast<double>(captureHandleHits) / static_cast<double>(captureHandleHits + captureHandleMisses) : 0.0);

    auto& resultMemoCounters = AgcDriver::ShaderRecompiler::resultMemoCounters();
    const auto resultMemoHits = resultMemoCounters.hits.exchange(0, std::memory_order_relaxed);
    const auto resultMemoMisses = resultMemoCounters.misses.exchange(0, std::memory_order_relaxed);
    const auto resultMemoEvictions = resultMemoCounters.evictions.exchange(0, std::memory_order_relaxed);
    const auto resultMemoPopulate = resultMemoCounters.populateNanoseconds.exchange(0, std::memory_order_relaxed);
    AgcDriver::ProfilePrint_nid_no_patch("[recompile-memo] %llu calls (10 s): %llu hits, %llu misses (%.1f%% hits), Populate %.1f us per miss / %.1f ms in total, %llu evictions\n", count(resultMemoHits + resultMemoMisses), count(resultMemoHits), count(resultMemoMisses), resultMemoHits + resultMemoMisses != 0 ? 100.0 * static_cast<double>(resultMemoHits) / static_cast<double>(resultMemoHits + resultMemoMisses) : 0.0, resultMemoMisses != 0 ? static_cast<double>(resultMemoPopulate) / 1000.0 / static_cast<double>(resultMemoMisses) : 0.0, static_cast<double>(resultMemoPopulate) / 1e6, count(resultMemoEvictions));
}

DrawEntryCounters Driver::DrawCacheCounters() {
    std::lock_guard cacheLock(drawCacheMutex);
    return drawEntryCounters;
}

}

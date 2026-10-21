#include "prx/libSceAgcDriver/Execution/include/ProfileOutput.hpp"
#include "ThreadOwned.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Synchronization/DeferredLabels.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/PipelineLookahead.hpp"
#include "prx/libSceAgcDriver/Execution/include/Pm4.hpp"
#include "prx/libSceAgcDriver/Execution/include/VideoOutput.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace AgcDriver::DriverDetail {

std::map<std::size_t, Driver::ResolvedDispatch>& Driver::resolvedAhead() {
    static thread_local std::map<std::size_t, ResolvedDispatch>* resolved = nullptr;
    return ShaderRecompiler::ThreadOwned(resolved);
}

Driver::GroupCaptureStats& Driver::groupCaptureStats() {
    static thread_local GroupCaptureStats stats;
    return stats;
}

std::size_t& Driver::currentPacketOffset() {
    static thread_local std::size_t offset = 0;
    return offset;
}

bool& Driver::resolvingAhead() {
    static thread_local bool resolving = false;
    return resolving;
}

bool& Driver::resolvingForPipelines() {
    static thread_local bool pipelinesOnly = false;
    return pipelinesOnly;
}

std::size_t& Driver::resolvedUpTo() {
    static thread_local std::size_t reached = 0;
    return reached;
}

bool Driver::submitLookahead() {
    static const bool enabled = Graphics::SubmitLookaheadEnabled();
    return enabled;
}

bool& Driver::gateOpened() {
    static thread_local bool opened = false;
    return opened;
}

std::uint64_t Driver::DispatchesAdoptedAhead() const {
    return dispatchesAdoptedAhead.load(std::memory_order_relaxed);
}

void Driver::clearResolvedAhead() {
    auto& resolved = resolvedAhead();
    if (resolved.empty()) return;
    groupCaptureStats().stale += resolved.size();
    resolved.clear();
}

namespace {

struct RunLabel {
    std::uint64_t address;
    std::size_t size;
    std::uint64_t value;
};

bool InCode(const ShaderRecompiler::MemoryRegion& region, const std::shared_ptr<const ShaderSnapshot>& code) {
    if (code == nullptr) return false;
    if (region.guestAddress >= code->codeAddress && region.guestAddress < code->codeAddress + code->code.size() * sizeof(std::uint32_t)) return true;
    return !code->header.empty() && region.guestAddress >= code->headerAddress && region.guestAddress < code->headerAddress + code->header.size();
}

bool OverlapsRange(std::span<const ShaderRecompiler::MemoryRegion> regions, const std::vector<std::pair<std::uint64_t, std::uint64_t>>& ranges, const std::shared_ptr<const ShaderSnapshot>& code) {
    for (const auto& region : regions) {
        if (InCode(region, code)) continue;
        for (const auto& [begin, end] : ranges) {
            if (begin < region.guestAddress + region.bytes.size() && region.guestAddress < end) return true;
        }
    }
    return false;
}

bool WrittenByRun(std::span<const std::uint32_t> wait, const std::vector<RunLabel>& labels) {
    const std::uint64_t awaited = wait[2] | (static_cast<std::uint64_t>(wait[3]) << 32u);
    const auto bytes = Pm4::WaitAwaitedBytes(wait);
    for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
        if (it->address != awaited || it->size < bytes) continue;
        return Pm4::WaitComparesValue(wait, bytes == 8 ? it->value : (it->value & 0xffffffffu));
    }
    return false;
}

void NoteLabel(std::vector<RunLabel>& labels, std::uint64_t address, std::span<const std::byte> bytes) {
    if (bytes.size() != 4 && bytes.size() != 8) return;
    std::uint64_t value = 0;
    std::memcpy(&value, bytes.data(), bytes.size());
    labels.push_back({address, bytes.size(), value});
}

bool WritesThroughPointers(const ShaderRecompiler::RecompileResult& compiled) {
    return std::any_of(compiled.bindings.begin(), compiled.bindings.end(), [](const ShaderRecompiler::DescriptorBinding& binding) { return binding.role == ShaderRecompiler::DescriptorRole::BdaPagetable; });
}

bool OverlapsLabel(std::span<const ShaderRecompiler::MemoryRegion> regions, const std::vector<RunLabel>& labels) {
    for (const auto& label : labels) {
        for (const auto& region : regions) {
            if (label.address < region.guestAddress + region.bytes.size() && region.guestAddress < label.address + label.size) return true;
        }
    }
    return false;
}

}

void Driver::resolveGroupAhead(const Submission& submission, const QueueState& live, std::size_t from) {
    static const bool allQueues = std::getenv("APS5_GROUP_CAPTURE_ALL_QUEUES") != nullptr;
    const bool pipelines = submitLookahead();
    const bool adopt = allQueues || submission.queue != 0;
    if (!adopt && !pipelines) return;
    auto& resolved = resolvedAhead();
    if (from < resolvedUpTo()) return;
    if (adopt && resolved.contains(from)) return;
    std::shared_ptr<VulkanDevice> localDevice = device.Load();
    if (localDevice == nullptr) {
        if (!pipelines) return;
        GuestMemory::TagGpuLockSite(GuestMemory::GpuLockSite::Draw);
        std::lock_guard createLock(GuestMemory::GpuMutex());
        if (device == nullptr) device = std::make_shared<VulkanDevice>();
        localDevice = device;
    }
    auto& stats = groupCaptureStats();
    const auto begin = std::chrono::steady_clock::now();
    ++stats.runs;
    QueueState scratch = live;
    std::vector<RunLabel> labels;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> writes;
    bool unknownWrites = false;
    for (const auto& label : deferredLabels().labels) NoteLabel(labels, label.address, std::span<const std::byte>(label.bytes).first(label.size));
    struct Resolving {
        std::size_t offset;
        explicit Resolving(bool pipelinesOnly) : offset(currentPacketOffset()) {
            resolvingAhead() = true;
            resolvingForPipelines() = pipelinesOnly;
        }
        ~Resolving() {
            resolvingAhead() = false;
            resolvingForPipelines() = false;
            currentPacketOffset() = offset;
        }
    } resolving(!adopt);
    const auto& commands = submission.commands;
    std::size_t cursor = from;
    while (cursor < commands.size()) {
        const auto header = commands[cursor];
        if (Pm4::FillerPacket(header)) {
            ++cursor;
            continue;
        }
        const auto count = Pm4::PacketWords(header);
        if (cursor + count > commands.size()) break;
        const auto packet = std::span(commands).subspan(cursor, count);
        const auto opcode = (header >> 8u) & 0xffu;
        const auto next = cursor + count;
        if (header == FlipPacketHeader || header == RenderingWaitPacketHeader || opcode == 0x22) {
            if (adopt) break;
            cursor = next;
            continue;
        }
        if (adopt && Pm4::Predicated(header) && scratch.predication.operation != 0) break;
        if (opcode == 0x3c || opcode == 0x93) {
            if (adopt && !Pm4::WaitSatisfiedUnchecked(packet) && !WrittenByRun(packet, labels)) break;
        } else if (opcode == 0x15 || opcode == 0x16) {
            if (resolved.contains(cursor)) break;
            if (adopt && opcode == 0x16) {
                std::uint64_t arguments = 0;
                try {
                    arguments = Pm4::DispatchArgumentAddress(packet, scratch);
                } catch (const std::exception&) {
                    break;
                }
                const auto argumentsEnd = arguments + 3u * sizeof(std::uint32_t);
                const bool written = std::any_of(writes.begin(), writes.end(), [&](const auto& range) { return range.first < argumentsEnd && arguments < range.second; })
                    || std::any_of(labels.begin(), labels.end(), [&](const RunLabel& label) { return label.address < argumentsEnd && arguments < label.address + label.size; });
                if ((unknownWrites || written) && indirectArgumentsPath(packet, arguments) != IndirectGpu) {
                    ++stats.writtenBefore;
                    break;
                }
            }
            currentPacketOffset() = cursor;
            bool failed = false;
            try {
                if (opcode == 0x15) dispatch(scratch, packet, submission);
                else dispatchIndirect(scratch, packet, submission);
                Pm4::NoteDispatchRegisters(packet, scratch);
            } catch (const std::exception&) {
                ++stats.failures;
                failed = true;
            }
            if (failed) {
                if (adopt) break;
                cursor = next;
                continue;
            }
            if (const auto found = resolved.find(cursor); found != resolved.end()) {
                if (pipelines && found->second.compiledResult != nullptr) {
                    localDevice->BuildComputePipelineAhead(found->second.compiledResult);
                    ++stats.computeAhead;
                }
                if (!adopt) {
                    resolved.erase(found);
                } else {
                    if (OverlapsLabel(found->second.captured, labels)) {
                        resolved.erase(found);
                        ++stats.labelOverlaps;
                        break;
                    }
                    if (OverlapsRange(found->second.captured, writes, found->second.registeredShader)) {
                        resolved.erase(found);
                        ++stats.writtenBefore;
                        break;
                    }
                    if (found->second.compiledResult != nullptr) appendWrittenRanges(*found->second.compiledResult, writes);
                    if (found->second.compiledResult == nullptr || WritesThroughPointers(*found->second.compiledResult)) unknownWrites = true;
                }
            } else if (adopt) {
                unknownWrites = true;
            }
        } else if (Pm4::DrawOpcode(opcode)) {
            if (!pipelines) break;
            currentPacketOffset() = cursor;
            const auto drawBegin = std::chrono::steady_clock::now();
            try {
                std::string rejected;
                draw(scratch, packet, submission, rejected);
                ++stats.drawsAhead;
            } catch (const std::exception&) {
                ++stats.drawFailures;
            }
            stats.drawMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - drawBegin).count();
            if (adopt) unknownWrites = true;
        } else if (opcode == 0x37 || opcode == 0x49) {
            const auto label = Pm4::DecodeLabelWrite(packet);
            if (!label) {
                if (adopt) break;
            } else {
                NoteLabel(labels, label->address, label->Bytes());
                writes.emplace_back(label->address, label->address + label->Bytes().size());
            }
        } else if (Pm4::IndirectRegisterOpcode(opcode)) {
            try {
                Pm4::ExecuteIndirectRegisters(packet, submission.registerLists.at(cursor), scratch);
            } catch (const std::exception&) {
                if (adopt) break;
            }
        } else if (opcode != 0x3f && opcode != 0x42 && opcode != 0x46 && opcode != 0x58) {
            if (Pm4::AccessesMemory(header)) {
                if (adopt) break;
            } else {
                try {
                    Pm4::Execute(packet, scratch);
                } catch (const std::exception&) {
                    if (adopt) break;
                }
            }
        }
        cursor = next;
    }
    resolvedUpTo() = std::max(resolvedUpTo(), cursor);
    const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    stats.resolveMs += ms;
    stats.resolveMaxMs = std::max(stats.resolveMaxMs, ms);
    reportGroupCapture(submission.queue);
}

void Driver::reportGroupCapture(std::uint32_t queue) {
    static const bool profile = std::getenv("APS5_PROFILE_DRAW") != nullptr;
    if (!profile) return;
    auto& stats = groupCaptureStats();
    const auto now = std::chrono::steady_clock::now();
    if (now - stats.reported < std::chrono::seconds(10)) return;
    AgcDriver::ProfilePrint_nid_no_patch("[groupcap] queue 0x%x (10 s): %llu runs resolved %llu dispatches (%llu cache hits) in %.1f ms (max %.2f ms); %llu adopted %.2f ms after the resolve on average, max %.2f ms; mismatches: address %llu key %llu; %llu resolve failures, %llu stale, %llu label overlaps, %llu read an earlier dispatch's output, %llu rejected by the device, %llu dropped because the argument re-read differed\n",
                                         queue, static_cast<unsigned long long>(stats.runs), static_cast<unsigned long long>(stats.resolved), static_cast<unsigned long long>(stats.resolvedHits), stats.resolveMs, stats.resolveMaxMs,
                                         static_cast<unsigned long long>(stats.adopted), stats.adopted != 0 ? stats.ageMs / static_cast<double>(stats.adopted) : 0.0, stats.ageMaxMs, static_cast<unsigned long long>(stats.addressMismatches), static_cast<unsigned long long>(stats.keyMismatches),
                                         static_cast<unsigned long long>(stats.failures), static_cast<unsigned long long>(stats.stale), static_cast<unsigned long long>(stats.labelOverlaps), static_cast<unsigned long long>(stats.writtenBefore), static_cast<unsigned long long>(stats.deviceRejects), static_cast<unsigned long long>(stats.argumentMismatches));
    if (stats.drawsAhead != 0 || stats.drawFailures != 0) {
        AgcDriver::ProfilePrint_nid_no_patch("[lookahead] queue 0x%x (10 s): %llu draws decoded ahead in %.1f ms (%llu failed), %llu graphics and %llu compute pipelines handed to the pool\n", queue, static_cast<unsigned long long>(stats.drawsAhead), stats.drawMs, static_cast<unsigned long long>(stats.drawFailures), static_cast<unsigned long long>(stats.graphicsAhead), static_cast<unsigned long long>(stats.computeAhead));
    }
    stats = GroupCaptureStats{};
    stats.reported = now;
}

}

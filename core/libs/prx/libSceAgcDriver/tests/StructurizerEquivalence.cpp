#include "ControlFlow/Structurizer.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#if ANYPS5_ENABLE_SPIRV_TOOLS
#include <initializer_list>
#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.hpp>
#endif

using namespace ShaderRecompiler;

namespace {

class Random {
public:
    explicit Random(std::uint64_t seed) : state(seed) {}

    std::uint32_t Below(std::uint32_t bound) {
        state += 0x9e3779b97f4a7c15ull;
        auto mixed = state;
        mixed = (mixed ^ (mixed >> 30u)) * 0xbf58476d1ce4e5b9ull;
        mixed = (mixed ^ (mixed >> 27u)) * 0x94d049bb133111ebull;
        mixed ^= mixed >> 31u;
        return static_cast<std::uint32_t>(mixed % bound);
    }

    bool Percent(std::uint32_t chance) {
        return Below(100u) < chance;
    }

private:
    std::uint64_t state;
};

struct Shape {
    std::vector<std::vector<std::uint32_t>> targets;
    std::vector<std::uint32_t> words;
    std::uint32_t entry = 0;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

std::string blockName(std::uint32_t id) {
    return "block " + std::to_string(id);
}

std::vector<std::uint32_t> sortedUnique(std::vector<std::uint32_t> values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

bool isEmpty(const BasicBlock& block) {
    return block.instructionBegin == block.instructionEnd;
}

std::vector<std::uint32_t> terminatorTargets(const Terminator& terminator) {
    if (terminator.kind == TerminatorKind::Branch) return {terminator.trueBlock};
    if (terminator.kind == TerminatorKind::ConditionalBranch) return {terminator.trueBlock, terminator.falseBlock};
    return {};
}

constexpr std::uint32_t maxConditionals = 6u;

void assignWords(Shape& shape, Random& random) {
    const auto count = static_cast<std::uint32_t>(shape.targets.size());
    shape.words.assign(count, 0u);
    const auto mode = random.Below(100u);
    if (mode < 50u) return;
    if (mode < 58u) {
        std::fill(shape.words.begin(), shape.words.end(), 1000u);
        return;
    }
    if (mode < 70u) {
        shape.words[random.Below(count)] = 1000u;
        return;
    }
    constexpr std::uint32_t choices[] = {0u, 16u, 256u, 257u, 1000u};
    for (auto& words : shape.words) {
        const auto choice = random.Below(6u);
        words = choice < 5u ? choices[choice] : random.Below(2001u);
    }
}

Shape randomShape(std::uint64_t seed) {
    Random random(seed);
    const auto count = 2u + random.Below(1u + random.Below(23u));
    std::vector<std::vector<std::uint32_t>> targets(count);
    std::vector<std::uint32_t> parents(count, InvalidControlFlowId);
    std::uint32_t conditionals = 0;
    std::vector<std::uint32_t> open{0u};
    for (std::uint32_t block = 1; block < count; ++block) {
        const auto parent = open[random.Percent(45u) ? static_cast<std::uint32_t>(open.size() - 1u) : random.Below(static_cast<std::uint32_t>(open.size()))];
        targets[parent].push_back(block);
        parents[block] = parent;
        if (targets[parent].size() == 2u) ++conditionals;
        open.push_back(block);
        std::erase_if(open, [&](std::uint32_t candidate) { return targets[candidate].size() == 2u || (conditionals == maxConditionals && !targets[candidate].empty()); });
    }
    std::vector<std::uint32_t> leaves;
    for (std::uint32_t block = 0; block < count; ++block) {
        if (targets[block].empty()) leaves.push_back(block);
    }
    const auto returnChance = 10u + random.Below(30u);
    const auto conditionalChance = 25u + random.Below(40u);
    const auto extraTarget = [&](std::uint32_t block) {
        if (random.Percent(30u)) {
            std::vector<std::uint32_t> ancestors;
            for (auto ancestor = block; ancestor != InvalidControlFlowId; ancestor = parents[ancestor]) ancestors.push_back(ancestor);
            return ancestors[random.Below(static_cast<std::uint32_t>(ancestors.size()))];
        }
        return block + 1u < count ? block + 1u + random.Below(count - block - 1u) : random.Below(count);
    };
    for (std::uint32_t block = 0; block < count; ++block) {
        auto& edges = targets[block];
        auto degree = edges.size() == 2u || (conditionals < maxConditionals && random.Percent(conditionalChance)) ? 2u : 1u;
        if (edges.empty() && random.Percent(returnChance)) degree = 0u;
        if (degree == 2u && edges.size() < 2u) ++conditionals;
        while (edges.size() < degree) edges.push_back(extraTarget(block));
        if (edges.size() == 2u && random.Percent(50u)) std::swap(edges[0], edges[1]);
        if (edges.size() == 1u && conditionals < maxConditionals && random.Percent(5u)) {
            edges.push_back(edges[0]);
            ++conditionals;
        }
    }
    if (std::none_of(targets.begin(), targets.end(), [](const std::vector<std::uint32_t>& edges) { return edges.empty(); }) && random.Percent(85u)) {
        targets[leaves[random.Below(static_cast<std::uint32_t>(leaves.size()))]].clear();
    }
    std::vector<std::uint32_t> ids(count);
    for (std::uint32_t id = 0; id < count; ++id) ids[id] = id;
    for (auto size = count; size > 1u; --size) std::swap(ids[size - 1u], ids[random.Below(size)]);
    Shape shape;
    shape.targets.resize(count);
    for (std::uint32_t block = 0; block < count; ++block) {
        for (const auto target : targets[block]) shape.targets[ids[block]].push_back(ids[target]);
    }
    shape.entry = ids[0];
    assignWords(shape, random);
    return shape;
}

Shape codeShape(std::uint64_t seed) {
    Random random(seed);
    for (;;) {
        const auto count = 2u + random.Below(23u);
        std::vector<std::vector<std::uint32_t>> targets(count);
        std::uint32_t conditionals = 0;
        for (std::uint32_t block = 0; block < count; ++block) {
            const auto next = block + 1u;
            const auto backward = [&] { return block - random.Below(std::min(block + 1u, 8u)); };
            const auto choice = random.Below(100u);
            auto& edges = targets[block];
            if (next == count) {
                if (choice >= 85u) edges = {backward()};
                continue;
            }
            const auto forward = next + random.Below(std::min(count - next, 6u));
            if (choice < 30u || (choice < 75u && conditionals == maxConditionals)) {
                edges = {next};
            } else if (choice < 60u) {
                edges = {forward, next};
                ++conditionals;
            } else if (choice < 75u) {
                edges = {backward(), next};
                ++conditionals;
            } else if (choice < 85u) {
                edges = {forward};
            } else if (choice < 90u) {
                edges = {backward()};
            }
            if (edges.size() == 2u && random.Percent(50u)) std::swap(edges[0], edges[1]);
        }
        std::vector<std::uint32_t> ids(count, InvalidControlFlowId);
        std::vector<std::uint32_t> pending{0u};
        ids[0] = 0u;
        while (!pending.empty()) {
            const auto block = pending.back();
            pending.pop_back();
            for (const auto target : targets[block]) {
                if (ids[target] != InvalidControlFlowId) continue;
                ids[target] = 0u;
                pending.push_back(target);
            }
        }
        std::uint32_t kept = 0;
        for (auto& id : ids) {
            if (id != InvalidControlFlowId) id = kept++;
        }
        if (kept < 2u) continue;
        Shape shape;
        shape.targets.resize(kept);
        for (std::uint32_t block = 0; block < count; ++block) {
            if (ids[block] == InvalidControlFlowId) continue;
            for (const auto target : targets[block]) shape.targets[ids[block]].push_back(ids[target]);
        }
        assignWords(shape, random);
        return shape;
    }
}

ControlFlowGraph makeGraph(const Shape& shape) {
    constexpr BranchCondition conditions[] = {BranchCondition::SccZero, BranchCondition::SccNonZero, BranchCondition::VccZero, BranchCondition::VccNonZero, BranchCondition::ExecZero, BranchCondition::ExecNonZero};
    ControlFlowGraph graph;
    graph.entryBlock = shape.entry;
    const auto count = static_cast<std::uint32_t>(shape.targets.size());
    for (std::uint32_t id = 0; id < count; ++id) {
        BasicBlock block;
        block.id = id;
        block.startProgramCounter = id * 8u;
        block.endProgramCounter = id * 8u + 8u;
        block.instructionBegin = id * 2u;
        block.instructionEnd = id * 2u + 2u;
        block.estimatedSpirvWords = shape.words[id];
        const auto& targets = shape.targets[id];
        auto& terminator = block.terminator;
        if (targets.size() == 1u) {
            terminator.kind = TerminatorKind::Branch;
            terminator.trueBlock = targets[0];
        } else if (targets.size() == 2u) {
            terminator.kind = TerminatorKind::ConditionalBranch;
            terminator.condition = conditions[id % 6u];
            terminator.trueBlock = targets[0];
            terminator.falseBlock = targets[1];
        }
        block.successors = sortedUnique(targets);
        graph.blocks.push_back(std::move(block));
    }
    for (const auto& block : graph.blocks) {
        for (const auto successor : block.successors) graph.blocks[successor].predecessors.push_back(block.id);
    }
    return graph;
}

std::string describe(const Shape& shape) {
    std::string text = "entry " + std::to_string(shape.entry) + " {";
    for (std::size_t block = 0; block < shape.targets.size(); ++block) {
        text += block == 0 ? "{" : ", {";
        for (std::size_t edge = 0; edge < shape.targets[block].size(); ++edge) text += (edge == 0 ? "" : ",") + std::to_string(shape.targets[block][edge]);
        text += "}";
    }
    text += "} words {";
    for (std::size_t block = 0; block < shape.words.size(); ++block) text += (block == 0 ? "" : ",") + std::to_string(shape.words[block]);
    return text + "}";
}

bool sameCopy(const BasicBlock& copy, const BasicBlock& original) {
    const auto& copied = copy.terminator;
    const auto& source = original.terminator;
    return copy.instructionEnd == original.instructionEnd && copy.startProgramCounter == original.startProgramCounter && copy.endProgramCounter == original.endProgramCounter && copy.estimatedSpirvWords == original.estimatedSpirvWords && copied.kind == source.kind && copied.condition == source.condition && copied.gotoVariable == source.gotoVariable && copied.gotoValue == source.gotoValue && copied.indirectPcSgpr == source.indirectPcSgpr && copied.indirectSelectorCode == source.indirectSelectorCode && copied.indirectTargetProgramCounters == source.indirectTargetProgramCounters && copied.indirectTargets == source.indirectTargets && copied.indirectSelectorValues == source.indirectSelectorValues && copied.indirectSelectorTargets == source.indirectSelectorTargets;
}

bool sameRun(const BasicBlock& copy, const BasicBlock& original) {
    return copy.instructionBegin == original.instructionBegin && copy.instructionEnd == original.instructionEnd && copy.startProgramCounter == original.startProgramCounter && copy.endProgramCounter == original.endProgramCounter && copy.terminator.kind == original.terminator.kind && copy.terminator.condition == original.terminator.condition;
}

void requireBlockShapes(const ControlFlowGraph& input, const ControlFlowGraph& output) {
    std::map<std::uint32_t, std::uint32_t> originals;
    for (const auto& block : input.blocks) originals.emplace(block.instructionBegin, block.id);
    std::vector<bool> copied(input.blocks.size(), false);
    std::uint32_t emptyReturns = 0;
    for (const auto& block : output.blocks) {
        const auto& terminator = block.terminator;
        if (terminator.loopHeader && (!isEmpty(block) || terminator.kind != TerminatorKind::Branch)) fail("loop header " + blockName(block.id) + " is not an empty branch");
        if (!isEmpty(block)) {
            const auto original = originals.find(block.instructionBegin);
            if (original == originals.end() || !sameCopy(block, input.blocks[original->second])) fail(blockName(block.id) + " is not an unchanged copy of an original block");
            copied[original->second] = true;
            continue;
        }
        const bool routed = terminator.gotoVariable != InvalidControlFlowId;
        const bool always = terminator.condition == BranchCondition::Always;
        const bool forwarder = terminator.kind == TerminatorKind::Branch && always && !routed && terminator.gotoValue == -1;
        const bool assignment = terminator.kind == TerminatorKind::Branch && always && routed && (terminator.gotoValue == 0 || terminator.gotoValue == 1);
        const bool dispatch = terminator.kind == TerminatorKind::ConditionalBranch && terminator.condition == BranchCondition::GotoVariable && routed && terminator.gotoValue == -1;
        const bool emptyReturn = terminator.kind == TerminatorKind::Return && always && !routed && terminator.gotoValue == -1;
        if (emptyReturn) ++emptyReturns;
        if (!forwarder && !assignment && !dispatch && !emptyReturn) fail("empty " + blockName(block.id) + " is not a forwarder, a route assignment, a route dispatch or an empty return");
    }
    if (emptyReturns > 1u) fail("the structured graph has " + std::to_string(emptyReturns) + " empty returns");
    const auto missing = std::find(copied.begin(), copied.end(), false);
    if (missing != copied.end()) fail("original block " + std::to_string(missing - copied.begin()) + " has no copy");
}

std::vector<std::uint32_t> immediateDominators(const ControlFlowGraph& graph) {
    const auto count = static_cast<std::uint32_t>(graph.blocks.size());
    std::vector<std::uint32_t> order;
    std::vector<std::uint32_t> position(count, InvalidControlFlowId);
    std::vector<bool> seen(count, false);
    std::vector<std::pair<std::uint32_t, std::size_t>> stack{{0u, 0u}};
    seen[0] = true;
    while (!stack.empty()) {
        auto& [blockId, next] = stack.back();
        const auto& successors = graph.blocks[blockId].successors;
        if (next < successors.size()) {
            const auto successor = successors[next++];
            if (!seen[successor]) {
                seen[successor] = true;
                stack.emplace_back(successor, 0u);
            }
            continue;
        }
        order.push_back(blockId);
        stack.pop_back();
    }
    std::reverse(order.begin(), order.end());
    for (std::uint32_t index = 0; index < order.size(); ++index) position[order[index]] = index;
    std::vector<std::uint32_t> dominators(count, InvalidControlFlowId);
    dominators[0] = 0u;
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto blockId : order) {
            if (blockId == 0u) continue;
            auto dominator = InvalidControlFlowId;
            for (const auto predecessor : graph.blocks[blockId].predecessors) {
                if (dominators[predecessor] == InvalidControlFlowId) continue;
                if (dominator == InvalidControlFlowId) {
                    dominator = predecessor;
                    continue;
                }
                auto other = predecessor;
                while (dominator != other) {
                    while (position[dominator] > position[other]) dominator = dominators[dominator];
                    while (position[other] > position[dominator]) other = dominators[other];
                }
            }
            if (dominators[blockId] != dominator) {
                dominators[blockId] = dominator;
                changed = true;
            }
        }
    }
    return dominators;
}

void requireStructure(const ControlFlowGraph& output) {
    const auto count = static_cast<std::uint32_t>(output.blocks.size());
    const auto dominators = immediateDominators(output);
    const auto dominates = [&](std::uint32_t dominator, std::uint32_t blockId) {
        for (;;) {
            if (blockId == dominator) return true;
            if (blockId == 0u) return false;
            blockId = dominators[blockId];
        }
    };
    std::vector<std::uint32_t> mergeOwners(count, InvalidControlFlowId);
    std::vector<std::uint32_t> incoming(count, 0u);
    for (const auto& block : output.blocks) {
        const auto& terminator = block.terminator;
        if (block.id != 0u && dominators[block.id] >= block.id) fail(blockName(block.id) + " comes before its immediate dominator " + std::to_string(dominators[block.id]));
        if ((terminator.continueBlock != InvalidControlFlowId) != terminator.loopHeader) fail(blockName(block.id) + " has a continue block but is not a loop header, or the reverse");
        if (terminator.loopHeader) {
            const auto merge = terminator.mergeBlock;
            const auto latch = terminator.continueBlock;
            if (merge == InvalidControlFlowId || merge == latch || merge == block.id || latch == block.id) fail("loop header " + blockName(block.id) + " has merge " + std::to_string(merge) + " and continue " + std::to_string(latch));
            const auto& exits = output.blocks[latch].successors;
            if (!dominates(block.id, latch) || std::find(exits.begin(), exits.end(), block.id) == exits.end()) fail("continue " + blockName(latch) + " of the loop at " + blockName(block.id) + " is not its latch");
            if (std::any_of(exits.begin(), exits.end(), [&](std::uint32_t exit) { return exit != block.id && exit != merge; })) fail("continue " + blockName(latch) + " of the loop at " + blockName(block.id) + " branches outside the header and the merge");
        }
        if (terminator.mergeBlock != InvalidControlFlowId) {
            if (!terminator.loopHeader && terminator.kind != TerminatorKind::ConditionalBranch) fail(blockName(block.id) + " declares a merge but is neither a loop header nor a conditional branch");
            auto& owner = mergeOwners[terminator.mergeBlock];
            if (owner != InvalidControlFlowId) fail("block " + std::to_string(terminator.mergeBlock) + " is the merge of blocks " + std::to_string(owner) + " and " + std::to_string(block.id));
            owner = block.id;
        }
        if (terminator.kind == TerminatorKind::ConditionalBranch && !terminator.loopHeader && terminator.mergeBlock == InvalidControlFlowId && terminator.trueBlock != terminator.falseBlock) {
            const BasicBlock* loop = nullptr;
            for (const auto& header : output.blocks) {
                if (header.terminator.loopHeader && dominates(header.id, block.id) && !dominates(header.terminator.mergeBlock, block.id) && (loop == nullptr || dominates(loop->id, header.id))) loop = &header;
            }
            const auto exits = [&](std::uint32_t target) { return loop != nullptr && (target == loop->terminator.mergeBlock || target == loop->terminator.continueBlock); };
            if (!exits(terminator.trueBlock) && !exits(terminator.falseBlock)) fail(blockName(block.id) + " branches without a merge and does not leave its loop");
        }
        for (const auto successor : block.successors) {
            if (!dominates(successor, block.id)) {
                ++incoming[successor];
                continue;
            }
            const auto& header = output.blocks[successor].terminator;
            if (!header.loopHeader || header.continueBlock != block.id) fail("back edge " + std::to_string(block.id) + " -> " + std::to_string(successor) + " does not come from the continue block of a loop header");
        }
    }
    std::vector<std::uint32_t> ready{0u};
    std::uint32_t sorted = 0;
    while (!ready.empty()) {
        const auto blockId = ready.back();
        ready.pop_back();
        ++sorted;
        for (const auto successor : output.blocks[blockId].successors) {
            if (!dominates(successor, blockId) && --incoming[successor] == 0u) ready.push_back(successor);
        }
    }
    if (sorted != count) fail("the structured graph is irreducible");
}

void requireContract(const ControlFlowGraph& input, const ControlFlowGraph& output) {
    const auto count = static_cast<std::uint32_t>(output.blocks.size());
    if (count == 0u || output.entryBlock != 0u) fail("the structured graph does not start at block 0");
    if (output.irreducible) fail("the structured graph is marked irreducible");
    std::vector<std::vector<std::uint32_t>> predecessors(count);
    std::vector<bool> returns(count, false);
    for (std::uint32_t id = 0; id < count; ++id) {
        const auto& block = output.blocks[id];
        const auto& terminator = block.terminator;
        if (block.id != id) fail("the block at index " + std::to_string(id) + " has id " + std::to_string(block.id));
        if (terminator.kind != TerminatorKind::Branch && terminator.kind != TerminatorKind::ConditionalBranch && terminator.kind != TerminatorKind::Return) fail(blockName(id) + " has an unsupported terminator");
        const auto targets = terminatorTargets(terminator);
        if (std::any_of(targets.begin(), targets.end(), [&](std::uint32_t target) { return target >= count; })) fail(blockName(id) + " branches out of range");
        if ((terminator.mergeBlock != InvalidControlFlowId && terminator.mergeBlock >= count) || (terminator.continueBlock != InvalidControlFlowId && terminator.continueBlock >= count)) fail(blockName(id) + " names an out-of-range merge or continue block");
        if (block.successors != sortedUnique(targets)) fail(blockName(id) + " lists successors that are not its sorted branch targets");
        for (const auto successor : block.successors) predecessors[successor].push_back(id);
        returns[id] = terminator.kind == TerminatorKind::Return;
    }
    for (std::uint32_t id = 0; id < count; ++id) {
        if (output.blocks[id].predecessors != predecessors[id]) fail(blockName(id) + " lists predecessors that are not the inverse of the successors");
    }
    std::vector<bool> reached(count, false);
    std::vector<std::uint32_t> pending{0u};
    reached[0] = true;
    while (!pending.empty()) {
        const auto blockId = pending.back();
        pending.pop_back();
        for (const auto successor : output.blocks[blockId].successors) {
            if (reached[successor]) continue;
            reached[successor] = true;
            pending.push_back(successor);
        }
    }
    for (std::uint32_t id = 0; id < count; ++id) {
        if (returns[id]) pending.push_back(id);
    }
    while (!pending.empty()) {
        const auto blockId = pending.back();
        pending.pop_back();
        for (const auto predecessor : predecessors[blockId]) {
            if (returns[predecessor]) continue;
            returns[predecessor] = true;
            pending.push_back(predecessor);
        }
    }
    for (std::uint32_t id = 0; id < count; ++id) {
        if (!reached[id]) fail(blockName(id) + " is unreachable");
        if (!returns[id]) fail(blockName(id) + " cannot reach a return");
    }
    requireBlockShapes(input, output);
    requireStructure(output);
}

class StructuredRun {
public:
    explicit StructuredRun(const ControlFlowGraph& structured) : graph(structured), variables(structured.blocks.size(), -1) {
        std::map<std::uint32_t, std::int32_t> dense;
        for (const auto& block : graph.blocks) {
            const auto& terminator = block.terminator;
            if (terminator.gotoValue < 0 && terminator.condition != BranchCondition::GotoVariable) continue;
            variables[block.id] = dense.emplace(terminator.gotoVariable, static_cast<std::int32_t>(dense.size())).first->second;
        }
        variableCount = dense.size();
        live.assign(graph.blocks.size(), std::string(variableCount, '\0'));
        bool changed = variableCount != 0u;
        while (changed) {
            changed = false;
            for (auto blockId = graph.blocks.size(); blockId-- > 0u;) {
                const auto& block = graph.blocks[blockId];
                std::string needed(variableCount, '\0');
                for (const auto successor : block.successors) {
                    for (std::size_t variable = 0; variable < variableCount; ++variable) {
                        if (live[successor][variable] != '\0') needed[variable] = '\1';
                    }
                }
                if (block.terminator.condition == BranchCondition::GotoVariable) needed[variables[blockId]] = '\1';
                if (block.terminator.gotoValue >= 0) needed[variables[blockId]] = '\0';
                if (needed != live[blockId]) {
                    live[blockId] = std::move(needed);
                    changed = true;
                }
            }
        }
    }

    std::size_t VariableCount() const {
        return variableCount;
    }

    std::uint32_t Settle(std::uint32_t blockId, std::string& routes) const {
        std::vector<std::pair<std::uint32_t, std::string>> headers;
        for (std::size_t step = 0; step < 1000000u; ++step) {
            const auto& block = graph.blocks[blockId];
            const auto& terminator = block.terminator;
            const bool dispatch = terminator.kind == TerminatorKind::ConditionalBranch && terminator.condition == BranchCondition::GotoVariable;
            if (!isEmpty(block) || (terminator.kind != TerminatorKind::Branch && !dispatch)) {
                project(blockId, routes);
                return blockId;
            }
            if (terminator.loopHeader) {
                std::pair<std::uint32_t, std::string> state{blockId, routes};
                project(blockId, state.second);
                if (std::find(headers.begin(), headers.end(), state) != headers.end()) fail("the structured graph loops forever through empty blocks at " + blockName(blockId));
                headers.push_back(std::move(state));
            }
            if (terminator.gotoValue >= 0) routes[variables[blockId]] = terminator.gotoValue != 0 ? '\2' : '\1';
            if (!dispatch) {
                blockId = terminator.trueBlock;
                continue;
            }
            const auto value = routes[variables[blockId]];
            if (value == '\0') fail(blockName(blockId) + " reads route variable " + std::to_string(terminator.gotoVariable) + " before any path sets it");
            blockId = value == '\2' ? terminator.trueBlock : terminator.falseBlock;
        }
        fail("the structured graph does not reach an original block from " + blockName(blockId));
    }

private:
    void project(std::uint32_t blockId, std::string& routes) const {
        for (std::size_t variable = 0; variable < variableCount; ++variable) {
            if (live[blockId][variable] == '\0') routes[variable] = '\0';
        }
    }

    const ControlFlowGraph& graph;
    std::vector<std::int32_t> variables;
    std::size_t variableCount = 0;
    std::vector<std::string> live;
};

struct RunPair {
    std::uint32_t structured = 0;
    std::uint32_t original = 0;
    std::string routes;
    std::size_t parent = 0;
    char decision = 0;
};

void requireSameExecutions(const ControlFlowGraph& input, const ControlFlowGraph& output) {
    constexpr auto root = std::numeric_limits<std::size_t>::max();
    const StructuredRun run(output);
    std::vector<RunPair> pairs;
    std::unordered_set<std::string> seen;
    const auto witness = [&](std::size_t index, char decision) {
        std::string decisions;
        std::string path;
        if (decision != 0) decisions += decision;
        for (auto at = index; at != root; at = pairs[at].parent) {
            if (pairs[at].decision != 0) decisions.insert(decisions.begin(), pairs[at].decision);
            path.insert(0, " " + std::to_string(pairs[at].original));
        }
        return "after decisions \"" + decisions + "\" through original blocks" + path;
    };
    const auto visit = [&](std::size_t parent, char decision, std::uint32_t structured, std::uint32_t original, std::string routes) {
        try {
            structured = run.Settle(structured, routes);
        } catch (const std::runtime_error& error) {
            fail((parent == root ? std::string("at the entry") : witness(parent, decision)) + ": " + error.what());
        }
        std::string key(reinterpret_cast<const char*>(&structured), sizeof(structured));
        key.append(reinterpret_cast<const char*>(&original), sizeof(original));
        key += routes;
        if (!seen.insert(std::move(key)).second) return;
        if (pairs.size() >= 2000000u) fail("more than 2000000 execution states");
        pairs.push_back({structured, original, std::move(routes), parent, decision});
    };
    visit(root, 0, 0u, input.entryBlock, std::string(run.VariableCount(), '\0'));
    for (std::size_t index = 0; index < pairs.size(); ++index) {
        const auto& copy = output.blocks[pairs[index].structured];
        const auto& original = input.blocks[pairs[index].original];
        if (!sameRun(copy, original)) fail(witness(index, 0) + ": the input runs original block " + std::to_string(original.id) + " but the structured graph runs " + blockName(copy.id) + " with instructions [" + std::to_string(copy.instructionBegin) + ", " + std::to_string(copy.instructionEnd) + ")");
        const auto routes = pairs[index].routes;
        if (original.terminator.kind == TerminatorKind::Branch) visit(index, '-', copy.terminator.trueBlock, original.terminator.trueBlock, routes);
        if (original.terminator.kind != TerminatorKind::ConditionalBranch) continue;
        visit(index, 'T', copy.terminator.trueBlock, original.terminator.trueBlock, routes);
        visit(index, 'F', copy.terminator.falseBlock, original.terminator.falseBlock, routes);
    }
}

#if ANYPS5_ENABLE_SPIRV_TOOLS
void addInstruction(std::vector<std::uint32_t>& words, spv::Op opcode, std::initializer_list<std::uint32_t> operands) {
    words.push_back(static_cast<std::uint32_t>(operands.size() + 1u) << spv::WordCountShift | static_cast<std::uint32_t>(opcode));
    words.insert(words.end(), operands);
}

std::vector<std::uint32_t> skeletonModule(const ControlFlowGraph& graph, std::uint32_t version) {
    constexpr std::uint32_t voidType = 1u;
    constexpr std::uint32_t boolType = 2u;
    constexpr std::uint32_t functionType = 3u;
    constexpr std::uint32_t function = 4u;
    constexpr std::uint32_t prologue = 5u;
    constexpr std::uint32_t start = 6u;
    constexpr std::uint32_t firstLabel = 7u;
    const auto count = static_cast<std::uint32_t>(graph.blocks.size());
    const auto label = [](std::uint32_t blockId) { return firstLabel + blockId; };
    std::vector<std::uint32_t> conditions(count, 0u);
    auto bound = firstLabel + count;
    for (const auto& block : graph.blocks) {
        if (block.terminator.kind == TerminatorKind::ConditionalBranch && block.terminator.trueBlock != block.terminator.falseBlock) conditions[block.id] = bound++;
    }
    std::vector<std::uint32_t> words{spv::MagicNumber, version, 0u, bound, 0u};
    addInstruction(words, spv::OpCapability, {spv::CapabilityShader});
    addInstruction(words, spv::OpMemoryModel, {spv::AddressingModelLogical, spv::MemoryModelGLSL450});
    addInstruction(words, spv::OpEntryPoint, {spv::ExecutionModelGLCompute, function, 0x6e69616du, 0u});
    addInstruction(words, spv::OpExecutionMode, {function, spv::ExecutionModeLocalSize, 1u, 1u, 1u});
    addInstruction(words, spv::OpTypeVoid, {voidType});
    addInstruction(words, spv::OpTypeBool, {boolType});
    addInstruction(words, spv::OpTypeFunction, {functionType, voidType});
    for (const auto condition : conditions) {
        if (condition != 0u) addInstruction(words, spv::OpUndef, {boolType, condition});
    }
    addInstruction(words, spv::OpFunction, {voidType, function, spv::FunctionControlMaskNone, functionType});
    addInstruction(words, spv::OpLabel, {prologue});
    addInstruction(words, spv::OpBranch, {start});
    addInstruction(words, spv::OpLabel, {start});
    addInstruction(words, spv::OpBranch, {label(0u)});
    for (const auto& block : graph.blocks) {
        const auto& terminator = block.terminator;
        addInstruction(words, spv::OpLabel, {label(block.id)});
        if (terminator.kind != TerminatorKind::Branch && terminator.kind != TerminatorKind::ConditionalBranch) {
            addInstruction(words, spv::OpReturn, {});
            continue;
        }
        if (terminator.loopHeader) {
            addInstruction(words, spv::OpLoopMerge, {label(terminator.mergeBlock), label(terminator.continueBlock), spv::LoopControlMaskNone});
        } else if (conditions[block.id] != 0u && terminator.mergeBlock != InvalidControlFlowId) {
            addInstruction(words, spv::OpSelectionMerge, {label(terminator.mergeBlock), spv::SelectionControlMaskNone});
        }
        if (conditions[block.id] == 0u) {
            addInstruction(words, spv::OpBranch, {label(terminator.trueBlock)});
        } else {
            addInstruction(words, spv::OpBranchConditional, {conditions[block.id], label(terminator.trueBlock), label(terminator.falseBlock)});
        }
    }
    addInstruction(words, spv::OpFunctionEnd, {});
    return words;
}

class SkeletonValidator {
public:
    SkeletonValidator(spv_target_env environment, std::uint32_t moduleVersion) : tools(environment), version(moduleVersion) {
        tools.SetMessageConsumer([this](spv_message_level_t, const char*, const spv_position_t&, const char* message) {
            if (message != nullptr) diagnostics += std::string(message) + '\n';
        });
    }

    void Require(const ControlFlowGraph& graph) {
        diagnostics.clear();
        if (!tools.Validate(skeletonModule(graph, version))) fail("SPIR-V " + std::to_string(version >> 16u) + "." + std::to_string(version >> 8u & 0xffu) + " rejects the structured graph: " + diagnostics);
    }

private:
    spvtools::SpirvTools tools;
    std::uint32_t version;
    std::string diagnostics;
};
#endif

void checkShape(const Shape& shape) {
    const auto input = makeGraph(shape);
    auto output = input;
    try {
        Structurizer{}.Structurize(output);
    } catch (const std::exception& error) {
        fail(std::string("Structurize refused: ") + error.what());
    }
    requireContract(input, output);
    requireSameExecutions(input, output);
#if ANYPS5_ENABLE_SPIRV_TOOLS
    static SkeletonValidator spirv13(SPV_ENV_VULKAN_1_1, 0x00010300u);
    static SkeletonValidator spirv14(SPV_ENV_VULKAN_1_1_SPIRV_1_4, 0x00010400u);
    spirv13.Require(output);
    spirv14.Require(output);
#endif
}

}

int main() {
    constexpr std::uint32_t randomShapes = 1000u;
    constexpr std::uint32_t codeShapes = 1000u;
    std::uint32_t failures = 0;
    const auto check = [&](const char* kind, std::uint32_t index, const Shape& shape) {
        try {
            checkShape(shape);
        } catch (const std::exception& error) {
            if (++failures <= 5u) std::fprintf(stderr, "%s graph %u (%s): %s\n", kind, index, describe(shape).c_str(), error.what());
        }
    };
    for (std::uint32_t index = 0; index < randomShapes; ++index) check("random", index, randomShape(0x243f6a8885a308d3ull + index));
    for (std::uint32_t index = 0; index < codeShapes; ++index) check("code-like", index, codeShape(0x13198a2e03707344ull + index));
    if (failures != 0u) {
        std::fprintf(stderr, "%u of %u graphs failed\n", failures, randomShapes + codeShapes);
        return 1;
    }
    std::printf("%u graphs structured, valid and equivalent\n", randomShapes + codeShapes);
    return 0;
}

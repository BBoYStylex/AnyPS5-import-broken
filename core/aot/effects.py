from collections import deque
from typing import NamedTuple

from capstone import CS_AC_WRITE
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

import abi


class MemoryEffects(NamedTuple):
    writes: tuple
    bindings: frozenset = frozenset()


def validate_provider(contract):
    if 'memory_effects' not in contract:
        return
    effects = contract['memory_effects']
    if not isinstance(effects, dict) or set(effects) != {'writes', 'callbacks', 'evidence'}:
        raise ValueError('provider memory effects require writes, callbacks and evidence')
    if type(effects['callbacks']) is not bool or not isinstance(effects['evidence'], str) or not effects['evidence'].strip():
        raise ValueError('provider memory effects require a callback boolean and nonempty evidence')
    if not isinstance(effects['writes'], list):
        raise ValueError('provider memory writes must be a list')
    parameters = contract['parameters']
    for write in effects['writes']:
        if not isinstance(write, dict) or set(write) not in ({'parameter', 'size'}, {'parameter', 'size_parameter'}):
            raise ValueError('provider writes require a pointer parameter and one size source')
        index = write['parameter']
        if type(index) is not int or not 0 <= index < len(parameters) or parameters[index] != 'ptr':
            raise ValueError('provider write target must be a declared pointer parameter')
        if 'size' in write:
            if type(write['size']) is not int or not 0 <= write['size'] <= (1 << 64) - 1:
                raise ValueError('provider write size must be an unsigned 64-bit integer')
        else:
            index = write['size_parameter']
            if type(index) is not int or not 0 <= index < len(parameters) or parameters[index] not in abi.INTEGER_TYPES - {'ptr'}:
                raise ValueError('provider write size parameter must be an integer')


def provider_writes(function, contract, state):
    effects = contract.get('memory_effects')
    if effects is None or effects['callbacks']:
        return None
    locations = abi.locations(contract['parameters'])
    writes = []
    for write in effects['writes']:
        size = write.get('size')
        if size is None:
            length = locations[write['size_parameter']]
            if length['kind'] != 'register':
                return None
            size = state['constants'].get(length['name'])
            if size is None:
                return None
            size &= (1 << length['bits']) - 1
        if size == 0:
            continue
        location = locations[write['parameter']]
        if location['kind'] != 'register':
            return None
        name = location['name']
        pointer = state['pointers'].get(name)
        if name in ('rsp', 'rbp') and state[name] is not None:
            pointer = ('frame', 0, state[name])
        if pointer is None or pointer[0] not in ('frame', 'image'):
            return None
        kind, _, begin = pointer
        end = begin + size
        if kind == 'frame':
            if not -function.depth <= begin <= end <= 0:
                return None
        elif not any(spec['address'] <= begin <= end <= spec['address'] + spec['size'] for spec in function.image.data):
            return None
        writes.append((kind, begin, end))
    return tuple(writes)


def direct_target(function, ins):
    if ins.mnemonic != 'call' and ins.address not in function.tail_calls:
        return None
    operand = ins.operands[0]
    if operand.type == X86_OP_IMM and operand.imm in function.image.functions:
        return operand.imm
    return None


def call_targets(function, ins):
    target = direct_target(function, ins)
    if target is not None:
        return {target}
    site = function.image.annotated_calls(function.contract).get(ins.address)
    return set() if site is None else set(site['target_addresses']) & function.image.functions.keys()


def required_bindings(function, ins):
    return frozenset().union(*(function.image.provider_requirements[target] for target in call_targets(function, ins)))


def infer_requirements(functions):
    image = functions[0].image
    requirements = {function.contract['address']: set() for function in functions}
    callers = {address: set() for address in requirements}
    for function in functions:
        address = function.contract['address']
        for ins in function.instructions.values():
            if ins.mnemonic != 'call' and ins.address not in function.tail_calls:
                continue
            contract, slot = image.call(ins, function.contract)
            effects = contract.get('memory_effects')
            if type(slot) is int and effects is not None and not effects['callbacks']:
                requirements[address].add(slot)
            for target in call_targets(function, ins):
                callers[target].add(address)
    pending = deque(address for address, slots in requirements.items() if slots)
    while pending:
        target = pending.popleft()
        for caller in callers[target]:
            if not requirements[target] <= requirements[caller]:
                requirements[caller].update(requirements[target])
                pending.append(caller)
    image.provider_requirements = {address: frozenset(slots) for address, slots in requirements.items()}


def call_writes(function, ins, state=None):
    state = function.states[ins.address] if state is None else state
    target = direct_target(function, ins)
    if target is not None:
        result = function.image.memory_effects.get(target)
        return result if result is not None and result.bindings <= state['import_bindings'] else None
    contract, slot = function.image.call(ins, function.contract)
    if slot not in state['import_bindings']:
        return None
    writes = provider_writes(function, contract, state)
    return None if writes is None else MemoryEffects(writes, frozenset({slot}))


def summarize(function):
    writes, bindings = set(), set()
    for address, ins in function.instructions.items():
        if ins.mnemonic == 'call' or address in function.tail_calls:
            nested = call_writes(function, ins)
            if nested is None:
                return None
            writes.update((begin, end) for kind, begin, end in nested.writes if kind == 'image')
            bindings.update(nested.bindings)
        for operand in ins.operands:
            if operand.type != X86_OP_MEM or not operand.access & CS_AC_WRITE:
                continue
            origin = function.memory_origin(ins, function.states[address], operand)
            if origin is None:
                return None
            kind, begin = origin
            if kind == 'image':
                writes.add((begin, begin + operand.size))
            elif begin + operand.size > 0:
                return None
    merged = []
    for begin, end in sorted(writes):
        if merged and begin <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(end, merged[-1][1]))
        else:
            merged.append((begin, end))
    return MemoryEffects(tuple(('image', begin, end) for begin, end in merged), frozenset(bindings))


def analyze(functions):
    if not functions:
        return
    image = functions[0].image
    image.memory_effects.clear()
    infer_requirements(functions)
    by_address = {function.contract['address']: function for function in functions}
    dependencies, callers = {}, {address: set() for address in by_address}
    for address, function in by_address.items():
        dependencies[address] = {target for ins in function.instructions.values()
                                 if (target := direct_target(function, ins)) is not None}
        for target in dependencies[address]:
            callers[target].add(address)
    ready = deque(sorted(address for address, targets in dependencies.items() if not targets))
    analyzed = set()
    while ready:
        address = ready.popleft()
        function = by_address[address]
        function.analyze()
        image.memory_effects[address] = summarize(function)
        analyzed.add(address)
        for caller in sorted(callers[address]):
            dependencies[caller].remove(address)
            if not dependencies[caller]:
                ready.append(caller)
    for address in sorted(by_address.keys() - analyzed):
        by_address[address].analyze()
        image.memory_effects[address] = None


def report(writes):
    if writes is None:
        return {'status': 'unknown'}
    result = {'status': 'inferred', 'image_writes': [{'address': begin, 'size': end - begin} for _, begin, end in writes.writes]}
    if writes.bindings:
        result['required_provider_bindings'] = sorted(writes.bindings)
    return result

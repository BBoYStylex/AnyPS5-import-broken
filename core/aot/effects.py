from collections import deque

from capstone import CS_AC_WRITE
from capstone.x86 import X86_OP_IMM, X86_OP_MEM


def direct_target(function, ins):
    if ins.mnemonic != 'call' and ins.address not in function.tail_calls:
        return None
    operand = ins.operands[0]
    if operand.type == X86_OP_IMM and operand.imm in function.image.functions:
        return operand.imm
    return None


def call_writes(function, ins):
    return function.image.memory_effects.get(direct_target(function, ins))


def summarize(function):
    writes = set()
    for address, ins in function.instructions.items():
        if ins.mnemonic == 'call' or address in function.tail_calls:
            nested = call_writes(function, ins)
            if nested is None:
                return None
            writes.update(nested)
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
    return tuple(merged)


def analyze(functions):
    if not functions:
        return
    image = functions[0].image
    image.memory_effects.clear()
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
    return {'status': 'inferred', 'image_writes': [{'address': begin, 'size': end - begin} for begin, end in writes]}

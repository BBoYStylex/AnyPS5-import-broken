#ifndef CORE_AOT_COMPILER_LLVM_IR_CONSTANTS_H
#define CORE_AOT_COMPILER_LLVM_IR_CONSTANTS_H

#include <cstdint>

namespace llvm {

class Type;

class Value {
public:
    explicit Value(Type* inType = nullptr) : valType(inType) {}
    virtual ~Value() = default;

    Type* getType() const { return valType; }
    void replaceAllUsesWith(Value* newVal) { static_cast<void>(newVal); }
    bool use_empty() const { return true; }

private:
    Type* valType = nullptr;
};

class Constant : public Value {
public:
    using Value::Value;
};

class ConstantInt : public Constant {
public:
    ConstantInt(Type* t, uint64_t v) : Constant(t), val(v) {}
    uint64_t getZExtValue() const { return val; }

    static ConstantInt* get(Type* t, uint64_t v) {
        return new ConstantInt(t, v);
    }

private:
    uint64_t val = 0;
};

template <typename To, typename From>
To* dyn_cast(From* val) {
    return dynamic_cast<To*>(val);
}

template <typename To, typename From>
To* cast(From* val) {
    return dynamic_cast<To*>(val);
}

template <typename To, typename From>
bool isa(From* val) {
    return dynamic_cast<To*>(val) != nullptr;
}

} // namespace llvm
#endif

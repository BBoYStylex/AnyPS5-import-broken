#include <cstddef>
#include "SceTypes.hpp"

namespace {
struct LibreSslStack {
    int count;
    void** data;
    int sorted;
    int capacity;
    int (*compare)(const void*, const void*);
};

static_assert(sizeof(LibreSslStack) == 32);
static_assert(offsetof(LibreSslStack, data) == 8);
static_assert(offsetof(LibreSslStack, compare) == 24);
}

extern "C" {

int APS5_VABI sk_num_nid_postfix(const LibreSslStack* stack) {
    return stack ? stack->count : -1;
}

void* APS5_VABI sk_value_nid_postfix(const LibreSslStack* stack, int index) {
    if (!stack || index < 0 || index >= stack->count) return nullptr;
    return stack->data[index];
}

void* APS5_VABI sk_pop_nid_postfix(LibreSslStack* stack) {
    if (!stack || stack->count <= 0) return nullptr;
    return stack->data[--stack->count];
}

}

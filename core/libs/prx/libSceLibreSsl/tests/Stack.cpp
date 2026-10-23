#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>

struct LibreSslStack {
    int count;
    void** data;
    int sorted;
    int capacity;
    int (*compare)(const void*, const void*);
};

extern "C" {
int APS5_VABI sk_num_nid_postfix(const LibreSslStack* stack);
void* APS5_VABI sk_value_nid_postfix(const LibreSslStack* stack, int index);
void* APS5_VABI sk_pop_nid_postfix(LibreSslStack* stack);
}

static_assert(sizeof(LibreSslStack) == 32);
static_assert(offsetof(LibreSslStack, data) == 8);
static_assert(offsetof(LibreSslStack, compare) == 24);

namespace {
void Require(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "failed: %s\n", message);
    std::abort();
}
}

int main() {
    int first = 1;
    int second = 2;
    int third = 3;
    void* values[] {&first, nullptr, &second, &third};
    LibreSslStack stack {4, values, 1, 4, nullptr};

    Require(sk_num_nid_postfix(nullptr) == -1, "null stack count");
    Require(sk_num_nid_postfix(&stack) == 4, "stack count");
    Require(sk_value_nid_postfix(nullptr, 0) == nullptr, "null stack value");
    Require(sk_value_nid_postfix(&stack, -1) == nullptr, "negative value index");
    Require(sk_value_nid_postfix(&stack, 4) == nullptr, "past-end value index");
    Require(sk_value_nid_postfix(&stack, 0) == &first, "first value");
    Require(sk_value_nid_postfix(&stack, 3) == &third, "last value");

    Require(sk_pop_nid_postfix(nullptr) == nullptr, "null stack pop");
    Require(sk_pop_nid_postfix(&stack) == &third, "first pop");
    Require(stack.count == 3, "first pop decrements count");
    Require(sk_pop_nid_postfix(&stack) == &second, "second pop");
    Require(stack.count == 2, "second pop decrements count");
    Require(sk_pop_nid_postfix(&stack) == nullptr, "null element pop");
    Require(stack.count == 1, "null element pop decrements count");
    Require(sk_pop_nid_postfix(&stack) == &first, "final value pop");
    Require(stack.count == 0, "final value pop decrements count");
    Require(sk_pop_nid_postfix(&stack) == nullptr, "empty stack pop");
    Require(stack.count == 0, "empty stack count remains zero");

}

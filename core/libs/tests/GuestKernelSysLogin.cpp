#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <source_location>

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI getlogin_r_nid_postfix(char* output, int size);
char* APS5_VABI getlogin_nid_postfix(void);
}

namespace {

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "check failed at line %u\n", static_cast<unsigned>(location.line()));
    std::exit(1);
}

int Error() {
    return *__error_nid_postfix();
}

void Getlogin() {
    char expected[33]{};
    const int error = getlogin_r_nid_postfix(expected, static_cast<int>(sizeof(expected)));
    char* name = getlogin_nid_postfix();
    if (error != 0) {
        Require(name == nullptr && Error() == error);
        return;
    }
    if (expected[0] == '\0') {
        Require(name == nullptr);
        return;
    }
    Require(name != nullptr && std::strcmp(name, expected) == 0);
    Require(getlogin_nid_postfix() == name);
}

}

int main() {
    Getlogin();
    std::puts("getlogin passed");
    return 0;
}

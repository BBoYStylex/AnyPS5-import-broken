#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
extern "C" {
char* APS5_VABI getenv_nid_postfix(const char*);
int APS5_VABI setenv_nid_postfix(const char*, const char*, int);
int APS5_VABI unsetenv_nid_postfix(const char*);
int APS5_VABI putenv_nid_postfix(char*);
int* APS5_VABI __error_nid_postfix();
}
static void Require(bool value) { if (!value) std::abort(); }
int main() {
    const char* key = "ANYPS5_GUEST_ENV_TEST_4C27";
#ifdef _WIN32
    Require(_putenv_s(key, "inherited") == 0);
#else
    Require(::setenv(key, "inherited", 1) == 0);
#endif
    Require(std::strcmp(getenv_nid_postfix(key), "inherited") == 0);
    Require(setenv_nid_postfix(key, "kept out", 0) == 0);
    Require(std::strcmp(getenv_nid_postfix(key), "inherited") == 0);
    char value[] = "copied";
    Require(setenv_nid_postfix(key, value, 1) == 0);
    value[0] = 'X';
    Require(std::strcmp(getenv_nid_postfix(key), "copied") == 0);
    Require(std::strcmp(std::getenv(key), "inherited") == 0);
    Require(setenv_nid_postfix(key, "", 1) == 0);
    Require(getenv_nid_postfix(key) && *getenv_nid_postfix(key) == 0);
    char borrowed[] = "ANYPS5_GUEST_ENV_TEST_4C27=one";
    Require(putenv_nid_postfix(borrowed) == 0);
    char* position = std::strchr(borrowed, '=') + 1;
    Require(getenv_nid_postfix(key) == position);
    std::memcpy(position, "two", 3);
    Require(std::strcmp(getenv_nid_postfix(key), "two") == 0);
    Require(setenv_nid_postfix("anyps5_guest_env_test_4c27", "lower", 1) == 0);
    Require(std::strcmp(getenv_nid_postfix(key), "two") == 0);
    Require(setenv_nid_postfix("bad=name", "x", 1) == -1);
    Require(*__error_nid_postfix() == 22);
    const char* const invalidNames[] = {nullptr, "", "bad=name"};
    for (const char* name : invalidNames) {
        *__error_nid_postfix() = 0;
        Require(getenv_nid_postfix(name) == nullptr && *__error_nid_postfix() == 22);
    }
    *__error_nid_postfix() = 34;
    Require(getenv_nid_postfix("ANYPS5_GUEST_ENV_ABSENT_4C27") == nullptr && *__error_nid_postfix() == 34);
    Require(unsetenv_nid_postfix("") == -1);
    char invalid[] = "no-separator";
    Require(putenv_nid_postfix(invalid) == -1);
    Require(unsetenv_nid_postfix(key) == 0);
    Require(getenv_nid_postfix(key) == nullptr);
    Require(unsetenv_nid_postfix(key) == 0);
    Require(unsetenv_nid_postfix("anyps5_guest_env_test_4c27") == 0);
    Require(setenv_nid_postfix("Q7A", "a", 1) == 0);
    Require(setenv_nid_postfix("Q7B", "b", 1) == 0);
    const char* kept = getenv_nid_postfix("Q7B");
    Require(unsetenv_nid_postfix("Q7A") == 0);
    for (int index = 0; index < 64; ++index)
        Require(setenv_nid_postfix(("ANYPS5_GUEST_ENV_GROW_4C27_" + std::to_string(index)).c_str(), "x", 1) == 0);
    Require(getenv_nid_postfix("Q7B") == kept && std::strcmp(kept, "b") == 0);

    const std::string expected(512, 'v');
    Require(setenv_nid_postfix("Q7L", "Q7L=x", 1) == 0);
    char* shortAlias = getenv_nid_postfix("Q7L");
    Require(shortAlias != nullptr && putenv_nid_postfix(shortAlias) == 0);
    Require(setenv_nid_postfix("Q7M", "churn", 1) == 0);
    Require(getenv_nid_postfix("Q7L") == shortAlias + 4 && std::strcmp(shortAlias + 4, "x") == 0);
    Require(unsetenv_nid_postfix("Q7L") == 0 && unsetenv_nid_postfix("Q7M") == 0);
    const auto checkValue = [&](const char* name, char* value) {
        for (int index = 0; index < 8; ++index)
            Require(setenv_nid_postfix("Q7K", std::string(512, 'z').c_str(), 1) == 0);
        if (getenv_nid_postfix(name) != value) std::fputs("putenv lost an aliased environment value\n", stderr);
        Require(getenv_nid_postfix(name) == value);
        Require(std::strcmp(value, expected.c_str()) == 0);
    };
    Require(setenv_nid_postfix("Q7C", ("Q7C=" + expected).c_str(), 1) == 0);
    char* selfAlias = getenv_nid_postfix("Q7C");
    Require(selfAlias != nullptr && putenv_nid_postfix(selfAlias) == 0);
    checkValue("Q7C", selfAlias + 4);
    Require(unsetenv_nid_postfix("Q7C") == 0);

    Require(setenv_nid_postfix("Q7D", ("Q7E=" + expected).c_str(), 1) == 0);
    char* unsetAlias = getenv_nid_postfix("Q7D");
    Require(unsetAlias != nullptr && putenv_nid_postfix(unsetAlias) == 0);
    Require(unsetenv_nid_postfix("Q7D") == 0);
    checkValue("Q7E", unsetAlias + 4);
    Require(unsetenv_nid_postfix("Q7E") == 0);

    Require(setenv_nid_postfix("Q7F", ("Q7G=" + expected).c_str(), 1) == 0);
    char* replaceAlias = getenv_nid_postfix("Q7F");
    Require(replaceAlias != nullptr && putenv_nid_postfix(replaceAlias) == 0);
    Require(setenv_nid_postfix("Q7F", std::string(1024, 'w').c_str(), 1) == 0);
    checkValue("Q7G", replaceAlias + 4);
    Require(unsetenv_nid_postfix("Q7F") == 0 && unsetenv_nid_postfix("Q7G") == 0);

    Require(setenv_nid_postfix("Q7H", ("Q7I=Q7J=" + expected).c_str(), 1) == 0);
    char* retainedAlias = getenv_nid_postfix("Q7H");
    Require(retainedAlias != nullptr && putenv_nid_postfix(retainedAlias) == 0);
    Require(unsetenv_nid_postfix("Q7H") == 0);
    char* nestedAlias = getenv_nid_postfix("Q7I");
    Require(nestedAlias == retainedAlias + 4 && putenv_nid_postfix(nestedAlias) == 0);
    Require(unsetenv_nid_postfix("Q7I") == 0);
    checkValue("Q7J", nestedAlias + 4);
    Require(unsetenv_nid_postfix("Q7J") == 0);
    Require(unsetenv_nid_postfix("Q7K") == 0);
}

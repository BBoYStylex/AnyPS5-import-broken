#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <source_location>
#include <string>
#include <thread>

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI open_nid_postfix(const char* path, int flags, int mode);
int APS5_VABI close_nid_postfix(int descriptor);
int APS5_VABI dup2_nid_postfix(int from, int to);
int APS5_VABI ksem_init_nid_postfix(std::intptr_t* idp, unsigned int value);
int APS5_VABI ksem_post_nid_postfix(std::intptr_t id);
int APS5_VABI ksem_wait_nid_postfix(std::intptr_t id);
int APS5_VABI ksem_trywait_nid_postfix(std::intptr_t id);
int APS5_VABI ksem_timedwait_nid_postfix(std::intptr_t id, const void* abstime);
int APS5_VABI ksem_getvalue_nid_postfix(std::intptr_t id, int* value);
int APS5_VABI ksem_destroy_nid_postfix(std::intptr_t id);
int APS5_VABI ksem_open_nid_postfix(std::intptr_t* idp, const char* name, int oflag, unsigned int mode, unsigned int value);
int APS5_VABI ksem_close_nid_postfix(std::intptr_t id);
int APS5_VABI ksem_unlink_nid_postfix(const char* name);
}

namespace {

constexpr int Enoent = 2;
constexpr int Efault = 14;
constexpr int Eexist = 17;
constexpr int Enametoolong = 63;
constexpr int OCreat = 0x0200;
constexpr int OExcl = 0x0800;
constexpr int Ebusy = 16;
constexpr int Einval = 22;
constexpr int Eagain = 35;
constexpr int Etimedout = 60;
constexpr int Eoverflow = 84;

struct Timespec {
    std::int64_t seconds;
    std::int64_t nanoseconds;
};

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "check failed at line %u\n", static_cast<unsigned>(location.line()));
    std::exit(1);
}

int Error() {
    return *__error_nid_postfix();
}

Timespec FromNow(std::chrono::milliseconds delay) {
    const auto when = std::chrono::system_clock::now() + delay;
    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(when.time_since_epoch()).count();
    return {nanoseconds / 1000000000, nanoseconds % 1000000000};
}

int Value(std::intptr_t id) {
    int value = -1;
    Require(ksem_getvalue_nid_postfix(id, &value) == 0);
    return value;
}

void Counting() {
    std::intptr_t id = -1;
    Require(ksem_init_nid_postfix(&id, 0x80000000u) == -1 && Error() == Einval);
    Require(ksem_init_nid_postfix(nullptr, 1) == -1 && Error() == Efault);
    Require(ksem_init_nid_postfix(&id, 2) == 0 && id >= 0);
    Require(Value(id) == 2);
    Require(ksem_trywait_nid_postfix(id) == 0 && ksem_wait_nid_postfix(id) == 0 && Value(id) == 0);
    Require(ksem_trywait_nid_postfix(id) == -1 && Error() == Eagain);
    Require(ksem_post_nid_postfix(id) == 0 && Value(id) == 1);
    Require(ksem_getvalue_nid_postfix(id, nullptr) == -1 && Error() == Efault);
    Require(ksem_destroy_nid_postfix(id) == 0);
    Require(ksem_post_nid_postfix(id) == -1 && Error() == Einval);
    Require(ksem_init_nid_postfix(&id, 0x7fffffffu) == 0);
    Require(ksem_post_nid_postfix(id) == -1 && Error() == Eoverflow);
    Require(ksem_destroy_nid_postfix(id) == 0);
}

void Timed() {
    std::intptr_t id = -1;
    Require(ksem_init_nid_postfix(&id, 0) == 0);
    const Timespec invalid{0, 1000000000};
    Require(ksem_timedwait_nid_postfix(id, &invalid) == -1 && Error() == Einval);
    const auto deadline = FromNow(std::chrono::milliseconds(150));
    const auto start = std::chrono::steady_clock::now();
    Require(ksem_timedwait_nid_postfix(id, &deadline) == -1 && Error() == Etimedout);
    Require(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(100));
    Require(ksem_post_nid_postfix(id) == 0);
    const auto later = FromNow(std::chrono::seconds(5));
    Require(ksem_timedwait_nid_postfix(id, &later) == 0 && Value(id) == 0);
    Require(ksem_destroy_nid_postfix(id) == 0);
}

void Blocking() {
    std::intptr_t id = -1;
    Require(ksem_init_nid_postfix(&id, 0) == 0);
    std::atomic<int> result{-2};
    std::thread waiter([&] { result = ksem_wait_nid_postfix(id); });
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    Require(result == -2);
    Require(ksem_destroy_nid_postfix(id) == -1 && Error() == Ebusy);
    Require(ksem_post_nid_postfix(id) == 0);
    waiter.join();
    Require(result == 0 && Value(id) == 0);
    Require(ksem_destroy_nid_postfix(id) == 0);
}

void Identifiers() {
    Require(ksem_post_nid_postfix(-1) == -1 && Error() == Einval);
    const auto path = (std::filesystem::temp_directory_path() / "anyps5-ksem-stale.bin").string();
    { std::ofstream stream(path); stream << "x"; }
    const int plain = open_nid_postfix(path.c_str(), 0, 0);
    Require(plain >= 0);
    Require(ksem_post_nid_postfix(plain) == -1 && Error() == Einval);
    Require(close_nid_postfix(plain) == 0);
    std::intptr_t id = -1;
    Require(ksem_init_nid_postfix(&id, 1) == 0);
    Require(close_nid_postfix(static_cast<int>(id)) == 0);
    Require(ksem_post_nid_postfix(id) == -1 && Error() == Einval);
    const int reused = open_nid_postfix(path.c_str(), 0, 0);
    Require(reused >= 0);
    Require(ksem_post_nid_postfix(reused) == -1 && Error() == Einval);
    Require(ksem_getvalue_nid_postfix(reused, nullptr) == -1 && Error() == Einval);
    Require(close_nid_postfix(reused) == 0);
    Require(ksem_init_nid_postfix(&id, 1) == 0);
    const int other = open_nid_postfix(path.c_str(), 0, 0);
    Require(other >= 0 && other != id);
    Require(dup2_nid_postfix(other, static_cast<int>(id)) == static_cast<int>(id));
    Require(ksem_post_nid_postfix(id) == -1 && Error() == Einval);
    Require(ksem_trywait_nid_postfix(id) == -1 && Error() == Einval);
    Require(close_nid_postfix(static_cast<int>(id)) == 0 && close_nid_postfix(other) == 0);
    std::filesystem::remove(path);
}

void Named() {
    const std::string name = "/anyps5-ksem-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::intptr_t first = -1;
    std::intptr_t second = -1;
    Require(ksem_open_nid_postfix(&first, name.c_str(), 0, 0600, 1) == -1 && Error() == Enoent);
    Require(ksem_open_nid_postfix(&first, "relative", OCreat, 0600, 1) == -1 && Error() == Einval);
    Require(ksem_open_nid_postfix(&first, name.c_str(), OCreat | 0x1, 0600, 1) == -1 && Error() == Einval);
    Require(ksem_open_nid_postfix(&first, name.c_str(), OCreat, 0600, 0x80000000u) == -1 && Error() == Einval);
    Require(ksem_open_nid_postfix(&first, nullptr, OCreat, 0600, 1) == -1 && Error() == Efault);
    const std::string longName = "/" + std::string(1100, 'a');
    Require(ksem_open_nid_postfix(&first, longName.c_str(), OCreat, 0600, 1) == -1 && Error() == Enametoolong);
    Require(ksem_open_nid_postfix(&first, name.c_str(), OCreat, 0600, 1) == 0 && first >= 0);
    Require(ksem_open_nid_postfix(&second, name.c_str(), OCreat | OExcl, 0600, 1) == -1 && Error() == Eexist);
    Require(ksem_open_nid_postfix(&second, name.c_str(), OCreat, 0600, 9) == 0 && second >= 0 && second != first);
    Require(Value(second) == 1);
    Require(ksem_post_nid_postfix(first) == 0 && Value(second) == 2);
    Require(ksem_destroy_nid_postfix(first) == -1 && Error() == Einval);
    std::intptr_t unnamed = -1;
    Require(ksem_init_nid_postfix(&unnamed, 0) == 0);
    Require(ksem_close_nid_postfix(unnamed) == -1 && Error() == Einval);
    Require(ksem_destroy_nid_postfix(unnamed) == 0);
    Require(ksem_unlink_nid_postfix(name.c_str()) == 0);
    Require(ksem_unlink_nid_postfix(name.c_str()) == -1 && Error() == Enoent);
    Require(ksem_unlink_nid_postfix(nullptr) == -1 && Error() == Efault);
    std::intptr_t third = -1;
    Require(ksem_open_nid_postfix(&third, name.c_str(), 0, 0600, 0) == -1 && Error() == Enoent);
    Require(ksem_trywait_nid_postfix(first) == 0 && Value(second) == 1);
    Require(ksem_close_nid_postfix(first) == 0);
    Require(ksem_post_nid_postfix(first) == -1 && Error() == Einval);
    Require(Value(second) == 1);
    Require(ksem_close_nid_postfix(second) == 0);
    Require(ksem_open_nid_postfix(&third, name.c_str(), OCreat, 0600, 5) == 0 && Value(third) == 5);
    Require(ksem_close_nid_postfix(third) == 0 && ksem_unlink_nid_postfix(name.c_str()) == 0);
}

}

int main() {
    Counting();
    Timed();
    Blocking();
    Identifiers();
    Named();
    std::puts("ksem unnamed and named semaphores passed");
    return 0;
}

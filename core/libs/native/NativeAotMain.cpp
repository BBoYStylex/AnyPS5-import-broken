#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/sysctl.h>
#include <vector>

extern "C" void guest_start(void*, void (*)());


namespace {

void Teardown() {
    std::fputs("native teardown completed\n", stderr);
}

}

int main(int argc, char** argv) {
    int translated = 0;
    std::size_t bytes = sizeof(translated);
    if (sysctlbyname("sysctl.proc_translated", &translated, &bytes, nullptr, 0) != 0 && errno != ENOENT)
        return 90;
    if (translated != 0) return 91;
    std::fprintf(stderr, "architecture=arm64 translated=%d\n", translated);
    std::vector<std::uintptr_t> parameters(static_cast<std::size_t>(argc) + 2);
    parameters[0] = static_cast<std::uintptr_t>(argc);
    for (int index = 0; index < argc; ++index)
        parameters[index + 1] = reinterpret_cast<std::uintptr_t>(argv[index]);
    guest_start(parameters.data(), Teardown);
    return 92;
}

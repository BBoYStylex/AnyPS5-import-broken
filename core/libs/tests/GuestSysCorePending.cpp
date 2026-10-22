#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

struct Export {
    const char* nid;
    const char* name;
};

const Export Exports[] = {
    {"45owwvXWN+o", "sceApplicationExitSpawn3"},
    {"jPgvaIiVhRU", "sceApplicationKickCoredump3"},
    {"gsItPZhA9-s", "sceApplicationNotifyUnmountForExitSpawn"},
    {"KVWYLwWcpHs", "sceApplicationSendResultOfDebuggerRestoreRequest"},
    {"sMlp8+iJX-U", "sceApplicationSendResultOfDebuggerSaveRequest"},
    {"GEsnTMR3YAA", "sceApplicationSwitchToPprNativeMode"},
};

void* Load(const char* path) {
#ifdef _WIN32
    return LoadLibraryExA(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void* Symbol(void* module, const char* name) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module), name));
#else
    return dlsym(module, name);
#endif
}

bool ReportsNotImplemented(const char* name, void (*call)()) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        return std::string(error.what()) == std::string(name) + " not implemented";
    }
    return false;
}

}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <patched libraries directory>\n", argv[0]);
        return 2;
    }
    std::string path = std::string(argv[1]) + "/libSceSysCore.prx";
    void* module = Load(path.c_str());
    if (!module) {
        std::fprintf(stderr, "%s: the library did not load\n", path.c_str());
        return 1;
    }
    for (const Export& entry : Exports) {
        void* symbol = Symbol(module, entry.nid);
        if (!symbol) {
            std::fprintf(stderr, "%s: the NID %s is not exported\n", entry.name, entry.nid);
            return 1;
        }
        if (!ReportsNotImplemented(entry.name, reinterpret_cast<void (*)()>(symbol))) {
            std::fprintf(stderr, "%s: the export did not report that it is not implemented\n", entry.name);
            return 1;
        }
    }
    std::printf("%zu exports resolve by their NID and throw\n", sizeof(Exports) / sizeof(Exports[0]));
    return 0;
}

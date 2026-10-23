#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <source_location>

struct LibreSslUi;
struct LibreSslUiString;
struct LibreSslUiMethod;
using Opener = int (APS5_VABI *)(LibreSslUi*);
using Writer = int (APS5_VABI *)(LibreSslUi*, LibreSslUiString*);

extern "C" {
LibreSslUiMethod* APS5_VABI UI_create_method_nid_postfix(char*);
void APS5_VABI UI_destroy_method_nid_postfix(LibreSslUiMethod*);
int APS5_VABI UI_method_set_opener_nid_postfix(LibreSslUiMethod*, Opener);
Opener APS5_VABI UI_method_get_opener_nid_postfix(LibreSslUiMethod*);
int APS5_VABI UI_method_set_writer_nid_postfix(LibreSslUiMethod*, Writer);
}

namespace {

struct ReferenceMethod {
    char* name;
    Opener opener;
    Writer writer;
    Opener flusher;
    Writer reader;
    Opener closer;
    char* (APS5_VABI *promptConstructor)(LibreSslUi*, const char*, const char*);
};

static_assert(sizeof(ReferenceMethod) == 56);
static_assert(offsetof(ReferenceMethod, opener) == 8);
static_assert(offsetof(ReferenceMethod, writer) == 16);

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "LibreSSL UI check failed at line %u\n", location.line());
        std::abort();
    }
}

ReferenceMethod ReadMethod(const LibreSslUiMethod* method) {
    ReferenceMethod result;
    std::memcpy(&result, method, sizeof(result));
    return result;
}

LibreSslUiMethod* CreateMethod(const char* name) {
    return UI_create_method_nid_postfix(const_cast<char*>(name));
}

unsigned callbackCalls = 0;
int uiValue = 17;
int stringValue = 23;

int APS5_VABI Open(LibreSslUi* ui) {
    ++callbackCalls;
    Require(ui == reinterpret_cast<LibreSslUi*>(&uiValue));
    return uiValue;
}

int APS5_VABI Reopen(LibreSslUi*) {
    ++callbackCalls;
    return -7;
}

int APS5_VABI Write(LibreSslUi* ui, LibreSslUiString* string) {
    ++callbackCalls;
    Require(ui == reinterpret_cast<LibreSslUi*>(&uiValue));
    Require(string == reinterpret_cast<LibreSslUiString*>(&stringValue));
    return stringValue;
}

int APS5_VABI Rewrite(LibreSslUi*, LibreSslUiString*) {
    ++callbackCalls;
    return -11;
}

#ifdef ANYPS5_UI_ALLOC_FAILURES
bool trackAllocations = false;
int remainingAllocations = -1;
std::array<void*, 4> allocations{};
unsigned frees = 0;

bool FailAllocation() {
    if (!trackAllocations || remainingAllocations < 0) return false;
    return remainingAllocations-- == 0;
}

void* RecordAllocation(void* pointer) {
    if (trackAllocations && pointer != nullptr) {
        for (auto& slot : allocations) {
            if (slot == nullptr) {
                slot = pointer;
                return pointer;
            }
        }
        Require(false);
    }
    return pointer;
}
#endif

}

#ifdef ANYPS5_UI_ALLOC_FAILURES
extern "C" {
void* __real_malloc(std::size_t);
void* __real_calloc(std::size_t, std::size_t);
void __real_free(void*);

void* __wrap_malloc(std::size_t size) {
    return FailAllocation() ? nullptr : RecordAllocation(__real_malloc(size));
}

void* __wrap_calloc(std::size_t count, std::size_t size) {
    return FailAllocation() ? nullptr : RecordAllocation(__real_calloc(count, size));
}

void __wrap_free(void* pointer) {
    if (trackAllocations && pointer != nullptr) {
        bool found = false;
        for (auto& slot : allocations) {
            if (slot == pointer) {
                slot = nullptr;
                ++frees;
                found = true;
                break;
            }
        }
        Require(found);
    }
    __real_free(pointer);
}
}
#endif

int main() {
    Require(UI_method_get_opener_nid_postfix(nullptr) == nullptr);
    Require(UI_method_set_opener_nid_postfix(nullptr, Open) == -1);
    Require(UI_method_set_opener_nid_postfix(nullptr, nullptr) == -1);
    Require(UI_method_set_writer_nid_postfix(nullptr, Write) == -1);
    Require(UI_method_set_writer_nid_postfix(nullptr, nullptr) == -1);
    bool nullDestructionThrew = false;
    try {
        UI_destroy_method_nid_postfix(nullptr);
    } catch (const std::exception&) {
        nullDestructionThrew = true;
    }
    Require(nullDestructionThrew);

    for (const char* name : std::array<const char*, 3>{nullptr, "", "custom method"}) {
        auto* method = CreateMethod(name);
        Require(method != nullptr);
        const auto snapshot = ReadMethod(method);
        Require(name == nullptr ? snapshot.name == nullptr :
            snapshot.name != name && std::strcmp(snapshot.name, name) == 0);
        Require(snapshot.opener == nullptr && snapshot.writer == nullptr && snapshot.flusher == nullptr);
        Require(snapshot.reader == nullptr && snapshot.closer == nullptr && snapshot.promptConstructor == nullptr);
        UI_destroy_method_nid_postfix(method);
    }

    std::array<char, 4097> longName;
    longName.fill('x');
    longName.back() = '\0';
    auto* longMethod = CreateMethod(longName.data());
    Require(longMethod != nullptr);
    Require(std::strcmp(ReadMethod(longMethod).name, longName.data()) == 0);
    UI_destroy_method_nid_postfix(longMethod);

    char name[] = "owned name";
    auto* first = CreateMethod(name);
    auto* second = CreateMethod("second");
    Require(first != nullptr && second != nullptr && first != second);
    name[0] = 'x';
    Require(std::strcmp(ReadMethod(first).name, "owned name") == 0);
    Require(UI_method_set_opener_nid_postfix(first, Open) == 0);
    Require(UI_method_set_writer_nid_postfix(first, Write) == 0);
    Require(callbackCalls == 0);
    Require(UI_method_get_opener_nid_postfix(first) == Open);
    Require(UI_method_get_opener_nid_postfix(second) == nullptr);
    Require(ReadMethod(first).writer == Write && ReadMethod(second).writer == nullptr);
    Require(UI_method_get_opener_nid_postfix(first)(reinterpret_cast<LibreSslUi*>(&uiValue)) == 17);
    Require(ReadMethod(first).writer(reinterpret_cast<LibreSslUi*>(&uiValue), reinterpret_cast<LibreSslUiString*>(&stringValue)) == 23);
    Require(callbackCalls == 2);
    Require(UI_method_set_opener_nid_postfix(first, Reopen) == 0);
    Require(UI_method_get_opener_nid_postfix(first)(nullptr) == -7);
    Require(ReadMethod(first).writer == Write);
    Require(UI_method_set_writer_nid_postfix(first, Rewrite) == 0);
    Require(ReadMethod(first).writer == Rewrite);
    Require(ReadMethod(first).writer(nullptr, nullptr) == -11);
    Require(UI_method_get_opener_nid_postfix(first) == Reopen);
    Require(UI_method_set_opener_nid_postfix(first, nullptr) == 0);
    Require(UI_method_get_opener_nid_postfix(first) == nullptr);
    Require(ReadMethod(first).writer == Rewrite);
    Require(UI_method_set_writer_nid_postfix(first, nullptr) == 0);
    const auto cleared = ReadMethod(first);
    Require(cleared.writer == nullptr && cleared.flusher == nullptr && cleared.reader == nullptr);
    Require(cleared.closer == nullptr && cleared.promptConstructor == nullptr);
    Require(std::strcmp(cleared.name, "owned name") == 0);
    UI_destroy_method_nid_postfix(first);
    Require(UI_method_set_opener_nid_postfix(second, Open) == 0);
    Require(UI_method_get_opener_nid_postfix(second) == Open);
    UI_destroy_method_nid_postfix(second);

    for (unsigned i = 0; i < 256; ++i) {
        auto* method = CreateMethod("reused");
        Require(method != nullptr && UI_method_get_opener_nid_postfix(method) == nullptr);
        Require(UI_method_set_writer_nid_postfix(method, Write) == 0);
        UI_destroy_method_nid_postfix(method);
    }

#ifdef ANYPS5_UI_ALLOC_FAILURES
    trackAllocations = true;
    remainingAllocations = 0;
    frees = 0;
    Require(CreateMethod("allocation failure") == nullptr);
    Require(frees == 0);
    for (auto* allocation : allocations) Require(allocation == nullptr);
    remainingAllocations = 1;
    auto* noName = CreateMethod("name allocation failure");
    Require(noName != nullptr && ReadMethod(noName).name == nullptr);
    Require(UI_method_set_opener_nid_postfix(noName, Open) == 0);
    Require(UI_method_get_opener_nid_postfix(noName) == Open);
    UI_destroy_method_nid_postfix(noName);
    Require(frees == 1);
    for (auto* allocation : allocations) Require(allocation == nullptr);
    frees = 0;
    remainingAllocations = 1;
    auto* unnamed = CreateMethod(nullptr);
    Require(unnamed != nullptr && remainingAllocations == 0);
    UI_destroy_method_nid_postfix(unnamed);
    Require(frees == 1);
    remainingAllocations = -1;
    frees = 0;
    auto* named = CreateMethod("recovered");
    Require(named != nullptr);
    UI_destroy_method_nid_postfix(named);
    Require(frees == 2);
    for (auto* allocation : allocations) Require(allocation == nullptr);
    trackAllocations = false;
#endif
    return 0;
}

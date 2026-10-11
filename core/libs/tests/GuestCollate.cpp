#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/GuestLocale.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" {
extern GuestLocale::Implementation* _ZSt21_sceLibcClassicLocale_nid_postfix;
std::size_t APS5_VABI _ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(GuestLocale::Facet** facet, const GuestLocale::Implementation* const* locale);
std::size_t APS5_VABI _ZNSt7collateIwE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(GuestLocale::Facet** facet, const GuestLocale::Implementation* const* locale);
}

namespace {

std::size_t allocations = 0;
std::size_t frees = 0;
std::size_t lastSize = 0;
void* lastAllocation = nullptr;
void* lastFree = nullptr;

void require(bool condition) {
    if (!condition) std::abort();
}

template<typename TAction>
void reject(TAction action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    require(rejected);
}

void* APS5_VABI allocate(std::size_t bytes) {
    ++allocations;
    lastSize = bytes;
    lastAllocation = std::malloc(bytes);
    return lastAllocation;
}

void APS5_VABI release(void* pointer) {
    ++frees;
    lastFree = pointer;
    std::free(pointer);
}

void* APS5_VABI allocateZeroed(std::size_t, std::size_t) { std::abort(); }
void* APS5_VABI reallocate(void*, std::size_t) { std::abort(); }
void* APS5_VABI align(std::size_t, std::size_t) { std::abort(); }
void* APS5_VABI realign(void*, std::size_t, std::size_t) { std::abort(); }
int APS5_VABI posixAlign(void**, std::size_t, std::size_t) { std::abort(); }

const GuestLocale::CollateVtable& Vtable(const GuestLocale::Facet* facet) {
    return *reinterpret_cast<const GuestLocale::CollateVtable*>(facet->vtable);
}

int Compare(const GuestLocale::CollateFacet* facet, const std::string& left, const std::string& right) {
    return Vtable(&facet->base).compare(facet, left.data(), left.data() + left.size(), right.data(), right.data() + right.size());
}

std::uint64_t Hash(const GuestLocale::CollateFacet* facet, const std::string& text) {
    return static_cast<std::uint64_t>(Vtable(&facet->base).hash(facet, text.data(), text.data() + text.size()));
}

const GuestLocale::WideCollateVtable& WideVtable(const GuestLocale::Facet* facet) {
    return *reinterpret_cast<const GuestLocale::WideCollateVtable*>(facet->vtable);
}

int WideCompare(const GuestLocale::CollateFacet* facet, const std::u16string& left, const std::u16string& right) {
    return WideVtable(&facet->base).compare(facet, left.data(), left.data() + left.size(), right.data(), right.data() + right.size());
}

std::uint64_t WideHash(const GuestLocale::CollateFacet* facet, const std::u16string& text) {
    return static_cast<std::uint64_t>(WideVtable(&facet->base).hash(facet, text.data(), text.data() + text.size()));
}

std::uint64_t Fnv1a(const std::string& bytes) {
    std::uint64_t hash = 0xcbf29ce484222325ull;
    for (const char byte : bytes) hash = (hash ^ static_cast<unsigned char>(byte)) * 0x100000001b3ull;
    return hash;
}

void TestWideCollate(const GuestLocale::Implementation* const* classic, const GuestLocale::Implementation* const* french) {
    const auto getcat = _ZNSt7collateIwE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix;
    const std::size_t baseAllocations = allocations;
    const std::size_t baseFrees = frees;
    require(getcat(nullptr, classic) == 1);
    GuestLocale::Facet existing{};
    GuestLocale::Facet* facet = &existing;
    require(getcat(&facet, classic) == 1);
    require(facet == &existing && allocations == baseAllocations);

    facet = nullptr;
    reject([&] { getcat(&facet, french); });
    require(facet == nullptr && allocations == baseAllocations);

    require(getcat(&facet, classic) == 1);
    require(facet != nullptr && allocations == baseAllocations + 1 && lastSize == sizeof(GuestLocale::CollateFacet) && lastAllocation == facet);
    const auto* collate = reinterpret_cast<const GuestLocale::CollateFacet*>(facet);
    require(facet->references == 0 && collate->collation == nullptr && collate->wideCollation == nullptr);

    require(WideCompare(collate, u"abc", u"abd") == -1);
    require(WideCompare(collate, u"abd", u"abc") == 1);
    require(WideCompare(collate, u"abc", u"abc") == 0);
    require(WideCompare(collate, u"ab", u"abc") == -1);
    require(WideCompare(collate, u"abc", u"ab") == 1);
    require(WideCompare(collate, u"", u"") == 0);
    require(WideCompare(collate, u"", u"a") == -1);
    require(WideCompare(collate, u"\u00e9", u"z") == 1);
    require(WideCompare(collate, u"\uffff", u"\u0100") == 1);
    require(WideCompare(collate, u"\u0100", u"\u00ff") == 1);
    require(WideCompare(collate, std::u16string(u"a\0b", 3), std::u16string(u"a\0c", 3)) == -1);
    require(WideVtable(facet).compare(collate, nullptr, nullptr, nullptr, nullptr) == 0);
    const char16_t text[] = u"abc";
    reject([&] { WideVtable(facet).compare(collate, text + 2, text, text, text + 1); });
    reject([&] { WideVtable(facet).compare(collate, nullptr, text, text, text + 1); });

    require(WideHash(collate, u"") == 0xcbf29ce484222325ull);
    require(WideHash(collate, u"a") == Fnv1a(std::string("a\0", 2)));
    require(WideHash(collate, u"foobar") == Fnv1a(std::string("f\0o\0o\0b\0a\0r\0", 12)));
    require(WideHash(collate, u"\u1234") == Fnv1a("\x34\x12"));
    reject([&] { WideVtable(facet).hash(collate, text + 1, text); });

    GuestLocale::WideString result;
    std::memset(&result, 0xcd, sizeof(result));
    const std::u16string shortText = u"hello";
    require(WideVtable(facet).transform(&result, collate, shortText.data(), shortText.data() + shortText.size()) == &result);
    require(result.reserved == 0xcdcdcdcdcdcdcdcdull && result.size == 5 && result.capacity == 7 && std::u16string(result.buffer) == shortText);
    require(allocations == baseAllocations + 1);

    std::memset(&result, 0xcd, sizeof(result));
    require(WideVtable(facet).transform(&result, collate, nullptr, nullptr) == &result);
    require(result.size == 0 && result.capacity == 7 && result.buffer[0] == 0);

    const std::u16string seven(7, u'q');
    require(WideVtable(facet).transform(&result, collate, seven.data(), seven.data() + seven.size()) == &result);
    require(result.size == 7 && result.capacity == 7 && std::u16string(result.buffer) == seven && allocations == baseAllocations + 1);

    const std::u16string eight(8, u'\u00e9');
    require(WideVtable(facet).transform(&result, collate, eight.data(), eight.data() + eight.size()) == &result);
    require(allocations == baseAllocations + 2 && lastSize == 18 && result.pointer == lastAllocation);
    require(result.size == 8 && result.capacity == 8 && std::u16string(result.pointer) == eight);
    std::free(result.pointer);

    const std::u16string largest(2046, u'z');
    require(WideVtable(facet).transform(&result, collate, largest.data(), largest.data() + largest.size()) == &result);
    require(allocations == baseAllocations + 3 && lastSize == 4094 && result.capacity == 2046 && std::u16string(result.pointer) == largest);
    std::free(result.pointer);

    const std::u16string tooLong(2047, u'z');
    reject([&] { WideVtable(facet).transform(&result, collate, tooLong.data(), tooLong.data() + tooLong.size()); });
    const std::u16string embedded(u"a\0b", 3);
    reject([&] { WideVtable(facet).transform(&result, collate, embedded.data(), embedded.data() + embedded.size()); });
    reject([&] { WideVtable(facet).transform(nullptr, collate, text, text + 1); });
    require(allocations == baseAllocations + 3);

    WideVtable(facet).facet.retain(facet);
    require(facet->references == 1);
    require(WideVtable(facet).facet.release(facet) == facet && facet->references == 0);
    WideVtable(facet).facet.destroy(facet);
    require(frees == baseFrees);
    WideVtable(facet).facet.deleteObject(facet);
    require(frees == baseFrees + 1 && lastFree == collate);
}

}

int main() {
    const std::array<void*, 10> api{reinterpret_cast<void*>(&allocate), reinterpret_cast<void*>(&release), reinterpret_cast<void*>(&allocateZeroed), reinterpret_cast<void*>(&reallocate), reinterpret_cast<void*>(&align), reinterpret_cast<void*>(&realign), reinterpret_cast<void*>(&posixAlign)};
    ApplicationHeapRegister_nid_no_patch(api.data());

    const GuestLocale::Implementation* const* classic = &_ZSt21_sceLibcClassicLocale_nid_postfix;
    require(_ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(nullptr, classic) == 1);
    GuestLocale::Facet existing{};
    GuestLocale::Facet* facet = &existing;
    require(_ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(&facet, classic) == 1);
    require(facet == &existing && allocations == 0);

    GuestLocale::Implementation french = *_ZSt21_sceLibcClassicLocale_nid_postfix;
    french.name = "fr_FR";
    const GuestLocale::Implementation* frenchPointer = &french;
    facet = nullptr;
    reject([&] { _ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(&facet, &frenchPointer); });
    require(facet == nullptr && allocations == 0);

    require(_ZNSt7collateIcE7_GetcatEPPKNSt6locale5facetEPKS1__nid_postfix(&facet, classic) == 1);
    require(facet != nullptr && allocations == 1 && lastSize == sizeof(GuestLocale::CollateFacet) && lastAllocation == facet);
    const auto* collate = reinterpret_cast<const GuestLocale::CollateFacet*>(facet);
    require(facet->references == 0 && collate->collation == nullptr && collate->wideCollation == nullptr);

    require(Compare(collate, "abc", "abd") == -1);
    require(Compare(collate, "abd", "abc") == 1);
    require(Compare(collate, "a", "c") == -1);
    require(Compare(collate, "abc", "abc") == 0);
    require(Compare(collate, "ab", "abc") == -1);
    require(Compare(collate, "abc", "ab") == 1);
    require(Compare(collate, "", "") == 0);
    require(Compare(collate, "", "a") == -1);
    require(Compare(collate, "\x80", "a") == 1);
    require(Compare(collate, std::string("a\0", 2), "a") == 1);
    require(Compare(collate, std::string("a\0b", 3), std::string("a\0c", 3)) == -1);
    require(Vtable(facet).compare(collate, nullptr, nullptr, nullptr, nullptr) == 0);
    const char text[] = "abc";
    reject([&] { Vtable(facet).compare(collate, text + 2, text, text, text + 1); });
    reject([&] { Vtable(facet).compare(collate, nullptr, text, text, text + 1); });

    require(Hash(collate, "") == 0xcbf29ce484222325ull);
    require(Hash(collate, "a") == 0xaf63dc4c8601ec8cull);
    require(Hash(collate, "foobar") == 0x85944171f73967e8ull);
    require(Hash(collate, "\xff") == ((0xcbf29ce484222325ull ^ 0xffull) * 0x100000001b3ull));
    reject([&] { Vtable(facet).hash(collate, text + 1, text); });

    GuestLocale::String result;
    std::memset(&result, 0xcd, sizeof(result));
    const std::string shortText = "hello";
    require(Vtable(facet).transform(&result, collate, shortText.data(), shortText.data() + shortText.size()) == &result);
    require(result.reserved == 0xcdcdcdcdcdcdcdcdull && result.size == 5 && result.capacity == 15 && std::strcmp(result.buffer, "hello") == 0);
    require(allocations == 1);

    std::memset(&result, 0xcd, sizeof(result));
    require(Vtable(facet).transform(&result, collate, nullptr, nullptr) == &result);
    require(result.size == 0 && result.capacity == 15 && result.buffer[0] == 0);

    const std::string fifteen(15, 'q');
    require(Vtable(facet).transform(&result, collate, fifteen.data(), fifteen.data() + fifteen.size()) == &result);
    require(result.size == 15 && result.capacity == 15 && std::string(result.buffer) == fifteen && allocations == 1);

    const std::string longText = "collation of a longer text";
    require(Vtable(facet).transform(&result, collate, longText.data(), longText.data() + longText.size()) == &result);
    require(allocations == 2 && lastSize == longText.size() + 1 && result.pointer == lastAllocation);
    require(result.size == longText.size() && result.capacity == longText.size() && std::string(result.pointer) == longText);
    std::free(result.pointer);

    const std::string largest(4094, 'z');
    require(Vtable(facet).transform(&result, collate, largest.data(), largest.data() + largest.size()) == &result);
    require(allocations == 3 && lastSize == 4095 && result.capacity == 4094 && std::string(result.pointer) == largest);
    std::free(result.pointer);

    const std::string tooLong(4095, 'z');
    reject([&] { Vtable(facet).transform(&result, collate, tooLong.data(), tooLong.data() + tooLong.size()); });
    const std::string embedded("a\0b", 3);
    reject([&] { Vtable(facet).transform(&result, collate, embedded.data(), embedded.data() + embedded.size()); });
    reject([&] { Vtable(facet).transform(nullptr, collate, text, text + 1); });
    require(allocations == 3);

    Vtable(facet).facet.retain(facet);
    Vtable(facet).facet.retain(facet);
    require(facet->references == 2);
    require(Vtable(facet).facet.release(facet) == nullptr && facet->references == 1);
    require(Vtable(facet).facet.release(facet) == facet && facet->references == 0);
    Vtable(facet).facet.destroy(facet);
    require(frees == 0);
    Vtable(facet).facet.deleteObject(facet);
    require(frees == 1 && lastFree == collate);

    TestWideCollate(classic, &frenchPointer);
    return 0;
}

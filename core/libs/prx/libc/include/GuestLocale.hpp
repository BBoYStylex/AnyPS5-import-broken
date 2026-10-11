#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_GUESTLOCALE_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_GUESTLOCALE_HPP

#include <cstddef>
#include <cstdint>
#include "general/VabiMacros.hpp"

namespace GuestLocale {

struct LocinfoStorage {
    alignas(std::uint64_t) unsigned char bytes[64];
};

struct Facet;

struct FacetVtable {
    void (APS5_VABI *destroy)(Facet* self);
    void (APS5_VABI *deleteObject)(Facet* self);
    void (APS5_VABI *retain)(Facet* self);
    Facet* (APS5_VABI *release)(Facet* self);
};

struct Facet {
    const FacetVtable* vtable;
    std::uint32_t references;
    std::uint32_t reserved;
};

struct Implementation {
    Facet base;
    Facet** facets;
    std::uint64_t facetCount;
    std::uint32_t category;
    bool transparent;
    const char* name;
};

template<typename TChar>
struct BasicString {
    static constexpr std::size_t InlineCapacity = 16 / sizeof(TChar) - 1;
    std::uint64_t reserved;
    union {
        TChar buffer[InlineCapacity + 1];
        TChar* pointer;
    };
    std::uint64_t size;
    std::uint64_t capacity;
};

using String = BasicString<char>;
using WideString = BasicString<char16_t>;

struct CollateFacet {
    Facet base;
    const void* collation;
    const void* wideCollation;
};

template<typename TChar>
struct BasicCollateVtable {
    FacetVtable facet;
    int (APS5_VABI *compare)(const CollateFacet* self, const TChar* first1, const TChar* last1, const TChar* first2, const TChar* last2);
    BasicString<TChar>* (APS5_VABI *transform)(BasicString<TChar>* result, const CollateFacet* self, const TChar* first, const TChar* last);
    std::int64_t (APS5_VABI *hash)(const CollateFacet* self, const TChar* first, const TChar* last);
};

using CollateVtable = BasicCollateVtable<char>;
using WideCollateVtable = BasicCollateVtable<char16_t>;

struct IosBase {
    const void* vtable;
    std::int64_t standardStream;
    std::int32_t state;
    std::int32_t exceptions;
    std::int32_t flags;
    std::int32_t precision;
    std::int32_t width;
    std::uint32_t reserved;
    void* storage;
    void* callbacks;
    Implementation** locale;
};

static_assert(sizeof(Facet) == 0x10);
static_assert(offsetof(Implementation, facets) == 0x10);
static_assert(offsetof(Implementation, facetCount) == 0x18);
static_assert(offsetof(Implementation, transparent) == 0x24);
static_assert(offsetof(Implementation, name) == 0x28);
static_assert(offsetof(IosBase, state) == 0x10);
static_assert(offsetof(IosBase, storage) == 0x28);
static_assert(offsetof(IosBase, callbacks) == 0x30);
static_assert(offsetof(IosBase, locale) == 0x38);
static_assert(sizeof(IosBase) == 0x40);
static_assert(offsetof(String, buffer) == 0x8);
static_assert(offsetof(String, size) == 0x18);
static_assert(sizeof(String) == 0x28);
static_assert(String::InlineCapacity == 15);
static_assert(offsetof(WideString, buffer) == 0x8);
static_assert(offsetof(WideString, size) == 0x18);
static_assert(sizeof(WideString) == 0x28);
static_assert(WideString::InlineCapacity == 7);
static_assert(offsetof(CollateFacet, collation) == 0x10);
static_assert(sizeof(CollateFacet) == 0x20);
static_assert(offsetof(CollateVtable, compare) == 0x20);
static_assert(offsetof(CollateVtable, hash) == 0x30);
static_assert(offsetof(WideCollateVtable, compare) == 0x20);
static_assert(offsetof(WideCollateVtable, hash) == 0x30);
static_assert(alignof(LocinfoStorage) == 8);

}

#endif

#ifndef CORE_LIBS_PRX_LIBSCEAMPR_INCLUDE_AMPRDATAOBJECTS_HPP
#define CORE_LIBS_PRX_LIBSCEAMPR_INCLUDE_AMPRDATAOBJECTS_HPP

#include <cstddef>
#include <cstdint>

namespace AmprDataObjects {

constexpr std::size_t FacetIdBytes = sizeof(std::size_t);
constexpr std::size_t NumGetVtableSlots = 15;
constexpr std::size_t NumGetVtableBytes = NumGetVtableSlots * sizeof(std::uintptr_t);

}

extern "C" {
extern std::size_t _ZNSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE_nid_postfix;
extern std::uintptr_t _ZTVSt7num_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE_nid_postfix[AmprDataObjects::NumGetVtableSlots];
extern std::size_t _ZNSt8time_getIcSt19istreambuf_iteratorIcSt11char_traitsIcEEE2idE_nid_postfix;
}

#endif

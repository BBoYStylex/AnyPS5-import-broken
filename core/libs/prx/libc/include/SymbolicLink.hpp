#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_SYMBOLICLINK_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_SYMBOLICLINK_HPP

#include <filesystem>

#ifdef _WIN32
extern "C" bool IsSymbolicLink_nid_no_patch(const std::filesystem::path& host);
#endif

#endif

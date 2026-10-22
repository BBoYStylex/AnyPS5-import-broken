#ifndef CORE_LIBS_PRX_LIBKERNEL_FILE_TRAILINGSLASH_HPP
#define CORE_LIBS_PRX_LIBKERNEL_FILE_TRAILINGSLASH_HPP

#include <cstring>
#include <filesystem>
#include <string>
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/FilesystemError.hpp"

inline int TrailingSlashError_nid_no_patch(const char* path, bool create, bool module = false, bool archived = false) {
    if (path == nullptr || *path == '\0') return 0;
#ifdef _WIN32
    const std::size_t length = std::strlen(path);
    if (length == 0 || (path[length - 1] != '/' && path[length - 1] != '\\')) return 0;
    std::size_t end = length;
    while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\')) --end;
    if (end == 1 && (path[0] == '/' || path[0] == '\\')) return 0;
    const std::string trimmed(path, end);
    std::filesystem::path native;
    if (ResolvePathError_nid_no_patch(trimmed.c_str(), native, module, archived) != 0) return 0;
    std::error_code statusError;
    const auto status = std::filesystem::status(native, statusError);
    if (statusError) return create ? 2 : 0;
    return std::filesystem::is_directory(status) ? 0 : 20;
#else
    (void)create;
    (void)module;
    (void)archived;
    return 0;
#endif
}

#endif

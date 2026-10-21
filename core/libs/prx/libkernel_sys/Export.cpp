#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <sys/stat.h>
#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/FilesystemError.hpp"
#include "prx/libkernel/File/include/DirectoryDescriptor.hpp"
#include "prx/libc/include/CreationMask.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <sys/stat.h>
#include "prx/libkernel/File/include/WindowsSymbolicLink.hpp"
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif
#endif

extern "C" int* APS5_VABI __error_nid_postfix();

namespace {
#ifdef _WIN32
extern "C" _invalid_parameter_handler _set_thread_local_invalid_parameter_handler(_invalid_parameter_handler);
static void IgnoreInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, std::uintptr_t) {}
static int GuestFstat(int fd) {
    const auto previous = _set_thread_local_invalid_parameter_handler(IgnoreInvalidParameter);
    struct _stat64 status{};
    const int result = ::_fstat64(fd, &status);
    _set_thread_local_invalid_parameter_handler(previous);
    return result;
}
#endif

constexpr int GuestEnoent = 2;
constexpr int GuestEperm = 1;
constexpr int GuestEio = 5;
constexpr int GuestEbadf = 9;
constexpr int GuestEfault = 14;
constexpr int GuestEnotdir = 20;
constexpr int GuestEisdir = 21;
constexpr int GuestEinval = 22;
constexpr int GuestEexist = 17;
constexpr int GuestExdev = 18;
constexpr int GuestEdeadlk = 11;
constexpr int GuestEagain = 35;
constexpr int GuestEloop = 62;
constexpr int GuestEnametoolong = 63;
constexpr int GuestEnotempty = 66;
constexpr int GuestEoverflow = 84;
constexpr int GuestAtFdcwd = -100;
constexpr int GuestAtRemovedir = 0x800;
constexpr std::size_t GuestPathMax = 1024;
constexpr std::size_t GuestNameMax = 255;

constexpr int PcLinkMax = 1;
constexpr int PcMaxCanon = 2;
constexpr int PcMaxInput = 3;
constexpr int PcNameMax = 4;
constexpr int PcPathMax = 5;
constexpr int PcPipeBuf = 6;
constexpr int PcChownRestricted = 7;
constexpr int PcNoTrunc = 8;
constexpr int PcVdisable = 9;
constexpr int PcAllocSizeMin = 10;
constexpr int PcFileSizeBits = 12;
constexpr int PcRecIncrXferSize = 14;
constexpr int PcRecMaxXferSize = 15;
constexpr int PcRecMinXferSize = 16;
constexpr int PcRecXferAlign = 17;
constexpr int PcSymlinkMax = 18;
constexpr int PcMinHoleSize = 21;
constexpr int PcAsyncIo = 53;
constexpr int PcPrioIo = 54;
constexpr int PcSyncIo = 55;
constexpr int PcAclExtended = 59;
constexpr int PcAclPathMax = 60;
constexpr int PcMacPresent = 63;
constexpr int PcAclNfs4 = 64;
constexpr int GuestAtSymlinkNofollow = 0x0200;

int GuestFileError(int error) {
    switch (error) {
    case ENOENT: return GuestEnoent;
    case EACCES: return 13;
    case ENOTDIR: return 20;
    case EINVAL: return GuestEinval;
    case EPERM: return GuestEperm;
    case EROFS: return 30;
    case ENAMETOOLONG: return GuestEnametoolong;
    case EIO: return GuestEio;
#ifdef ELOOP
    case ELOOP: return GuestEloop;
#endif
    default: return GuestEio;
    }
}

std::int64_t Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

int GuestError(const std::error_code& error) {
    const auto condition = error.default_error_condition();
    if (condition == std::errc::resource_unavailable_try_again) return GuestEagain;
    if (condition == std::errc::resource_deadlock_would_occur) return GuestEdeadlk;
    if (condition == std::errc::too_many_symbolic_link_levels) return GuestEloop;
    if (condition == std::errc::filename_too_long) return GuestEnametoolong;
    if (condition == std::errc::directory_not_empty) return GuestEnotempty;
    if (condition == std::errc::value_too_large) return GuestEoverflow;
    const int value = condition.value();
    return value > 0 && value <= 34 ? value : GuestEio;
}

std::int64_t CopyLinkTarget(const std::string& target, char* buffer, std::size_t size) {
    const auto count = std::min(size, target.size());
    if (count != 0 && buffer == nullptr) return Fail(GuestEfault);
    if (count != 0) std::memcpy(buffer, target.data(), count);
    return static_cast<std::int64_t>(count);
}

bool HasNonDirectoryPrefix(const std::filesystem::path& native) {
    std::error_code error;
    for (auto prefix = native.parent_path(); prefix.has_relative_path(); prefix = prefix.parent_path()) {
        const auto status = std::filesystem::status(prefix, error);
        if (std::filesystem::exists(status)) return !std::filesystem::is_directory(status);
    }
    return false;
}

std::int64_t ReadNativeLink(const std::filesystem::path& native, char* buffer, std::size_t size) {
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(native.wstring().c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        std::string target;
        if (!File::WindowsSymbolicLink::Read(native, target, "readlink") && !File::WindowsSymbolicLink::ReadJunction(native, target, "readlink")) {
            NotImplemented_nid_no_patch("readlink of a Windows reparse point other than a symbolic link");
            return -1;
        }
        return CopyLinkTarget(target, buffer, size);
    }
#endif
    std::error_code error;
    const auto status = std::filesystem::symlink_status(native, error);
    if (error)
        return Fail(error == std::errc::no_such_file_or_directory && HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestError(error));
    if (status.type() == std::filesystem::file_type::not_found)
        return Fail(HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestEnoent);
    if (!std::filesystem::is_symlink(status)) return Fail(GuestEinval);
    const auto target = std::filesystem::read_symlink(native, error).generic_string();
    if (error) return Fail(GuestError(error));
    return CopyLinkTarget(target, buffer, size);
}

bool HasLongComponent(std::string_view path) {
    for (std::size_t start = 0; start < path.size();) {
        const auto end = std::min(path.find('/', start), path.size());
        if (end - start > GuestNameMax) {
            if (start == 0) return true;
            std::filesystem::path prefix;
            if (ResolvePathError_nid_no_patch(std::string(path.substr(0, start)).c_str(), prefix)) return false;
            std::error_code error;
            return std::filesystem::is_directory(prefix, error);
        }
        start = end + 1;
    }
    return false;
}

bool IsDotOrDotDot(std::string_view path) {
    while (!path.empty() && (path.back() == '/' || path.back() == '\\')) path.remove_suffix(1);
    const auto separator = path.find_last_of("/\\");
    const auto component = path.substr(separator == std::string_view::npos ? 0 : separator + 1);
    return component == "." || component == "..";
}

int RenamePath(const std::filesystem::path& source, const std::filesystem::path& destination) {
    namespace fs = std::filesystem;
    std::error_code error;
    const auto sourceStatus = fs::symlink_status(source, error);
    if (error && error != std::errc::no_such_file_or_directory) return FilesystemError(error);
    if (sourceStatus.type() == fs::file_type::not_found)
        return HasNonDirectoryPrefix(source) ? GuestEnotdir : GuestEnoent;

    const auto parent = destination.parent_path().empty() ? fs::path(".") : destination.parent_path();
    error.clear();
    const auto parentStatus = fs::status(parent, error);
    if (error) return FilesystemError(error);
    if (!fs::exists(parentStatus))
        return HasNonDirectoryPrefix(destination) ? GuestEnotdir : GuestEnoent;
    if (!fs::is_directory(parentStatus)) return GuestEnotdir;

    error.clear();
    const auto destinationStatus = fs::symlink_status(destination, error);
    if (error && error != std::errc::no_such_file_or_directory) return FilesystemError(error);
    const bool destinationExists = destinationStatus.type() != fs::file_type::not_found;
    const bool sourceIsDirectory = fs::is_directory(sourceStatus);
    const bool destinationIsDirectory = destinationExists && fs::is_directory(destinationStatus);
    bool sameFile = false;
    if (destinationExists) {
        error.clear();
        sameFile = fs::equivalent(source, destination, error);
        if (error == std::errc::no_such_file_or_directory) error.clear();
        if (error) return FilesystemError(error);
        if (!sameFile && sourceIsDirectory && !destinationIsDirectory) return GuestEnotdir;
        if (!sameFile && !sourceIsDirectory && destinationIsDirectory) return GuestEisdir;
    }
    if (sourceIsDirectory && !sameFile) {
        error.clear();
        const auto canonicalSource = fs::weakly_canonical(source, error);
        if (error) return FilesystemError(error);
        const auto canonicalDestination = fs::weakly_canonical(destination, error);
        if (error) return FilesystemError(error);
        const auto relative = canonicalDestination.lexically_relative(canonicalSource);
        if (!relative.empty() && *relative.begin() != "..") return GuestEinval;
    }
    if (destinationExists && !sameFile && destinationIsDirectory) {
        error.clear();
        if (!fs::is_empty(destination, error)) {
            return error ? FilesystemError(error) : GuestEnotempty;
        }
    }
#ifdef _WIN32
    if (!sameFile && sourceIsDirectory && destinationIsDirectory) {
        error.clear();
        if (!fs::remove(destination, error)) return error ? FilesystemError(error) : GuestEio;
    }
#endif

    error.clear();
    fs::rename(source, destination, error);
    return error ? FilesystemError(error) : 0;
}
}

extern "C" {

int APS5_VABI fork_nid_postfix(void) {
    NotImplemented_nid_no_patch("fork: child processes");
    return -1;
}

int APS5_VABI setsid_nid_postfix(void) {
    return static_cast<int>(Fail(GuestEperm));
}

std::uint16_t APS5_VABI umask_nid_postfix(std::uint16_t mask) {
    return File::ExchangeCreationMask_nid_no_patch(mask);
}

int APS5_VABI _fstatfs_nid_postfix(int d, void* buf);

int APS5_VABI fstatfs_nid_postfix(int d, void* buf) {
    return _fstatfs_nid_postfix(d, buf);
}

std::int64_t APS5_VABI readlink_nid_postfix(const char* path, char* buffer, std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) return Fail(GuestEinval);
    if (path == nullptr) return Fail(GuestEfault);
    if (*path == '\0') return Fail(GuestEnoent);
    std::filesystem::path native;
    if (const int error = ResolvePathError_nid_no_patch(path, native)) return Fail(error);
    return ReadNativeLink(native, buffer, size);
}

std::int64_t APS5_VABI readlinkat_nid_postfix(int fd, const char* path, char* buffer, std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) return Fail(GuestEinval);
    if (path == nullptr) return Fail(GuestEfault);
    if (*path == '\0') return Fail(GuestEnoent);
    if (fd == GuestAtFdcwd || *path == '/') return readlink_nid_postfix(path, buffer, size);
    int error = 0;
    const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (base == nullptr) return Fail(error);
    return ReadNativeLink(std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                          std::filesystem::path(reinterpret_cast<const char8_t*>(path)), buffer, size);
}

std::int64_t APS5_VABI pathconf_nid_postfix(const char* path, int name) {
    if (path == nullptr) return Fail(GuestEfault);
    if (*path == '\0') return Fail(GuestEnoent);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return Fail(GuestEnametoolong);
    std::filesystem::path native;
    if (const int error = ResolvePathError_nid_no_patch(path, native)) return Fail(error);
    std::error_code error;
    const auto status = std::filesystem::status(native, error);
    if (error) return Fail(GuestError(error));
    if (!std::filesystem::exists(status)) return Fail(GuestEnoent);
    switch (name) {
    case PcLinkMax: return 32767;
    case PcNameMax: return static_cast<std::int64_t>(GuestNameMax);
    case PcPathMax: return static_cast<std::int64_t>(GuestPathMax);
    case PcPipeBuf: return 512;
    case PcChownRestricted: return 1;
    case PcNoTrunc: return 1;
    case PcFileSizeBits: return 64;
    case PcRecMaxXferSize: return -1;
    case PcSymlinkMax: return static_cast<std::int64_t>(GuestPathMax);
    case PcPrioIo: return 0;
    case PcSyncIo: return 0;
    case PcMaxCanon:
    case PcMaxInput:
    case PcVdisable:
    case PcAllocSizeMin:
    case PcRecIncrXferSize:
    case PcRecMinXferSize:
    case PcRecXferAlign:
    case PcMinHoleSize:
    case PcAsyncIo:
    case PcAclExtended:
    case PcAclPathMax:
    case PcMacPresent:
    case PcAclNfs4:
        throw std::runtime_error("pathconf: name " + std::to_string(name) + " depends on the console filesystem or kernel configuration");
    default: return Fail(GuestEinval);
    }
}

int APS5_VABI link_nid_postfix(const char* existing, const char* created) {
    if (existing == nullptr || created == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*existing == '\0' || *created == '\0') return static_cast<int>(Fail(GuestEnoent));
    std::filesystem::path source, destination;
    if (const int error = ResolvePathError_nid_no_patch(existing, source)) return static_cast<int>(Fail(error));
    if (const int error = ResolvePathError_nid_no_patch(created, destination)) return static_cast<int>(Fail(error));
    std::error_code error;
    const auto status = std::filesystem::status(source, error);
    if (!std::filesystem::exists(status)) return static_cast<int>(Fail(GuestEnoent));
    if (std::filesystem::is_directory(status)) return static_cast<int>(Fail(GuestEperm));
    if (std::filesystem::exists(std::filesystem::symlink_status(destination, error))) return static_cast<int>(Fail(GuestEexist));
    if (!std::filesystem::is_directory(destination.parent_path(), error)) return static_cast<int>(Fail(GuestEnoent));
    const auto target = std::filesystem::is_symlink(source, error) ? std::filesystem::canonical(source, error) : source;
    if (!error) std::filesystem::create_hard_link(target, destination, error);
    if (error == std::errc::cross_device_link) return static_cast<int>(Fail(GuestExdev));
    if (error) throw std::system_error(error, "link");
    RecordWrittenPath_nid_no_patch(destination);
    return 0;
}

int APS5_VABI fchdir_nid_postfix(int fd) {
    int error = 0;
    const char* path = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (path == nullptr) return static_cast<int>(Fail(error));
#ifndef _WIN32
    if (::faccessat(fd, ".", X_OK, AT_EACCESS) != 0) return static_cast<int>(Fail(errno));
#endif
    error = SetWorkingDirectory_nid_no_patch(path);
    if (error != 0) return static_cast<int>(Fail(error));
    return 0;
}

int APS5_VABI chroot_nid_postfix(const char* path) {
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return static_cast<int>(Fail(GuestEnametoolong));
    std::filesystem::path native;
    if (const int error = ResolvePathError_nid_no_patch(path, native)) return static_cast<int>(Fail(error));
    std::error_code error;
    const auto status = std::filesystem::status(native, error);
    if (status.type() == std::filesystem::file_type::not_found) return static_cast<int>(Fail(HasNonDirectoryPrefix(native) ? GuestEnotdir : GuestEnoent));
    if (error) return static_cast<int>(Fail(GuestError(error)));
    if (!std::filesystem::is_directory(status)) return static_cast<int>(Fail(GuestEnotdir));
    const bool root = std::filesystem::equivalent(native, ResolvePath_nid_no_patch("/"), error);
    if (error) return static_cast<int>(Fail(GuestError(error)));
    if (root) return 0;
    throw std::runtime_error(std::string("chroot: changing the root directory to ") + path + " is not supported, the guest path resolver keeps a fixed root");
}

int APS5_VABI symlink_nid_postfix(const char* target, const char* created) {
    if (target == nullptr || created == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*target == '\0' || *created == '\0') return static_cast<int>(Fail(GuestEnoent));
    const std::filesystem::path contents(target);
    if (contents.has_root_name() || contents.has_root_directory()) {
        NotImplemented_nid_no_patch("symlink: absolute targets");
        return -1;
    }
    for (const auto& part : contents) {
        if (part == "..") {
            NotImplemented_nid_no_patch("symlink: parent-directory targets");
            return -1;
        }
    }
    std::filesystem::path native;
    if (const int error = ResolvePathError_nid_no_patch(created, native)) return static_cast<int>(Fail(error));
    std::error_code error;
#ifdef _WIN32
    if (GetFileAttributesW(native.wstring().c_str()) != INVALID_FILE_ATTRIBUTES) return static_cast<int>(Fail(GuestEexist));
#else
    if (std::filesystem::exists(std::filesystem::symlink_status(native, error))) return static_cast<int>(Fail(GuestEexist));
#endif
    if (!std::filesystem::is_directory(native.parent_path(), error)) return static_cast<int>(Fail(GuestEnoent));
    const bool directory = std::filesystem::is_directory(native.parent_path() / contents, error);
#ifdef _WIN32
    DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
    if (directory) flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    if (!CreateSymbolicLinkW(native.wstring().c_str(), std::filesystem::path(contents).make_preferred().wstring().c_str(), flags)) {
        const DWORD code = GetLastError();
        if (code == ERROR_PRIVILEGE_NOT_HELD) return static_cast<int>(Fail(GuestEperm));
        if (code == ERROR_ALREADY_EXISTS || code == ERROR_FILE_EXISTS) return static_cast<int>(Fail(GuestEexist));
        if (code == ERROR_PATH_NOT_FOUND) return static_cast<int>(Fail(GuestEnoent));
        throw std::system_error(static_cast<int>(code), std::system_category(), "symlink");
    }
#else
    if (directory)
        std::filesystem::create_directory_symlink(contents, native, error);
    else
        std::filesystem::create_symlink(contents, native, error);
    if (error) throw std::system_error(error, "symlink");
#endif
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI open_nid_postfix(const char* path, int flags, int mode);

int APS5_VABI openat_nid_postfix(int fd, const char* path, int flags, ...) {
    int mode = 0;
    if (flags & SCE_KERNEL_O_CREAT) {
#ifdef _WIN32
        __builtin_sysv_va_list arguments;
        __builtin_sysv_va_start(arguments, flags);
        mode = __builtin_va_arg(arguments, int);
        __builtin_sysv_va_end(arguments);
#else
        std::va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, int);
        va_end(arguments);
#endif
    }
    if (File::InvalidOpenFlags(flags)) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return open_nid_postfix(path, flags, mode);
    const int result = OpenInDirectory_nid_no_patch(fd, path, flags, static_cast<std::uint16_t>(mode));
    if (result < 0) return static_cast<int>(Fail(result & 0xffff));
    return result;
}

int APS5_VABI mkdir_nid_postfix(const char* path, std::uint16_t mode);

int APS5_VABI mkdirat_nid_postfix(int fd, const char* path, std::uint16_t mode) {
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return mkdir_nid_postfix(path, mode);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return static_cast<int>(Fail(GuestEnametoolong));
    int error = 0;
    const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (base == nullptr) return static_cast<int>(Fail(error));
    auto native = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                  std::filesystem::path(reinterpret_cast<const char8_t*>(path));
    while (!native.has_filename() && native.has_relative_path()) native = native.parent_path();
#ifdef _WIN32
    if (!CreateDirectoryW(native.wstring().c_str(), nullptr)) {
        const DWORD code = GetLastError();
        if (code == ERROR_ALREADY_EXISTS || code == ERROR_FILE_EXISTS) return static_cast<int>(Fail(GuestEexist));
        if (code == ERROR_ACCESS_DENIED) return static_cast<int>(Fail(GuestEperm));
        if (code == ERROR_FILENAME_EXCED_RANGE) return static_cast<int>(Fail(GuestEnametoolong));
        if (code == ERROR_PATH_NOT_FOUND) {
            if (HasNonDirectoryPrefix(native)) return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestEnoent));
        }
        return static_cast<int>(Fail(GuestEio));
    }
#else
    if (::mkdir(native.c_str(), static_cast<mode_t>(mode & 0777)) != 0) {
        const int nativeError = errno;
        if ((nativeError == ENOENT || nativeError == ENOTDIR) && HasNonDirectoryPrefix(native))
            return static_cast<int>(Fail(GuestEnotdir));
        return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
    }
#endif
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI fchmodat_nid_postfix(int fd, const char* path, int mode, int flag) {
    if (flag != 0 && flag != GuestAtSymlinkNofollow) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    std::filesystem::path native;
    if (fd == GuestAtFdcwd || *path == '/') {
        if (const int error = ResolvePathError_nid_no_patch(path, native)) return static_cast<int>(Fail(error));
    } else {
        int error = 0;
        const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
        if (base == nullptr) return static_cast<int>(Fail(error));
        native = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) / std::filesystem::path(reinterpret_cast<const char8_t*>(path));
    }
    if (flag == GuestAtSymlinkNofollow) {
#ifdef _WIN32
        const DWORD attributes = GetFileAttributesW(native.wstring().c_str());
        const bool symbolic = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
        std::error_code status;
        const bool symbolic = std::filesystem::is_symlink(std::filesystem::symlink_status(native, status));
#endif
        if (symbolic) {
#ifdef __APPLE__
        if (::fchmodat(AT_FDCWD, native.c_str(), static_cast<mode_t>(mode & 07777), AT_SYMLINK_NOFOLLOW) != 0) {
            return static_cast<int>(Fail(GuestFileError(errno)));
        }
#endif
        RecordWrittenPath_nid_no_patch(native);
        return 0;
    }
    }
#ifdef _WIN32
    if (::_wchmod(native.wstring().c_str(), mode & 07777) != 0) return static_cast<int>(Fail(GuestFileError(errno)));
#else
    if (::chmod(native.c_str(), static_cast<mode_t>(mode & 07777)) != 0) return static_cast<int>(Fail(GuestFileError(errno)));
#endif
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI fchown_nid_postfix(int fd, std::uint32_t uid, std::uint32_t gid) {
    constexpr std::uint32_t Unchanged = 0xFFFFFFFFu;
    if (uid != 0 && uid != Unchanged) NotImplemented_nid_no_patch("fchown: an owner other than the guest user");
    if (gid != 0 && gid != Unchanged) NotImplemented_nid_no_patch("fchown: a group other than the guest group");
#ifdef _WIN32
    if (GuestFstat(fd) != 0) return static_cast<int>(Fail(GuestEbadf));
#else
    struct stat status{};
    if (::fstat(fd, &status) != 0) return static_cast<int>(Fail(GuestEbadf));
#endif
    return 0;
}

int APS5_VABI stat_nid_postfix(const char* path, FileStat* sb);
int APS5_VABI lstat_nid_postfix(const char* path, FileStat* sb);

int APS5_VABI fstatat_nid_postfix(int fd, const char* path, FileStat* sb, int flag) {
    if (flag & ~GuestAtSymlinkNofollow) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return flag ? lstat_nid_postfix(path, sb) : stat_nid_postfix(path, sb);
    const int result = StatInDirectory_nid_no_patch(fd, path, sb, flag == 0);
    if (result < 0) return static_cast<int>(Fail(result & 0xffff));
    return 0;
}

int APS5_VABI unlink_nid_postfix(const char* path);
int APS5_VABI rmdir_nid_postfix(const char* path);

int APS5_VABI unlinkat_nid_postfix(int fd, const char* path, int flag) {
    if (flag & ~GuestAtRemovedir) return static_cast<int>(Fail(GuestEinval));
    if (path == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*path == '\0') return static_cast<int>(Fail(GuestEnoent));
    if (fd == GuestAtFdcwd || *path == '/') return (flag & GuestAtRemovedir) ? rmdir_nid_postfix(path) : unlink_nid_postfix(path);
    if (std::strlen(path) >= GuestPathMax || HasLongComponent(path)) return static_cast<int>(Fail(GuestEnametoolong));
    int error = 0;
    const char* base = GetDirectoryDescriptorPath_nid_no_patch(fd, &error);
    if (base == nullptr) return static_cast<int>(Fail(error));
    auto native = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                  std::filesystem::path(reinterpret_cast<const char8_t*>(path));
    const bool trailingSlash = !native.has_filename() && native.has_relative_path();
    while (!native.has_filename() && native.has_relative_path()) native = native.parent_path();
    std::error_code statusError;
    const auto symlinkStatus = std::filesystem::symlink_status(native, statusError);
    if (flag & GuestAtRemovedir) {
        if (IsDotOrDotDot(path)) return static_cast<int>(Fail(GuestEinval));
        if (std::filesystem::is_symlink(symlinkStatus)) return static_cast<int>(Fail(GuestEnotdir));
        if (!std::filesystem::is_directory(symlinkStatus)) {
            if (HasNonDirectoryPrefix(native) || std::filesystem::exists(symlinkStatus))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestEnoent));
        }
        if (!std::filesystem::is_empty(native, statusError)) return static_cast<int>(Fail(GuestEnotempty));
#ifdef _WIN32
        struct _stat64 status{};
        const bool unlocked = ::_wstat64(native.c_str(), &status) == 0 && !(status.st_mode & _S_IWRITE) &&
            ::_wchmod(native.c_str(), _S_IREAD | _S_IWRITE) == 0;
        if (!std::filesystem::remove(native, statusError)) {
            if (unlocked) ::_wchmod(native.c_str(), _S_IREAD);
            return static_cast<int>(Fail(GuestEio));
        }
#else
        if (::rmdir(native.c_str()) != 0) {
            const int nativeError = errno;
            if ((nativeError == ENOENT || nativeError == ENOTDIR) && HasNonDirectoryPrefix(native))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
        }
#endif
    } else {
        if (trailingSlash) {
            if (std::filesystem::is_directory(symlinkStatus)) return static_cast<int>(Fail(GuestEperm));
            if (std::filesystem::exists(symlinkStatus) || HasNonDirectoryPrefix(native))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestEnoent));
        }
        if (std::filesystem::is_directory(symlinkStatus)) return static_cast<int>(Fail(GuestEperm));
#ifdef _WIN32
        struct _stat64 status{};
        const bool unlocked = ::_wstat64(native.c_str(), &status) == 0 && (status.st_mode & _S_IFMT) == _S_IFREG &&
            !(status.st_mode & _S_IWRITE) && ::_wchmod(native.c_str(), _S_IREAD | _S_IWRITE) == 0;
        if (::_wunlink(native.c_str()) != 0) {
            const int nativeError = errno;
            if (unlocked) ::_wchmod(native.c_str(), _S_IREAD);
            if (HasNonDirectoryPrefix(native)) return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
        }
#else
        if (::unlink(native.c_str()) != 0) {
            const int nativeError = errno;
            if ((nativeError == ENOENT || nativeError == ENOTDIR) && HasNonDirectoryPrefix(native))
                return static_cast<int>(Fail(GuestEnotdir));
            return static_cast<int>(Fail(GuestError(std::error_code(nativeError, std::generic_category()))));
        }
#endif
    }
    RecordWrittenPath_nid_no_patch(native);
    return 0;
}

int APS5_VABI renameat_nid_postfix(int oldfd, const char* from, int newfd, const char* to) {
    if (from == nullptr || to == nullptr) return static_cast<int>(Fail(GuestEfault));
    if (*from == '\0' || *to == '\0') return static_cast<int>(Fail(GuestEnoent));
    if ((oldfd == GuestAtFdcwd || *from == '/') && (newfd == GuestAtFdcwd || *to == '/'))
        return RenameGuestPath_nid_no_patch(from, to);
    if (std::strlen(from) >= GuestPathMax || HasLongComponent(from) ||
        std::strlen(to) >= GuestPathMax || HasLongComponent(to))
        return static_cast<int>(Fail(GuestEnametoolong));
    if (IsDotOrDotDot(from) || IsDotOrDotDot(to)) return static_cast<int>(Fail(GuestEinval));

    std::filesystem::path source;
    if (oldfd == GuestAtFdcwd || *from == '/') {
        if (const int error = ResolvePathError_nid_no_patch(from, source)) return static_cast<int>(Fail(error));
    } else {
        int error = 0;
        const char* base = GetDirectoryDescriptorPath_nid_no_patch(oldfd, &error);
        if (base == nullptr) return static_cast<int>(Fail(error));
        source = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                 std::filesystem::path(reinterpret_cast<const char8_t*>(from));
    }

    std::filesystem::path destination;
    if (newfd == GuestAtFdcwd || *to == '/') {
        if (const int error = ResolvePathError_nid_no_patch(to, destination)) return static_cast<int>(Fail(error));
    } else {
        int error = 0;
        const char* base = GetDirectoryDescriptorPath_nid_no_patch(newfd, &error);
        if (base == nullptr) return static_cast<int>(Fail(error));
        destination = std::filesystem::path(reinterpret_cast<const char8_t*>(base)) /
                      std::filesystem::path(reinterpret_cast<const char8_t*>(to));
    }

    const std::string_view fromView(from);
    const std::string_view toView(to);
    const bool oldTrailingSlash = !fromView.empty() && fromView.back() == '/';
    const bool newTrailingSlash = !toView.empty() && toView.back() == '/';

    while (!source.has_filename() && source.has_relative_path()) source = source.parent_path();
    while (!destination.has_filename() && destination.has_relative_path()) destination = destination.parent_path();

    std::error_code statusError;
    const auto sourceStatus = std::filesystem::symlink_status(source, statusError);
    if (oldTrailingSlash) {
        if (sourceStatus.type() == std::filesystem::file_type::not_found)
            return static_cast<int>(Fail(HasNonDirectoryPrefix(source) ? GuestEnotdir : GuestEnoent));
        if (!std::filesystem::is_directory(sourceStatus)) return static_cast<int>(Fail(GuestEnotdir));
    }
    if (newTrailingSlash && sourceStatus.type() != std::filesystem::file_type::not_found &&
        !std::filesystem::is_directory(sourceStatus))
        return static_cast<int>(Fail(GuestEnotdir));

    const int error = RenamePath(source, destination);
    if (error) return static_cast<int>(Fail(error));
    RecordWrittenPath_nid_no_patch(source);
    RecordWrittenPath_nid_no_patch(destination);
    return 0;
}
int APS5_VABI aio_cancel_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI aio_error_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI aio_fsync_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI aio_read_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI aio_return_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI aio_suspend_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI aio_waitcomplete_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI aio_write_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI amd64_set_fsbase_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI blockpool_batch_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI blockpool_map_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI blockpool_open_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI blockpool_unmap_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI chown_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI cpuset_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI cpuset_getid_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI cpuset_setid_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI dynlib_get_obj_member_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI fchownat_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI fpathconf_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI fstat_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI futimesat_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI get_authinfo_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI get_module_info_list_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI get_page_table_stats_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI get_sdk_compiled_version_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI get_self_auth_info_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI get_vm_map_timestamp_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI getcontext_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI getfsstat_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI getlogin_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI htonl_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI htons_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ipmimgr_call_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI is_in_sandbox_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kenv_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kldfind_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kldfirstmod_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kldload_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kldnext_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kldstat_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kldsym_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kldunload_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kmq_notify_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kmq_open_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kmq_setattr_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kmq_timedreceive_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kmq_timedsend_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI kmq_unlink_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_close_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_destroy_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_getvalue_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_init_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_open_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_post_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_timedwait_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_trywait_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_unlink_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ksem_wait_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ktimer_create_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ktimer_delete_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ktimer_getoverrun_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ktimer_gettime_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ktimer_settime_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ktrace_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI lchflags_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI lchmod_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI lchown_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI linkat_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI mdbg_call_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI mdbg_service_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI mount_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ntohl_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ntohs_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI openintr_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI profil_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pselect_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_attr_getaffinity_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_attr_getsolosched_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_attr_setaffinity_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_attr_setcreatesuspend_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_attr_setstackaddr_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_barrier_setname_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_cleanup_pop_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_cleanup_push_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_cond_setname_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_cond_signalto_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_condattr_getclock_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_get_specificarray_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_get_user_context_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_getprio_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_getstack_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_main_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_multi_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_getprioceiling_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_getspinloops_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_getyieldloops_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_init_for_mono_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_isowned_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_reltimedlock_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_setname_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_setprioceiling_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_setspinloops_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutex_setyieldloops_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutexattr_getgen_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutexattr_getkind_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutexattr_setgen_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_mutexattr_setkind_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_resume_all_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_resume_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_resume_user_context_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_rwlock_reltimedrdlock_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_rwlock_reltimedwrlock_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_rwlock_setname_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_rwlockattr_gettype_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_rwlockattr_settype_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_set_defaultstacksize_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_set_user_context_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_single_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_suspend_all_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_suspend_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_suspend_user_context_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_switch_add_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_switch_delete_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pthread_timedjoin_np_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI ptrace_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI pwrite_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI reboot_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI revoke_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI rfork_thread_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI rtld_printf_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCoredumpDebugForceCoredumpOnAppClose(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCoredumpDebugTriggerCoredump(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCoredumpGetStopInfoGpu(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceCoredumpInternalGetThreadContextInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAddACInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAddCpumodeEvent(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAddGpuExceptionEvent(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAddProcessToCanvasMap(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAddResource(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAllocateDirectMemory2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAllocateDirectMemoryForApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAllocateDirectMemoryForMiniApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAllocateToolMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAllocateTraceDirectMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAprCtrl(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAprSubmitCommandBufferAndGetResult_TEST(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAprSubmitCommandBuffer_TEST(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelAvailableToolMemorySize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelBacktraceSelf(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelBeginAppMount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelBeginAppMount2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelCallIndirectBuffer(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelCallIndirectBuffer2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelClearBootReqNotifyCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelClearGameDirectMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelCloseEport(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelCloseEventFlag(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelCloseSema(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelCreateBudget(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelCreateEport(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugAcquireAndUpdateDebugRegister(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugGetAppStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugGetPauseCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugGetPrivateLogText(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugGetSchedLockMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugGetSdkLogText(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugGpuPaDebugIsInProgress(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugInjectProcessEvent(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugPackageCorrupted(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugRaiseExceptionWithContext(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugRaiseExceptionWithInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDebugReleaseDebugContext(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDeleteACInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDeleteBudget(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDeleteCpumodeEvent(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDeleteEport(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDeleteGpuExceptionEvent(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDeleteProcessFromCanvasMap(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDeleteSavedApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelDirectMemoryQueryForId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEnableDmemAliasing2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEnablePthreadObjectCheck(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEndAppMount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEndAppMount2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEventLogInit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEventLogPread(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEventLogRead(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelEventLogWrite(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelExpectedHddFormat(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelFlock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetACInfoAllForCoredump(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetAppCategoryType(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetAppInfoByAppId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetAppState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetAslrStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetAutoUpdateVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBackupRestoreMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBackupRestoreModeOfNextBoot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBasicProductShape(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBetaUpdateTestForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBioUsageAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBootReqNotifyCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBudget(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetBuildNumber(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCallRecord(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetChildApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCompiledSdkVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCompiledSdkVersionByPath(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCompiledSdkVersionByPid(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCoredumpAppInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCpuUsage(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCpuUsageAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCpuUsageProc(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCpuUsageProc2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCpuUsageThread(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCpumodeGame(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetCurrentFanDuty(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetDataTransferMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetDebugMenuMiniModeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetDebugMenuModeForPsmForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetDebugMenuModeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetDefaultToolMemorySize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetEmergencyErrorLog(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetExecutableModuleHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetExtLibcHandle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetFakeFinalizeMenuForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetFirstImageAddr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetFlagedUpdaterForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetForceUpdateModeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetGPO(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetHwFeatureInfoForDecid(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetHwModelName(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetHwSerialNumber(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetIdPs(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetIdTableCurrentCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetIoreq(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetIpcPath(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetKnobs(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetKqueueFromEqueue(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetMemoryPstate(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetModuleInfoInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetModuleListInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetOpenPsIdForSystem(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetPagingStatsOfAllObjects(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetPagingStatsOfAllThreads(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetParentApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetPhysPageSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetProcessAbiType(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetProcessName(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetProcessType(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetProcessTypeOfBudget(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetProductCode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetProductStr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetPs4SystemSwVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetPsmIntdevModeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetPsnAccessTraceLogForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetQafExpirationTimeNotafterForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetQafExpirationTimeNotbeforeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetQafGenerationForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetQafNameForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetRenderingMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetResidentCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetResidentFmemCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetResource(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetResourceLimit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetResourceOne(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSafemode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSanitizerMallocReplace(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSanitizerNewReplace(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSocClock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSpecialIForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSubsysId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSubsysSubId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSystemExVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSystemLevelDebuggerModeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetSystemSwBeta(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetThreadName(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetTraceMemoryStats(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUniversalMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUpdVersion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenDataExecutionForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenExpirationTimeNotafterForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenExpirationTimeNotbeforeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenFakeSharefactoryForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenFlagedUpdaterForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenNpEnvSwitchingForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenSaveDataRepairForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenStoreModeForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenUseSoftwagnerForAcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenUseSoftwagnerForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetUtokenWeakenedPortRestrictionForRcmgr(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGetVrCaptureSize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelGiveDirectMemoryToMapper(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelHasNeoMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelHwGetBdDriveInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelHwHasOpticalOut(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelHwHasWlanBt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccControlBDPowerState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccControlUSBPowerState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetBDPowerState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetBootTimestamp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetCPMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetCountTime(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetCpuInfoBit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetErrLog(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetHwInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetPowerNumberOfBootShutdown(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetPowerOperatingTime(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetPowerUpCause(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetSysEventLog(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccGetUSBPowerState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccIndicatorBootDone(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccIndicatorShutdown(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccIndicatorStandby(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccIndicatorStandbyBoot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccIndicatorStandbyShutdown(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccNotifyBootStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccNvsFlush(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccReadPowerBootMessage(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccSetBuzzer(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccSetCPMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccSetCpuInfoBit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIccSetDownloadMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelInternalGetKmemStatistics(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelInternalGetMapStatistics(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelInternalHeapPrintBacktraceWithModuleInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelInternalMapDirectMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelInternalMapNamedDirectMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelInternalResumeDirectMemoryRelease(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelInternalSuspendDirectMemoryRelease(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsAllowedToSelectDvdRegion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsAuthenticNeo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsCEX(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsDebuggerAttached(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsDevKit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsExperimentalBeta(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsGenuineCEX(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsGenuineDevKit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsGenuineKratosCex(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsGenuineN(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsGenuineTestKit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsInSandbox(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsKratos(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsM2DeviceAttached(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsMainOnStanbyMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsMainOnStandbyMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsModuleLoaded(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsNeoMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsProspero(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsPs4Process(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelIsTestKit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelJitGetSharedMemoryInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelKernelHeapUsage(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelLoadStartModuleForSysmodule(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelLoadStartModuleInternalForMono(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelLwfsAllocateBlock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelLwfsLseek(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelLwfsSetAttribute(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelLwfsTrimBlock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelLwfsWrite(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMapNamedSystemFlexibleMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMapSanitizerShadowMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMapToolMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMapTraceMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMapperGetParam(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMemoryPoolMove(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMlockall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelMsync(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelNormalizePath(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelNotifyAppEvent(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelNotifyAppStateChanged(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelNotifySystemSuspendResumeProgress(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelNotifySystemSuspendStart(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelOpenEport(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelOpenInternal(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelOpenSema(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelPrepareDirectMemorySwap(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelPrepareToResumeProcess(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelPrepareToSuspendProcess(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelProtectDirectMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelProtectDirectMemoryForPID(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelQueryToolMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelQueryTraceMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelRandomizedPath(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelRdup(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelReboot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelReleaseToolMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelReleaseTraceDirectMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelReportUnpatchedFunctionCall(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelReserve2mbPage(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelReserveSystemDirectMemory(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelRestoreApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelResumeDirectMemoryRelease(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelResumeProcess(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelRtldControl(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSandboxPath(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSaveApp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetAppInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetAppState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetBackupRestoreMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetBaseModeClock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetBesteffort(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetBootReqNotify(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetBudget(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetCallRecord(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetCompressionAttribute(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetCoredumpAppInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetCpumodeGame(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetDataTransferMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetEmergencyErrorLog(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetFsstParam(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetGPI(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetGameDirectMemoryLimit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetGpuCu(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetIoreq(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetKnobs(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetMemoryPstate(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetNeoModeClock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetPackageInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetPhysFmemLimit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetProcessName(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetProcessProperty(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetProcessPropertyString(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetSafemode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetSafemodeDirect(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetSuspendState(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetTimezoneInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetUpdatemode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSetVmContainer(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSettimeofday(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSflashGetWritePrio(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSflashSetWritePrio(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSlvNotifyError(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSpawn(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelStreamWriteActivate(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelStreamWriteCreate(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelStreamWriteDeactivate(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelStreamWriteDelete(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelStreamWriteRead(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelStreamWriteStat(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelStreamWriteWrite(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSuspendDirectMemoryRelease(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSuspendProcess(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSuspendSystem(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSwitchToBaseMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSwitchToNativeMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelSwitchToNeoMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelTerminateProcess(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelTerminateSysCore(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelTitleWorkaroundIsEnabled(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelTitleWorkdaroundIsEnabled(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelTraceMemoryTypeProtect(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelTriggerEport(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelVirtualQueryAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWaitCommandBufferCompletion(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteEventQueueOnCompletionCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteMapCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteMapCommand2(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteMapDirectCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteMapDirectWithGpuMaskIdCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteMapWithGpuMaskIdCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteModifyMtypeProtectWithGpuMaskIdCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteModifyProtectCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteModifyProtectWithGpuMaskIdCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteMultiMapCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteMultiMapWithGpuMaskIdCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteRemapCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteRemapWithGpuMaskIdCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteSdkEventLog(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteSdkEventLogV(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteUnmapCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteWaitOnAddressCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteWaitOnCounterCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteWriteAddressCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelWriteWriteCounterCommand(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceKernelYieldCpumode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceLibcMspaceCreateForMonoMutex(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadAtfork(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadAttrGetscope(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadAttrSetcreatesuspend(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadAttrSetscope(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadBarrierDestroy(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadBarrierInit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadBarrierWait(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadBarrierattrDestroy(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadBarrierattrGetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadBarrierattrInit(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadBarrierattrSetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadCondattrGetclock(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadCondattrGetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadCondattrSetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadGetconcurrency(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadGetcpuclockid(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMain(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMulti(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexGetprioceiling(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexGetspinloops(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexGetyieldloops(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexInitForInternalLibc(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexIsowned(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexSetprioceiling(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexSetspinloops(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexSetyieldloops(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrGetgen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrGetkind(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrGetprioceiling(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrGetprotocol(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrGetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrGettype(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrInitForInternalLibc(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrSetgen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrSetkind(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrSetprioceiling(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadMutexattrSetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadResume(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadResumeAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadRwlockattrGetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadRwlockattrGettype(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadRwlockattrSetpshared(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadSetBesteffort(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadSetDefaultstacksize(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadSetName(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadSetconcurrency(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadSingle(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadSuspend(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadSuspendAll(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI scePthreadTimedjoin(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sched_getparam_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sched_getscheduler_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sched_rr_get_interval_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sched_setparam_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sched_setscheduler_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sem_close_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sem_open_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sem_setname_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sem_unlink_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI set_phys_fmem_limit_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI setlogin_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI setpgid_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI settimeofday_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI siglongjmp_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sigqueue_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sigreturn_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sigsetjmp_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI statfs_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI swapcontext_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI swapon_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI symlinkat_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysKernelGetIntdevModeForInternalLibc_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysKernelGetIntdevModeForRcmgr_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysKernelGetLowerLimitSysexVersion_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysKernelGetLowerLimitUpdVersion_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysKernelGetManufacturingMode_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysKernelGetUpdVersion_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysc_e00_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysc_m00_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sysc_s00_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI tcgetsid_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI tcsetsid_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI unmount_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI utrace_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI uuidgen_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI vfork_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI wait3_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI wait4_nid_postfix(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}

#include "prx/libkernel/File/include/NativeStat.hpp"
#include "prx/libkernel/File/include/DirectoryDescriptor.hpp"
#include "prx/libc/include/FilesystemError.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <cerrno>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

static bool NamesDirectory(std::string_view guestPath) {
    if (!guestPath.empty() && (guestPath.back() == '/' || guestPath.back() == '\\')) return true;
    const auto separator = guestPath.find_last_of("/\\");
    const auto last = guestPath.substr(separator == std::string_view::npos ? 0 : separator + 1);
    return last == "." || last == "..";
}

#ifdef _WIN32
#include "prx/libkernel/File/include/WindowsFileTime.hpp"
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
struct NativeStat : __stat64 {
    FILE_BASIC_INFO times{};
    BY_HANDLE_FILE_INFORMATION identity{};
    bool hasTimes = false;
    bool hasIdentity = false;
};
struct PathQuery {
    int error = 0;
    bool reparsePoint = false;
    NativeStat st{};
};
static int ReadHandleInfo(HANDLE handle, NativeStat* st) {
    if (GetFileType(handle) != FILE_TYPE_DISK) return 0;
    if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &st->times, sizeof(st->times)))
        return File::WindowsFileTime::Failure(GetLastError());
    st->hasTimes = true;
    st->hasIdentity = GetFileInformationByHandle(handle, &st->identity) != 0;
    return 0;
}
static std::uint32_t FoldFileIndex(const BY_HANDLE_FILE_INFORMATION& identity) {
    const std::uint32_t folded = identity.nFileIndexLow ^ (identity.nFileIndexHigh * 0x9E3779B1u);
    return folded == 0 ? 1 : folded;
}
static bool IsSeparator(wchar_t value) {
    return value == L'/' || value == L'\\';
}
static wchar_t FoldAscii(wchar_t value) {
    return value >= L'A' && value <= L'Z' ? static_cast<wchar_t>(value + (L'a' - L'A')) : value;
}
static bool HasDriveLetter(const wchar_t* path) {
    const wchar_t letter = FoldAscii(path[0]);
    return letter >= L'a' && letter <= L'z' && path[1] == L':';
}
static bool IsRootOrEmpty(const wchar_t* path) {
    const wchar_t* start = HasDriveLetter(path) ? path + 2 : path;
    return start[0] == L'\0' || (IsSeparator(start[0]) && start[1] == L'\0');
}
static bool HasExecutableExtension(std::wstring_view path) {
    const auto dot = path.rfind(L'.');
    if (dot == std::wstring_view::npos) return false;
    const auto extension = path.substr(dot);
    for (const std::wstring_view executable : {L".exe", L".cmd", L".bat", L".com"}) {
        if (std::ranges::equal(extension, executable, [](wchar_t left, wchar_t right) { return FoldAscii(left) == right; })) return true;
    }
    return false;
}
static unsigned short StatMode(DWORD attributes, const wchar_t* path) {
    const bool directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || IsRootOrEmpty(path);
    unsigned short mode = directory ? _S_IFDIR | _S_IEXEC : _S_IFREG;
    mode |= (attributes & FILE_ATTRIBUTE_READONLY) != 0 ? _S_IREAD : _S_IREAD | _S_IWRITE;
    if (HasExecutableExtension(path)) mode |= _S_IEXEC;
    mode |= (mode & 0700) >> 3;
    mode |= (mode & 0700) >> 6;
    return mode;
}
[[noreturn]] static void ThrowHandleQueryFailure(const char* query, const std::filesystem::path& p) {
    const DWORD error = GetLastError();
    throw std::runtime_error(std::string("stat: ") + query + " failed with Windows error " + std::to_string(error) + " for " + p.string());
}
static int ReadPathInfo(HANDLE handle, const std::filesystem::path& p, bool followLinks, PathQuery& query) {
    auto& st = query.st;
    const DWORD type = GetFileType(handle) & ~FILE_TYPE_REMOTE;
    if (type == FILE_TYPE_CHAR || type == FILE_TYPE_PIPE) {
        st.st_mode = type == FILE_TYPE_CHAR ? _S_IFCHR : _S_IFIFO;
        st.st_nlink = 1;
        st.st_rdev = st.st_dev = static_cast<_dev_t>(-1);
        DWORD available = 0;
        if (type == FILE_TYPE_PIPE && PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr)) st.st_size = available;
        return 0;
    }
    if (type != FILE_TYPE_DISK) ThrowHandleQueryFailure("GetFileType", p);
    const wchar_t* path = p.c_str();
    if (HasDriveLetter(path) && path[2] == L'\0') return ENOENT;
    const int drive = HasDriveLetter(path) ? FoldAscii(path[0]) - L'a' : _getdrive() - 1;
    st.st_rdev = st.st_dev = static_cast<_dev_t>(drive);
    st.st_nlink = 1;
    if (!GetFileInformationByHandle(handle, &st.identity)) ThrowHandleQueryFailure("GetFileInformationByHandle", p);
    st.hasIdentity = true;
    st.st_mode = StatMode(st.identity.dwFileAttributes, path);
    st.st_size = static_cast<__int64>((static_cast<std::uint64_t>(st.identity.nFileSizeHigh) << 32u) | st.identity.nFileSizeLow);
    if (!GetFileInformationByHandleEx(handle, FileBasicInfo, &st.times, sizeof(st.times))) ThrowHandleQueryFailure("GetFileInformationByHandleEx", p);
    st.hasTimes = true;
    query.reparsePoint = !followLinks && (st.identity.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    return 0;
}
static PathQuery QueryPath(const std::filesystem::path& p, bool followLinks) {
    PathQuery query;
    const auto target = followLinks && p.has_relative_path() && !p.has_filename() ? p.parent_path() : p;
    const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | (followLinks ? 0 : FILE_FLAG_OPEN_REPARSE_POINT);
    const auto handle = CreateFileW(target.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, flags, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        query.error = ENOENT;
        return query;
    }
    const std::unique_ptr<void, decltype(&CloseHandle)> owner(handle, &CloseHandle);
    query.error = ReadPathInfo(handle, target, followLinks, query);
    return query;
}
static int DoFstat(int fd, NativeStat* st) {
    if (const auto directory = File::DirectoryDescriptorPath(fd)) {
        const auto query = QueryPath(*directory, true);
        if (query.error != 0) {
            errno = query.error;
            return -1;
        }
        *st = query.st;
        return 0;
    }
    if (_fstat64(fd, st) != 0) return -1;
    return ReadHandleInfo(reinterpret_cast<HANDLE>(::_get_osfhandle(fd)), st);
}
static void RejectReparsePoint(const PathQuery& query) {
    if (query.reparsePoint) NotImplemented_nid_no_patch("lstat of a Windows symbolic link");
}
#else
#include <sys/stat.h>
using NativeStat = struct stat;
struct PathQuery {
    int error = 0;
    NativeStat st{};
};
static PathQuery QueryPath(const std::filesystem::path& p, bool followLinks) {
    PathQuery query;
    if ((followLinks ? ::stat(p.c_str(), &query.st) : ::lstat(p.c_str(), &query.st)) != 0) query.error = errno;
    return query;
}
static int DoFstat(int fd, NativeStat* st) {
    return ::fstat(fd, st);
}
static void RejectReparsePoint(const PathQuery&) {}
#endif

static void CopyNativeStat(const NativeStat& st, FileStat* sb) {
    *sb = FileStat{};
    sb->st_mode = static_cast<std::uint16_t>(st.st_mode);
    sb->st_size = static_cast<std::int64_t>(st.st_size);
#ifdef _WIN32
    if (st.hasIdentity) sb->st_size = static_cast<std::int64_t>((static_cast<std::uint64_t>(st.identity.nFileSizeHigh) << 32u) | st.identity.nFileSizeLow);
    sb->st_dev = static_cast<std::uint32_t>(st.st_dev);
    sb->st_ino = static_cast<std::uint32_t>(st.st_ino);
    sb->st_nlink = static_cast<std::uint16_t>(st.st_nlink);
    sb->st_uid = 0;
    sb->st_gid = 0;
    sb->st_rdev = static_cast<std::uint32_t>(st.st_rdev);
    sb->st_blksize = 512;
    sb->st_blocks = (sb->st_size + 511LL) / 512LL;
    sb->st_atim.tv_sec = static_cast<std::int64_t>(st.st_atime);
    sb->st_atim.tv_nsec = 0;
    sb->st_mtim.tv_sec = static_cast<std::int64_t>(st.st_mtime);
    sb->st_mtim.tv_nsec = 0;
    sb->st_ctim.tv_sec = static_cast<std::int64_t>(st.st_ctime);
    sb->st_ctim.tv_nsec = 0;
    sb->st_birthtim.tv_sec = static_cast<std::int64_t>(st.st_ctime);
    sb->st_birthtim.tv_nsec = 0;
    if (st.hasTimes) {
        sb->st_atim = File::WindowsFileTime::Decode(st.times.LastAccessTime.QuadPart);
        sb->st_mtim = File::WindowsFileTime::Decode(st.times.LastWriteTime.QuadPart);
        sb->st_ctim = File::WindowsFileTime::Decode(st.times.ChangeTime.QuadPart);
        sb->st_birthtim = File::WindowsFileTime::Decode(st.times.CreationTime.QuadPart);
    }
    if (st.hasIdentity) {
        sb->st_dev = st.identity.dwVolumeSerialNumber;
        sb->st_ino = FoldFileIndex(st.identity);
        sb->st_nlink = static_cast<std::uint16_t>(std::min<DWORD>(st.identity.nNumberOfLinks, 0xffff));
    }
#else
    sb->st_dev = static_cast<std::uint32_t>(st.st_dev);
    sb->st_ino = static_cast<std::uint32_t>(st.st_ino);
    sb->st_nlink = static_cast<std::uint16_t>(st.st_nlink);
    sb->st_uid = st.st_uid;
    sb->st_gid = st.st_gid;
    sb->st_rdev = static_cast<std::uint32_t>(st.st_rdev);
    sb->st_blksize = static_cast<std::uint32_t>(st.st_blksize);
    sb->st_blocks = static_cast<std::int64_t>(st.st_blocks);
    sb->st_atim.tv_sec = static_cast<std::int64_t>(st.st_atim.tv_sec);
    sb->st_atim.tv_nsec = static_cast<std::int64_t>(st.st_atim.tv_nsec);
    sb->st_mtim.tv_sec = static_cast<std::int64_t>(st.st_mtim.tv_sec);
    sb->st_mtim.tv_nsec = static_cast<std::int64_t>(st.st_mtim.tv_nsec);
    sb->st_ctim.tv_sec = static_cast<std::int64_t>(st.st_ctim.tv_sec);
    sb->st_ctim.tv_nsec = static_cast<std::int64_t>(st.st_ctim.tv_nsec);
#if defined(__APPLE__)
    sb->st_birthtim.tv_sec = static_cast<std::int64_t>(st.st_birthtimespec.tv_sec);
    sb->st_birthtim.tv_nsec = static_cast<std::int64_t>(st.st_birthtimespec.tv_nsec);
#elif defined(__linux__)
    sb->st_birthtim = sb->st_ctim;
#else
    sb->st_birthtim.tv_sec = static_cast<std::int64_t>(st.st_birthtim.tv_sec);
    sb->st_birthtim.tv_nsec = static_cast<std::int64_t>(st.st_birthtim.tv_nsec);
#endif
#endif
}

namespace File {

std::filesystem::path PathOrRelinkedModule(const std::filesystem::path& nativePath) {
    std::error_code error;
    if (std::filesystem::exists(std::filesystem::symlink_status(nativePath, error))) return nativePath;
    return RelinkedModulePath_nid_no_patch(nativePath);
}

void FillFileStat(const std::filesystem::path& nativePath, FileStat* sb) {
    const auto query = QueryPath(nativePath, true);
    if (query.error != 0) {
        throw std::runtime_error(std::string("FillFileStat: stat failed for ") + nativePath.string());
    }
    CopyNativeStat(query.st, sb);
}

void FillFileStat(int nativeDescriptor, FileStat* sb) {
    NativeStat st{};
    if (DoFstat(nativeDescriptor, &st) != 0) {
        throw std::runtime_error(std::string("FillFileStat: fstat failed for fd ") + std::to_string(nativeDescriptor));
    }
    CopyNativeStat(st, sb);
}

bool FillFileStatFromDescriptor(int fd, FileStat* sb) {
    NativeStat st{};
    if (DoFstat(fd, &st) != 0) return false;
    CopyNativeStat(st, sb);
    return true;
}

static void ReportQuery(const PathQuery& query, GuestPathStat& result) {
    result.error = query.error;
    if (result.error != 0) return;
    RejectReparsePoint(query);
    CopyNativeStat(query.st, &result.status);
}

static GuestPathStat StatResolvedPath(const char* guestPath, bool followLinks, std::filesystem::path& native) {
    GuestPathStat result;
    const bool directory = NamesDirectory(guestPath);
    const auto query = [follow = followLinks || directory](const std::filesystem::path& p) { return QueryPath(p, follow); };
    std::optional<std::pair<std::filesystem::path, PathQuery>> probed;
    result.resolveError = ResolvePathError_nid_no_patch(guestPath, native, [&](const std::filesystem::path& candidate) {
        probed.emplace(candidate, query(candidate));
        return probed->second.error == 0;
    });
    if (result.resolveError != 0) return result;
    if (!probed || probed->first != native) probed.emplace(native, query(native));
    ReportQuery(probed->second, result);
    if (result.error == 0 && directory && (result.status.st_mode & 0170000) != 0040000) result.error = ENOTDIR;
    return result;
}

GuestPathStat StatGuestPath(const char* guestPath, bool followLinks) {
    std::filesystem::path native;
    return StatResolvedPath(guestPath, followLinks, native);
}

GuestPathStat StatGuestPathOrRelinkedModule(const char* guestPath) {
    std::filesystem::path native;
    auto result = StatResolvedPath(guestPath, true, native);
    if (result.resolveError != 0 || result.error != ENOENT) return result;
    const auto module = PathOrRelinkedModule(native);
    if (module != native) ReportQuery(QueryPath(module, true), result);
    return result;
}

}

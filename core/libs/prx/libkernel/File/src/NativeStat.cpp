#include "prx/libkernel/File/include/NativeStat.hpp"
#include "prx/libkernel/File/include/DirectoryDescriptor.hpp"
#include "prx/libc/include/General.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <map>
#include <mutex>
#include <fstream>
#include <iomanip>

#ifdef _WIN32
#include "prx/libkernel/File/include/WindowsFileTime.hpp"
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
struct NativeStat : __stat64 {
    FILE_BASIC_INFO times{};
    BY_HANDLE_FILE_INFORMATION identity{};
    bool hasTimes = false;
    bool hasIdentity = false;
};

namespace File {
std::mutex& ChmodMutex() {
    static std::mutex mutex;
    return mutex;
}
std::filesystem::path ChmodCachePath();
std::map<std::wstring, std::uint16_t> LoadChmodCache();
std::map<std::wstring, std::uint16_t>& ChmodModes() {
    static std::map<std::wstring, std::uint16_t> modes = LoadChmodCache();
    return modes;
}
std::wstring ChmodKey(const std::filesystem::path& p) {
    std::error_code ec;
    auto key = std::filesystem::absolute(p, ec).lexically_normal().wstring();
    std::transform(key.begin(), key.end(), key.begin(), [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
    return key;
}
std::filesystem::path ChmodCachePath() {
    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    return std::filesystem::path(executable).parent_path() / L"anyps5-chmod-cache.tsv";
}
std::map<std::wstring, std::uint16_t> LoadChmodCache() {
    std::map<std::wstring, std::uint16_t> modes;
    std::error_code ec;
    if (!std::filesystem::exists(ChmodCachePath(), ec)) return modes;
    std::wifstream input(ChmodCachePath());
    std::wstring line;
    while (std::getline(input, line)) {
        const auto separator = line.rfind(L'\t');
        if (separator == std::wstring::npos || separator + 1 >= line.size()) continue;
        try {
            modes[line.substr(0, separator)] = static_cast<std::uint16_t>(std::stoul(line.substr(separator + 1), nullptr, 8));
        } catch (const std::exception&) {
        }
    }
    return modes;
}
void SaveChmodCacheImpl() {
    std::wofstream output(ChmodCachePath(), std::ios::trunc);
    if (!output) return;
    for (const auto& [path, mode] : ChmodModes())
        output << path << L'\t' << std::oct << mode << L'\n';
}
}

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
static int DoStat(const std::filesystem::path& p, NativeStat* st) {
    if (_wstat64(p.c_str(), st) != 0) return -1;
    const auto handle = CreateFileW(p.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return File::WindowsFileTime::Failure(GetLastError());
    const int result = ReadHandleInfo(handle, st);
    CloseHandle(handle);
    if (const std::uint16_t remembered = File::RememberedChmod(p))
        st->st_mode = (st->st_mode & ~01777) | (remembered & 01777);
    return result;
}
static bool PathFromHandle(HANDLE handle, std::filesystem::path& result) {
    char buffer[1024]{};
    if (!GetFileInformationByHandleEx(handle, FileNameInfo, buffer, sizeof(buffer))) return false;
    const auto* nameInfo = reinterpret_cast<FILE_NAME_INFO*>(buffer);
    std::wstring name(nameInfo->FileName, nameInfo->FileNameLength / sizeof(wchar_t));
    if (name.rfind(L"\\Device\\", 0) != 0) {
        result = std::filesystem::path(name);
        return true;
    }
    const auto separator = name.find(L'\\', 8);  // after \Device\
    if (separator == std::wstring::npos) return false;
    const std::wstring device = name.substr(0, separator);
    const std::wstring rest = name.substr(separator);
    wchar_t drives[512]{};
    if (!QueryDosDeviceW(nullptr, drives, 512)) return false;
    for (const wchar_t* drive = drives; *drive; drive += std::wcslen(drive) + 1) {
        wchar_t target[512]{};
        if (QueryDosDeviceW(drive, target, 512) && device == target) {
            result = std::filesystem::path(drive) / rest;
            return true;
        }
    }
    return false;
}

static int DoFstat(int fd, NativeStat* st) {
    if (const auto directory = File::DirectoryDescriptorPath(fd)) return DoStat(*directory, st);
    if (_fstat64(fd, st) != 0) return -1;
    const int result = ReadHandleInfo(reinterpret_cast<HANDLE>(::_get_osfhandle(fd)), st);
    if (result == 0) {
        std::filesystem::path path;
        if (PathFromHandle(reinterpret_cast<HANDLE>(::_get_osfhandle(fd)), path)) {
            if (const std::uint16_t remembered = File::RememberedChmod(path))
                st->st_mode = (st->st_mode & ~01777) | (remembered & 01777);
        }
    }
    return result;
}
static int DoLstat(const std::filesystem::path& p, NativeStat* st) {
    const auto attributes = GetFileAttributesW(p.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
        NotImplemented_nid_no_patch("lstat of a Windows symbolic link");
    return DoStat(p, st);
}
#else
#include <sys/stat.h>
using NativeStat = struct stat;
static int DoStat(const std::filesystem::path& p, NativeStat* st) {
    return ::stat(p.c_str(), st);
}
static int DoFstat(int fd, NativeStat* st) {
    return ::fstat(fd, st);
}
static int DoLstat(const std::filesystem::path& p, NativeStat* st) {
    return ::lstat(p.c_str(), st);
}
#endif

static void CopyNativeStat(const NativeStat& st, FileStat* sb) {
    *sb = FileStat{};
    sb->st_mode = static_cast<std::uint16_t>(st.st_mode);
    sb->st_size = static_cast<std::int64_t>(st.st_size);
#ifdef _WIN32
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

void FillFileStat(const std::filesystem::path& nativePath, FileStat* sb) {
    NativeStat st{};
    if (DoStat(nativePath, &st) != 0) {
        throw std::runtime_error(std::string("FillFileStat: stat failed for ") + nativePath.string());
    }
    CopyNativeStat(st, sb);
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

bool FillLinkStat(const std::filesystem::path& nativePath, FileStat* sb) {
    NativeStat st{};
    if (DoLstat(nativePath, &st) != 0) return false;
    CopyNativeStat(st, sb);
    return true;
}

#ifdef _WIN32
void RememberChmod(const std::filesystem::path& nativePath, std::uint16_t mode) {
    std::size_t pending = 0;
    {
        const std::lock_guard lock{ChmodMutex()};
        ChmodModes()[ChmodKey(nativePath)] = static_cast<std::uint16_t>(mode & 01777);
        static std::size_t updates = 0;
        pending = ++updates;
    }
    if (pending % 256 == 0) SaveChmodCacheImpl();
}
std::uint16_t RememberedChmod(const std::filesystem::path& nativePath) {
    const std::lock_guard lock{ChmodMutex()};
    const auto it = ChmodModes().find(ChmodKey(nativePath));
    return it == ChmodModes().end() ? 0 : it->second;
}
#else
void RememberChmod(const std::filesystem::path&, std::uint16_t) {}
std::uint16_t RememberedChmod(const std::filesystem::path&) { return 0; }
#endif

}

extern "C" void SaveChmodCache_nid_no_patch(void) {
#ifdef _WIN32
    File::SaveChmodCacheImpl();
#endif
}

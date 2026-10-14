#ifdef _WIN32
#include <windows.h>
#include "prx/libc/include/SymbolicLink.hpp"

extern "C" bool IsSymbolicLink_nid_no_patch(const std::filesystem::path& host) {
    const auto handle = CreateFileW(host.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    FILE_ATTRIBUTE_TAG_INFO information{};
    const bool read = GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &information, sizeof(information)) != 0;
    CloseHandle(handle);
    return read && (information.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 && information.ReparseTag == IO_REPARSE_TAG_SYMLINK;
}
#endif

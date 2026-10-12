#ifndef CORE_LIBS_PRX_LIBKERNEL_FILE_NATIVESTAT_HPP
#define CORE_LIBS_PRX_LIBKERNEL_FILE_NATIVESTAT_HPP

#include <filesystem>
#include "SceTypes.hpp"

namespace File {

struct GuestPathStat {
    int resolveError = 0;
    int error = 0;
    FileStat status{};
};

std::filesystem::path PathOrRelinkedModule(const std::filesystem::path& nativePath);
void FillFileStat(const std::filesystem::path& nativePath, FileStat* sb);
void FillFileStat(int nativeDescriptor, FileStat* sb);
bool FillFileStatFromDescriptor(int fd, FileStat* sb);
GuestPathStat StatGuestPath(const char* guestPath, bool followLinks);
GuestPathStat StatGuestPathOrRelinkedModule(const char* guestPath);

}

#endif

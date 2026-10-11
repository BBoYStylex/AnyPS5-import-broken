#ifndef CORE_LIBS_PRX_LIBKERNEL_FILE_NATIVESTAT_HPP
#define CORE_LIBS_PRX_LIBKERNEL_FILE_NATIVESTAT_HPP

#include <filesystem>
#include "SceTypes.hpp"

namespace File {

void FillFileStat(const std::filesystem::path& nativePath, FileStat* sb);
void FillFileStat(int nativeDescriptor, FileStat* sb);
bool FillFileStatFromDescriptor(int fd, FileStat* sb);
bool FillLinkStat(const std::filesystem::path& nativePath, FileStat* sb);

// POSIX permission bits set through chmod are remembered per path and served
// back by stat; the host has no persistent representation for them.
void RememberChmod(const std::filesystem::path& nativePath, std::uint16_t mode);
std::uint16_t RememberedChmod(const std::filesystem::path& nativePath);
extern "C" void SaveChmodCache_nid_no_patch(void);

}

#endif

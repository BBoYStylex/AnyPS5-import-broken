#include "SceTypes.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
int APS5_VABI sceSaveDataMount3(const SaveDataMount3*, SaveDataMountResult*);
int APS5_VABI sceSaveDataUmount2(std::uint32_t, const SaveDataMountPoint*);
int APS5_VABI sceSaveDataGetMountInfo(const SaveDataMountPoint*, SaveDataMountInfo*);
int APS5_VABI sceSaveDataDirNameSearch(const SaveDataDirNameSearchCond*, SaveDataDirNameSearchResult*);
int APS5_VABI sceSaveDataDelete(const SaveDataDelete*);
}

namespace {

constexpr std::uint32_t MountModeRdwr = 2;
constexpr std::uint32_t MountModeCreate2 = 32;

void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

SceSaveDataDirName DirName(const char* name) {
    SceSaveDataDirName dirName{};
    std::snprintf(dirName.data, sizeof(dirName.data), "%s", name);
    return dirName;
}

SaveDataMountResult Mount(const SceSaveDataDirName& dirName, std::uint32_t mode, std::uint64_t blocks) {
    SaveDataMount3 mount{};
    mount.user_id = 1;
    mount.dir_name = &dirName;
    mount.mount_mode = mode;
    mount.blocks = blocks;
    SaveDataMountResult mounted{};
    Require(sceSaveDataMount3(&mount, &mounted) == 0, "mount failed");
    return mounted;
}

SaveDataMountInfo MountInfo(const SaveDataMountPoint& point) {
    SaveDataMountInfo info{};
    Require(sceSaveDataGetMountInfo(&point, &info) == 0, "mount info failed");
    return info;
}

SaveDataSearchInfo SearchInfo(const char* name) {
    const auto pattern = DirName(name);
    SaveDataDirNameSearchCond cond{};
    cond.user_id = 1;
    cond.dir_name = &pattern;
    SceSaveDataDirName found{};
    SaveDataSearchInfo info{};
    SaveDataDirNameSearchResult result{};
    result.dir_names = &found;
    result.dir_names_num = 1;
    result.infos = &info;
    Require(sceSaveDataDirNameSearch(&cond, &result) == 0 && result.hit_num == 1 && result.set_num == 1, "search found no such directory");
    return info;
}

void WriteBytes(const std::filesystem::path& path, std::size_t bytes) {
    std::ofstream file(path, std::ios::binary);
    const std::vector<char> data(bytes, 'x');
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    Require(static_cast<bool>(file), "writing the fixture file failed");
}

void CreatedDirectoryReportsItsBlocks() {
    const auto dirName = DirName("BLOCKS80");
    const auto mounted = Mount(dirName, MountModeCreate2 | MountModeRdwr, 80);
    Require(mounted.mount_status == 1, "a new directory was not reported as created");
    auto info = MountInfo(mounted.mount_point);
    Require(info.blocks == 80 && info.free_blocks == 80, "a created directory must report the blocks its mount asked for, all free");
    WriteBytes(std::filesystem::path("_sd") / "BLOCKS80" / "level.dat", 40000);
    WriteBytes(std::filesystem::path("_sd") / "BLOCKS80" / "tiny.txt", 1);
    info = MountInfo(mounted.mount_point);
    Require(info.blocks == 80 && info.free_blocks == 77, "free blocks must drop by the files' sizes rounded up to 32 KiB blocks");
    Require(sceSaveDataUmount2(0, &mounted.mount_point) == 0, "unmount failed");
    const auto searched = SearchInfo("BLOCKS80");
    Require(searched.blocks == 80 && searched.free_blocks == 77, "the directory search must report the same blocks as the mount info");
    const auto reopened = Mount(dirName, MountModeCreate2 | MountModeRdwr, 32768);
    Require(reopened.mount_status == 0 && MountInfo(reopened.mount_point).blocks == 80, "reopening with another block count must keep the created size");
    Require(sceSaveDataUmount2(0, &reopened.mount_point) == 0, "unmount failed");
    Require(std::filesystem::exists(std::filesystem::path("_sd") / "BLOCKS80.blocks"), "the block count was not kept next to the directory");
    SaveDataDelete del{};
    del.user_id = 1;
    del.dir_name = &dirName;
    Require(sceSaveDataDelete(&del) == 0, "delete failed");
    Require(!std::filesystem::exists(std::filesystem::path("_sd") / "BLOCKS80.blocks"), "deleting the directory left its block count behind");
}

void DirectoryWithoutRecordReportsItsSize() {
    std::filesystem::create_directories(std::filesystem::path("_sd") / "LEGACY");
    WriteBytes(std::filesystem::path("_sd") / "LEGACY" / "data.bin", 65536);
    const auto searched = SearchInfo("LEGACY");
    Require(searched.blocks == 96 && searched.free_blocks == 94, "a directory without a recorded block count must report its files rounded up to the minimum, as allocated");
    WriteBytes(std::filesystem::path("_sd") / "LEGACY" / "big.bin", 200u * 32768u);
    const auto grown = SearchInfo("LEGACY");
    Require(grown.blocks == 202 && grown.free_blocks == 0, "a directory without a recorded block count larger than the minimum must report its files as allocated");
    const auto mounted = Mount(DirName("LEGACY"), MountModeRdwr, 32768);
    const auto info = MountInfo(mounted.mount_point);
    Require(info.blocks == 202 && info.free_blocks == 0, "opening a directory without a recorded block count must not record the opening mount's count");
    Require(sceSaveDataUmount2(0, &mounted.mount_point) == 0, "unmount failed");
    Require(!std::filesystem::exists(std::filesystem::path("_sd") / "LEGACY.blocks"), "opening a directory recorded a block count");
}

}

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("anyps5-savedata-blocks-" + std::to_string(std::random_device{}()));
    const auto work = root / "work";
    std::filesystem::create_directories(work / "_sd");
    const auto previous = std::filesystem::current_path();
    std::filesystem::current_path(work);
    try {
        CreatedDirectoryReportsItsBlocks();
        DirectoryWithoutRecordReportsItsSize();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        std::filesystem::current_path(previous);
        std::filesystem::remove_all(root);
        return 1;
    }
    std::filesystem::current_path(previous);
    std::filesystem::remove_all(root);
    std::puts("save data block tests passed");
    return 0;
}

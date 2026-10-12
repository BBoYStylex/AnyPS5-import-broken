#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/General.hpp"
#include "SceTypes.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <system_error>

extern "C" {
int APS5_VABI sceSaveDataInitialize3(const void*);
int APS5_VABI sceSaveDataMount3(const SaveDataMount3*, SaveDataMountResult*);
int APS5_VABI sceSaveDataUmount2(std::uint32_t, const SaveDataMountPoint*);
}

namespace {

constexpr std::uint32_t MountReadWrite = 2;
constexpr std::uint32_t MountCreate = 4;
constexpr std::uint32_t MountCreate2 = 32;
constexpr std::uint32_t ErrorInternal = 0x809f000b;
constexpr std::uint32_t ErrorNotFound = 0x809f0008;
constexpr std::uint32_t ErrorExists = 0x809f0007;
constexpr std::uint32_t ErrorBusy = 0x809f0003;

int failures = 0;

void Check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "savedata mount status check failed: %s\n", what.c_str());
    ++failures;
}

int Mount(const char* name, std::uint32_t mode, SaveDataMountResult& result) {
    SceSaveDataDirName dirName{};
    std::memcpy(dirName.data, name, std::strlen(name) + 1);
    SaveDataMount3 mount{};
    mount.dir_name = &dirName;
    mount.mount_mode = mode;
    std::fprintf(stderr, "Mount3 begin: _sd/%s mode0x%x\n", name, static_cast<unsigned>(mode));
    std::fflush(stderr);
    const int code = sceSaveDataMount3(&mount, &result);
    std::fprintf(stderr, "Mount3 returned: _sd/%s code0x%x\n", name, static_cast<unsigned>(code));
    std::fflush(stderr);
    return code;
}

void CheckError(const char* name, std::uint32_t mode, std::uint32_t expectedCode, const std::string& what) {
    SaveDataMountResult result;
    std::memset(&result, 0xa5, sizeof(result));
    Check(static_cast<std::uint32_t>(Mount(name, mode, result)) == expectedCode, what + ": error code");
    const std::array<unsigned char, sizeof(result)> zero{};
    Check(std::memcmp(&result, zero.data(), zero.size()) == 0, what + ": zero output");
}

void MountAndCheck(const char* name, std::uint32_t mode, std::uint32_t expectedStatus, const std::string& what) {
    SaveDataMountResult result{};
    const int code = Mount(name, mode, result);
    Check(code == 0, what + ": mounts");
    if (code != 0) return;
    Check(std::strcmp(result.mount_point.data, "/savedata0") == 0, what + ": first slot");
    const auto expectedPath = std::filesystem::absolute(std::filesystem::path("_sd") / name / "probe");
    Check(ResolvePath_nid_no_patch("/savedata0/probe") == expectedPath, what + ": real alias");
    Check(result.mount_status == expectedStatus, what + ": status is " + std::to_string(result.mount_status) + ", expected " + std::to_string(expectedStatus));
    CheckError(name, mode, ErrorBusy, what + ": busy mount");
    Check(sceSaveDataUmount2(0, &result.mount_point) == 0, what + ": unmounts");
}

}

int main() {
    const auto parent = std::filesystem::canonical(std::filesystem::temp_directory_path());
    const auto root = parent / ("anyps5-savedata-mount-status-" + std::to_string(std::random_device{}()));
    std::error_code error;
    if (!std::filesystem::create_directory(root, error) || error) {
        std::fprintf(stderr, "cannot create exclusive savedata test root\n");
        return 1;
    }
    const auto previous = std::filesystem::current_path();
    std::filesystem::current_path(root);
    Check(sceSaveDataInitialize3(nullptr) == 0, "SaveData initializes");
    const auto aliasBefore = ResolvePath_nid_no_patch("/savedata0/probe");
    const std::string sentinel = "savedata collision sentinel";
    {
        std::ofstream file("_sd", std::ios::binary);
        file << sentinel;
        Check(static_cast<bool>(file), "collision file created");
    }
    CheckError("collision", MountReadWrite | MountCreate2, ErrorInternal, "regular parent collision");
    {
        std::ifstream file("_sd", std::ios::binary);
        const std::string contents{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        Check(contents == sentinel, "collision sentinel preserved");
    }
    Check(ResolvePath_nid_no_patch("/savedata0/probe") == aliasBefore, "failed mount preserves alias");
    Check(std::filesystem::remove("_sd"), "remove owned collision file");
    CheckError("missing", MountReadWrite, ErrorNotFound, "open missing save");
    MountAndCheck("fresh", MountReadWrite | MountCreate2, 1u, "CREATE2 of a missing save");
    MountAndCheck("fresh", MountReadWrite | MountCreate2, 0u, "CREATE2 of an existing save");
    CheckError("fresh", MountReadWrite | MountCreate, ErrorExists, "CREATE of an existing save");
    MountAndCheck("made", MountReadWrite | MountCreate, 1u, "CREATE of a missing save");
    MountAndCheck("made", MountReadWrite, 0u, "open of an existing save");
    std::filesystem::current_path(previous);
    const auto actualRoot = std::filesystem::canonical(root, error);
    const auto status = std::filesystem::symlink_status(root);
    const bool owned = !error && actualRoot == root && actualRoot.parent_path() == parent && std::filesystem::is_directory(status) && !std::filesystem::is_symlink(status);
    Check(owned, "exclusive cleanup containment");
    if (owned) {
        std::filesystem::remove_all(root, error);
        Check(!error, "owned root cleanup");
    }
    if (failures != 0) return 1;
    std::printf("savedata mount status tests passed\n");
    return 0;
}

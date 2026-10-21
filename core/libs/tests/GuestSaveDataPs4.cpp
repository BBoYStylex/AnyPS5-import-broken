#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>

extern "C" int APS5_VABI sceSaveDataTransferringMountPs4(const SaveDataTransferringMount*, SaveDataMountResult*);
extern "C" int APS5_VABI sceSaveDataDirNameSearchPs4(const SaveDataDirNameSearchCond*, SaveDataDirNameSearchResult*);
extern "C" int APS5_VABI sceSaveDataDirNameSearch(const SaveDataDirNameSearchCond*, SaveDataDirNameSearchResult*);
extern "C" int APS5_VABI sceSaveDataInitialize3(const void*);
extern "C" int APS5_VABI sceSaveDataMount2(const SaveDataMount2*, SaveDataMountResult*);
extern "C" int APS5_VABI sceSaveDataUmount2(std::uint32_t, const SaveDataMountPoint*);

static_assert(offsetof(SaveDataMount2, dir_name) == 8 && offsetof(SaveDataMount2, blocks) == 16 && offsetof(SaveDataMount2, mount_mode) == 24 && sizeof(SaveDataMount2) == 64, "SaveDataMount2 has the PS4 layout");

namespace {

constexpr int SaveDataErrorNotFound = -2137063416;
constexpr int SaveDataErrorBusy = -2137063421;
constexpr std::uint32_t MountReadWrite = 2;
constexpr std::uint32_t MountCreate2 = 32;

int failures = 0;

void Check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "savedata ps4 check failed: %s\n", what.c_str());
    ++failures;
}

template <typename TFunction>
bool ThrowsRuntimeError(TFunction function) {
    try {
        function();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void TestTransferringMountPs4() {
    SceSaveDataTitleId title{};
    std::strcpy(title.data, "CUSA00001");
    SceSaveDataDirName dir{};
    std::strcpy(dir.data, "kept");
    SaveDataTransferringMount mount{};
    mount.user_id = 1;
    mount.title_id = &title;
    mount.dir_name = &dir;
    SaveDataMountResult result{};
    std::memset(&result, 0xAA, sizeof(result));
    Check(sceSaveDataTransferringMountPs4(&mount, &result) == SaveDataErrorNotFound, "TransferringMountPs4 returns NOT_FOUND");
    Check(result.mount_point.data[0] == '\0', "TransferringMountPs4 reports no mount point");
}

void TestDirNameSearchPs4() {
    SceSaveDataDirName names[4]{};
    std::memset(names, 0x5A, sizeof(names));
    SaveDataDirNameSearchCond cond{};
    cond.user_id = 1;
    SaveDataDirNameSearchResult result{};
    result.hit_num = 7;
    result.set_num = 7;
    result.dir_names = names;
    result.dir_names_num = 4;

    Check(sceSaveDataDirNameSearch(&cond, &result) == 0 && result.hit_num == 1 && result.set_num == 1, "the PS5 search finds the PS5 save");

    std::memset(names, 0x5A, sizeof(names));
    result.hit_num = 7;
    result.set_num = 7;
    Check(sceSaveDataDirNameSearchPs4(&cond, &result) == 0, "DirNameSearchPs4 succeeds");
    Check(result.hit_num == 0, "DirNameSearchPs4 writes zero hits");
    Check(result.set_num == 0, "DirNameSearchPs4 writes zero set entries");
    Check(static_cast<unsigned char>(names[0].data[0]) == 0x5A, "DirNameSearchPs4 leaves the name buffer untouched");

    Check(ThrowsRuntimeError([&] { sceSaveDataDirNameSearchPs4(nullptr, &result); }), "null cond throws");
    Check(ThrowsRuntimeError([&] { sceSaveDataDirNameSearchPs4(&cond, nullptr); }), "null result throws");
}

void TestMount2() {
    Check(sceSaveDataInitialize3(nullptr) == 0, "SaveData initializes");
    SceSaveDataDirName created{};
    std::strcpy(created.data, "mount2-new");
    SaveDataMount2 mount{};
    mount.user_id = 1;
    mount.dir_name = &created;
    mount.blocks = 96;
    mount.mount_mode = MountReadWrite | MountCreate2;
    SaveDataMountResult result{};
    Check(sceSaveDataMount2(&mount, &result) == 0, "Mount2 creates a missing save");
    Check(result.mount_status == 1u, "Mount2 reports the save as created");
    Check(result.mount_point.data[0] != '\0', "Mount2 reports a mount point");
    Check(std::filesystem::is_directory(std::filesystem::path("_sd") / "mount2-new"), "Mount2 creates the save directory");
    SaveDataMountResult busy{};
    Check(sceSaveDataMount2(&mount, &busy) == SaveDataErrorBusy, "Mount2 of a mounted save is BUSY");
    Check(sceSaveDataUmount2(0, &result.mount_point) == 0, "the Mount2 mount point unmounts");

    SceSaveDataDirName kept{};
    std::strcpy(kept.data, "kept");
    mount.dir_name = &kept;
    mount.mount_mode = MountReadWrite;
    Check(sceSaveDataMount2(&mount, &result) == 0, "Mount2 opens an existing save");
    Check(result.mount_status == 0u, "Mount2 reports the existing save as opened");
    Check(sceSaveDataUmount2(0, &result.mount_point) == 0, "the opened save unmounts");

    SceSaveDataDirName absent{};
    std::strcpy(absent.data, "absent");
    mount.dir_name = &absent;
    Check(sceSaveDataMount2(&mount, &result) == SaveDataErrorNotFound, "Mount2 open of a missing save is NOT_FOUND");
    Check(ThrowsRuntimeError([&] { sceSaveDataMount2(nullptr, &result); }), "null mount throws");
}

}

int main() {
    const auto root = std::filesystem::temp_directory_path() / ("anyps5-savedata-ps4-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(root / "_sd" / "kept");
    const auto previous = std::filesystem::current_path();
    std::filesystem::current_path(root);

    TestTransferringMountPs4();
    TestDirNameSearchPs4();
    TestMount2();

    std::filesystem::current_path(previous);
    std::error_code error;
    std::filesystem::remove_all(root, error);
    if (failures != 0) return 1;
    std::printf("savedata ps4 tests passed\n");
    return 0;
}

#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" int APS5_VABI sceSaveDataInitialize3(const void*);
extern "C" int APS5_VABI sceSaveDataTerminate();
extern "C" int APS5_VABI sceSaveDataMount3(const SaveDataMount3*, SaveDataMountResult*);
extern "C" int APS5_VABI sceSaveDataUmount2(std::uint32_t, const SaveDataMountPoint*);
extern "C" int APS5_VABI sceSaveDataSaveIcon(const SaveDataMountPoint*, const SaveDataIcon*);
extern "C" int APS5_VABI sceSaveDataLoadIcon(const SaveDataMountPoint*, SaveDataIcon*);
extern "C" int APS5_VABI sceSaveDataSetupSaveDataMemory2(const SaveDataMemorySetup2*, SaveDataMemorySetupResult*);
extern "C" int APS5_VABI sceSaveDataGetSaveDataMemory2(SaveDataMemoryGet2*);
extern "C" int APS5_VABI sceSaveDataSetSaveDataMemory2(const SaveDataMemorySet2*);

namespace {

constexpr int SaveDataErrorParameter = -2137063424;
constexpr int SaveDataErrorNotMounted = -2137063420;
constexpr int SaveDataErrorNotFound = -2137063416;
constexpr int SaveDataErrorBadMounted = -2137063411;

int failures = 0;

void Check(bool condition, const std::string& what) {
    if (condition) return;
    std::fprintf(stderr, "savedata icon check failed: %s\n", what.c_str());
    ++failures;
}

std::vector<char> PngBytes(char seed, std::size_t size) {
    return std::vector<char>(size, seed);
}

SaveDataIcon IconOf(std::vector<char>& storage) {
    SaveDataIcon icon{};
    icon.buf = storage.data();
    icon.buf_size = storage.size();
    icon.data_size = storage.size();
    return icon;
}

void TestMountedIconRoundTrip() {
    SaveDataMount3 mount{};
    mount.user_id = 1;
    SceSaveDataDirName dir{};
    std::strcpy(dir.data, "iconsave");
    mount.dir_name = &dir;
    mount.mount_mode = 0x22;
    SaveDataMountResult mounted{};
    Check(sceSaveDataMount3(&mount, &mounted) == 0, "rdwr mount succeeds");

    SaveDataMountPoint point{};
    std::memcpy(point.data, mounted.mount_point.data, sizeof(point.data));

    std::vector<char> sink(64);
    SaveDataIcon loader = IconOf(sink);
    Check(sceSaveDataLoadIcon(&point, &loader) == SaveDataErrorNotFound, "load without an icon returns NOT_FOUND");

    const std::vector<char> bytes = PngBytes('i', 100);
    std::vector<char> source = bytes;
    SaveDataIcon saver = IconOf(source);
    Check(sceSaveDataSaveIcon(&point, &saver) == 0, "save icon succeeds");
    Check(std::filesystem::is_regular_file(std::filesystem::path("_sd/iconsave/sce_sys/icon0.png")),
          "icon lands at sce_sys/icon0.png inside the save directory");

    std::vector<char> loaded(256);
    SaveDataIcon reader = IconOf(loaded);
    Check(sceSaveDataLoadIcon(&point, &reader) == 0, "load icon succeeds");
    Check(reader.data_size == bytes.size(), "load reports the full file size");
    Check(std::memcmp(loaded.data(), bytes.data(), bytes.size()) == 0, "loaded bytes match the saved bytes");

    std::vector<char> narrow(10);
    SaveDataIcon narrow_reader = IconOf(narrow);
    Check(sceSaveDataLoadIcon(&point, &narrow_reader) == 0, "load with a small buffer succeeds");
    Check(narrow_reader.data_size == bytes.size(), "small-buffer load still reports the full size");
    Check(std::memcmp(narrow.data(), bytes.data(), narrow.size()) == 0, "small-buffer load fills buf_size bytes");

    Check(sceSaveDataUmount2(0, &point) == 0, "umount succeeds");
}

void TestParameterChecks() {
    SaveDataMount3 mount{};
    mount.user_id = 1;
    SceSaveDataDirName dir{};
    std::strcpy(dir.data, "iconargs");
    mount.dir_name = &dir;
    mount.mount_mode = 0x22;
    SaveDataMountResult mounted{};
    Check(sceSaveDataMount3(&mount, &mounted) == 0, "mount for argument checks succeeds");
    SaveDataMountPoint point{};
    std::memcpy(point.data, mounted.mount_point.data, sizeof(point.data));

    const std::vector<char> bytes = PngBytes('a', 8);
    std::vector<char> source = bytes;
    SaveDataIcon saver = IconOf(source);
    Check(sceSaveDataSaveIcon(&point, nullptr) == SaveDataErrorParameter, "save with a null icon returns PARAMETER");
    saver.buf = nullptr;
    Check(sceSaveDataSaveIcon(&point, &saver) == SaveDataErrorParameter, "save with a null buffer returns PARAMETER");
    SaveDataMountPoint missing{};
    std::strcpy(missing.data, "/savedata15");
    Check(sceSaveDataSaveIcon(&missing, nullptr) == SaveDataErrorNotMounted, "save on an unmounted point returns NOT_MOUNTED");

    SaveDataIcon reader = IconOf(source);
    Check(sceSaveDataLoadIcon(&point, nullptr) == SaveDataErrorParameter, "load with a null icon returns PARAMETER");
    Check(sceSaveDataLoadIcon(&point, &reader) == SaveDataErrorNotFound, "load after failed saves still returns NOT_FOUND");

    Check(sceSaveDataUmount2(0, &point) == 0, "umount after argument checks succeeds");
}

void TestReadOnlyMount() {
    SaveDataMount3 mount{};
    mount.user_id = 1;
    SceSaveDataDirName dir{};
    std::strcpy(dir.data, "iconrdonly");
    mount.dir_name = &dir;
    mount.mount_mode = 0x21;
    SaveDataMountResult mounted{};
    Check(sceSaveDataMount3(&mount, &mounted) == 0, "rdonly mount succeeds");
    SaveDataMountPoint point{};
    std::memcpy(point.data, mounted.mount_point.data, sizeof(point.data));

    const std::vector<char> bytes = PngBytes('r', 8);
    std::vector<char> source = bytes;
    SaveDataIcon saver = IconOf(source);
    Check(sceSaveDataSaveIcon(&point, &saver) == SaveDataErrorBadMounted, "save on a read-only mount returns BAD_MOUNTED");

    std::vector<char> loaded(64);
    SaveDataIcon reader = IconOf(loaded);
    Check(sceSaveDataLoadIcon(&point, &reader) == SaveDataErrorNotFound, "a rejected save writes no icon");

    Check(sceSaveDataUmount2(0, &point) == 0, "rdonly umount succeeds");
}

void TestSaveDataMemoryIcon() {
    const std::int32_t user = 1;
    const std::uint32_t slot = 2;
    const std::vector<char> seeded = PngBytes('s', 24);

    SaveDataMemorySetup2 setup{};
    setup.user_id = user;
    setup.slot_id = slot;
    setup.memory_size = 1024;
    std::vector<char> icon_storage = seeded;
    SaveDataIcon init_icon = IconOf(icon_storage);
    setup.init_icon = &init_icon;
    SaveDataMemorySetupResult setup_result{};
    Check(sceSaveDataSetupSaveDataMemory2(&setup, &setup_result) == 0, "setup with an init icon succeeds");
    Check(setup_result.existed_memory_size == 0, "a fresh memory slot reports no existing size");
    Check(std::filesystem::is_regular_file(std::filesystem::path("_sd_mem/u1/slot2.icon")),
          "the init icon lands in the memory slot sidecar");

    std::vector<char> loaded(64);
    SaveDataMemoryGet2 get{};
    get.user_id = user;
    get.slot_id = slot;
    SaveDataIcon reader = IconOf(loaded);
    get.icon = &reader;
    Check(sceSaveDataGetSaveDataMemory2(&get) == 0, "get with an icon succeeds");
    Check(reader.data_size == seeded.size(), "get reports the seeded icon size");
    Check(std::memcmp(loaded.data(), seeded.data(), seeded.size()) == 0, "get returns the seeded icon bytes");

    SaveDataMemoryGet2 probe{};
    probe.user_id = user;
    probe.slot_id = slot;
    SaveDataIcon probe_icon{};
    probe.icon = &probe_icon;
    Check(sceSaveDataGetSaveDataMemory2(&probe) == 0, "get with a null icon buffer succeeds");
    Check(probe_icon.data_size == 0, "get with a null icon buffer reports zero");

    const std::vector<char> replaced = PngBytes('n', 48);
    std::vector<char> next_storage = replaced;
    SaveDataIcon next_icon = IconOf(next_storage);
    SaveDataMemorySet2 set{};
    set.user_id = user;
    set.slot_id = slot;
    set.icon = &next_icon;
    set.data_num = 0;
    Check(sceSaveDataSetSaveDataMemory2(&set) == 0, "set with an icon succeeds");
    reader = IconOf(loaded);
    get.icon = &reader;
    Check(sceSaveDataGetSaveDataMemory2(&get) == 0, "get after overwriting succeeds");
    Check(reader.data_size == replaced.size(), "get reports the replaced icon size");
    Check(std::memcmp(loaded.data(), replaced.data(), replaced.size()) == 0, "get returns the replaced bytes");

    SaveDataIcon empty_icon{};
    set.icon = &empty_icon;
    Check(sceSaveDataSetSaveDataMemory2(&set) == 0, "set with an empty icon succeeds");
    Check(!std::filesystem::exists(std::filesystem::path("_sd_mem/u1/slot2.icon")),
          "an empty icon set drops the sidecar file");
    reader = IconOf(loaded);
    get.icon = &reader;
    Check(sceSaveDataGetSaveDataMemory2(&get) == 0, "get after dropping the icon succeeds");
    Check(reader.data_size == 0, "get after dropping the icon reports zero");
}

}

int main() {
    const auto previous = std::filesystem::current_path();
    const auto root = std::filesystem::temp_directory_path() /
        ("anyps5-savedata-icon-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(root);
    std::filesystem::current_path(root);
    int status = 0;
    try {
        Check(sceSaveDataInitialize3(nullptr) == 0, "initialize succeeds");
        TestMountedIconRoundTrip();
        TestParameterChecks();
        TestReadOnlyMount();
        TestSaveDataMemoryIcon();
        Check(sceSaveDataTerminate() == 0, "terminate succeeds");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        status = 1;
    }
    std::filesystem::current_path(previous);
    std::error_code error;
    std::filesystem::remove_all(root, error);
    if (failures != 0) return 1;
    std::printf("savedata icon tests passed\n");
    return status;
}

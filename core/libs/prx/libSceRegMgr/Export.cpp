#include <cstdint>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

namespace {

constexpr int SCE_REG_MGR_ERROR_INVALID_PARAM = static_cast<int>(0x80820001);
constexpr int SCE_REG_MGR_ERROR_NOT_FOUND = static_cast<int>(0x80820002);
constexpr int SCE_REG_MGR_ERROR_BUFFER_TOO_SMALL = static_cast<int>(0x80820004);
constexpr int SCE_REG_MGR_ERROR_INVALID_TYPE = static_cast<int>(0x80820006);

constexpr uint32_t REG_KEY_VERSION = 17825792;
constexpr uint32_t REG_KEY_VERSION_PS4 = 16842752;
constexpr uint32_t REG_KEY_UPDATE_MODE = 33619968;
constexpr uint32_t REG_KEY_LANGUAGE = 33685504;
constexpr uint32_t REG_KEY_NICKNAME = 33882112;
constexpr uint32_t REG_KEY_DIMMER_INTERVAL = 33947648;
constexpr uint32_t REG_KEY_AUTO_POWER_DOWN = 42075904;
constexpr uint32_t REG_KEY_INIT_USER = 117637120;

enum EntryType {
    TypeInt = 0,
    TypeStr = 1,
    TypeBin = 2,
};

struct RegEntry {
    EntryType type;
    int32_t intVal = 0;
    std::string strVal;
    std::vector<uint8_t> binVal;
    bool hasInit = false;
    EntryType initType = TypeInt;
    int32_t initIntVal = 0;
    std::string initStrVal;
    std::vector<uint8_t> initBinVal;
};

class RegistryStore {
public:
    RegistryStore() {
        initDefaults();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        nonSysEntries_.clear();
        initDefaults();
    }

    int getInt(uint32_t key, int32_t* out) {
        if (out == nullptr) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.type != TypeInt) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        *out = it->second.intVal;
        return 0;
    }

    int getStr(uint32_t key, char* out, size_t len) {
        if (out == nullptr || len == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.type != TypeStr) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        if (len <= it->second.strVal.size()) return SCE_REG_MGR_ERROR_BUFFER_TOO_SMALL;
        std::memcpy(out, it->second.strVal.c_str(), it->second.strVal.size() + 1);
        return 0;
    }

    int getBin(uint32_t key, void* out, size_t size) {
        if (out == nullptr || size == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.type != TypeBin) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        if (size < it->second.binVal.size()) return SCE_REG_MGR_ERROR_BUFFER_TOO_SMALL;
        std::memcpy(out, it->second.binVal.data(), it->second.binVal.size());
        return 0;
    }

    int setInt(uint32_t key, int32_t val) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = entries_[key];
        entry.type = TypeInt;
        entry.intVal = val;
        return 0;
    }

    int setStr(uint32_t key, const char* str, size_t len) {
        if (str == nullptr) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = entries_[key];
        entry.type = TypeStr;
        size_t slen = 0;
        if (len > 0) {
            while (slen < len && str[slen] != '\0') ++slen;
        } else {
            slen = std::strlen(str);
        }
        entry.strVal.assign(str, slen);
        return 0;
    }

    int setBin(uint32_t key, const void* data, size_t size) {
        if (data == nullptr || size == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = entries_[key];
        entry.type = TypeBin;
        const auto* ptr = static_cast<const uint8_t*>(data);
        entry.binVal.assign(ptr, ptr + size);
        return 0;
    }

    int getIntInitVal(uint32_t key, int32_t* out) {
        if (out == nullptr) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end() || !it->second.hasInit) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.initType != TypeInt) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        *out = it->second.initIntVal;
        return 0;
    }

    int getStrInitVal(uint32_t key, char* out, size_t len) {
        if (out == nullptr || len == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end() || !it->second.hasInit) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.initType != TypeStr) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        if (len <= it->second.initStrVal.size()) return SCE_REG_MGR_ERROR_BUFFER_TOO_SMALL;
        std::memcpy(out, it->second.initStrVal.c_str(), it->second.initStrVal.size() + 1);
        return 0;
    }

    int getBinInitVal(uint32_t key, void* out, size_t size) {
        if (out == nullptr || size == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end() || !it->second.hasInit) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.initType != TypeBin) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        if (size < it->second.initBinVal.size()) return SCE_REG_MGR_ERROR_BUFFER_TOO_SMALL;
        std::memcpy(out, it->second.initBinVal.data(), it->second.initBinVal.size());
        return 0;
    }

    int resetVal(uint32_t key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.hasInit) {
            it->second.type = it->second.initType;
            it->second.intVal = it->second.initIntVal;
            it->second.strVal = it->second.initStrVal;
            it->second.binVal = it->second.initBinVal;
        } else {
            entries_.erase(it);
        }
        return 0;
    }

    int nonSysGetInt(int64_t key, int32_t* out) {
        if (out == nullptr) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nonSysEntries_.find(key);
        if (it == nonSysEntries_.end()) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.type != TypeInt) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        *out = it->second.intVal;
        return 0;
    }

    int nonSysGetStr(int64_t key, char* out, size_t len) {
        if (out == nullptr || len == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nonSysEntries_.find(key);
        if (it == nonSysEntries_.end()) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.type != TypeStr) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        if (len <= it->second.strVal.size()) return SCE_REG_MGR_ERROR_BUFFER_TOO_SMALL;
        std::memcpy(out, it->second.strVal.c_str(), it->second.strVal.size() + 1);
        return 0;
    }

    int nonSysGetBin(int64_t key, void* out, size_t size) {
        if (out == nullptr || size == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nonSysEntries_.find(key);
        if (it == nonSysEntries_.end()) return SCE_REG_MGR_ERROR_NOT_FOUND;
        if (it->second.type != TypeBin) return SCE_REG_MGR_ERROR_INVALID_TYPE;
        if (size < it->second.binVal.size()) return SCE_REG_MGR_ERROR_BUFFER_TOO_SMALL;
        std::memcpy(out, it->second.binVal.data(), it->second.binVal.size());
        return 0;
    }

    int nonSysSetInt(int64_t key, int32_t val) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = nonSysEntries_[key];
        entry.type = TypeInt;
        entry.intVal = val;
        return 0;
    }

    int nonSysSetStr(int64_t key, const char* str, size_t len) {
        if (str == nullptr) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = nonSysEntries_[key];
        entry.type = TypeStr;
        size_t slen = 0;
        if (len > 0) {
            while (slen < len && str[slen] != '\0') ++slen;
        } else {
            slen = std::strlen(str);
        }
        entry.strVal.assign(str, slen);
        return 0;
    }

    int nonSysSetBin(int64_t key, const void* data, size_t size) {
        if (data == nullptr || size == 0) return SCE_REG_MGR_ERROR_INVALID_PARAM;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = nonSysEntries_[key];
        entry.type = TypeBin;
        const auto* ptr = static_cast<const uint8_t*>(data);
        entry.binVal.assign(ptr, ptr + size);
        return 0;
    }

private:
    void initDefaults() {
        addDefaultInt(REG_KEY_VERSION, 0x03200000);
        addDefaultInt(REG_KEY_VERSION_PS4, 0x09000000);
        addDefaultInt(REG_KEY_UPDATE_MODE, 0);
        addDefaultInt(REG_KEY_LANGUAGE, 1);
        addDefaultStr(REG_KEY_NICKNAME, "PS5");
        addDefaultInt(REG_KEY_DIMMER_INTERVAL, 15);
        addDefaultInt(REG_KEY_AUTO_POWER_DOWN, 60);
        addDefaultInt(REG_KEY_INIT_USER, 1);
    }

    void addDefaultInt(uint32_t key, int32_t val) {
        RegEntry e;
        e.type = TypeInt;
        e.intVal = val;
        e.hasInit = true;
        e.initType = TypeInt;
        e.initIntVal = val;
        entries_[key] = e;
    }

    void addDefaultStr(uint32_t key, const std::string& val) {
        RegEntry e;
        e.type = TypeStr;
        e.strVal = val;
        e.hasInit = true;
        e.initType = TypeStr;
        e.initStrVal = val;
        entries_[key] = e;
    }

    std::mutex mutex_;
    std::unordered_map<uint32_t, RegEntry> entries_;
    std::unordered_map<int64_t, RegEntry> nonSysEntries_;
};

RegistryStore gStore;

} // namespace

extern "C" {

int APS5_VABI sceRegMgrGetBin(std::uint32_t optionId, void* data, std::size_t size) {
    return gStore.getBin(optionId, data, size);
}

int APS5_VABI sceRegMgrGetInt(int32_t optionId, int32_t* outInt) {
    return gStore.getInt(static_cast<uint32_t>(optionId), outInt);
}

int APS5_VABI sceRegMgrGetStr(int32_t optionId, char* outString, std::size_t stringLength) {
    return gStore.getStr(static_cast<uint32_t>(optionId), outString, stringLength);
}

int APS5_VABI sceRegMgrSetBin(std::uint32_t optionId, const void* data, std::size_t size) {
    return gStore.setBin(optionId, data, size);
}

int APS5_VABI sceRegMgrSetInt(int32_t optionId, int32_t inInt) {
    return gStore.setInt(static_cast<uint32_t>(optionId), inInt);
}

int APS5_VABI sceRegMgrSetStr(int32_t optionId, const char* inString, std::size_t stringLength) {
    return gStore.setStr(static_cast<uint32_t>(optionId), inString, stringLength);
}

std::int32_t APS5_VABI sceRegMgrNonSysGetInt(std::int64_t key, std::int32_t unk, std::int32_t* data) {
    (void)unk;
    return gStore.nonSysGetInt(key, data);
}

int APS5_VABI sceRegMgrCheckError(int error) {
    return error == 0 ? 1 : 0;
}

int APS5_VABI sceRegMgrCheckIntVal(int val) {
    return val >= 0 ? 0 : SCE_REG_MGR_ERROR_INVALID_PARAM;
}

int APS5_VABI sceRegMgrGetBinInitVal(std::uint32_t optionId, void* data, std::size_t size) {
    return gStore.getBinInitVal(optionId, data, size);
}

int APS5_VABI sceRegMgrGetIntInitVal(int32_t optionId, int32_t* outInt) {
    return gStore.getIntInitVal(static_cast<uint32_t>(optionId), outInt);
}

int APS5_VABI sceRegMgrGetStrInitVal(int32_t optionId, char* outString, std::size_t stringLength) {
    return gStore.getStrInitVal(static_cast<uint32_t>(optionId), outString, stringLength);
}

int APS5_VABI sceRegMgrGetVersion(void) {
    return 0x03200000;
}

int APS5_VABI sceRegMgrIsChange(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrEvtGetCnt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrEvtGetRegId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrResetVal(std::uint32_t optionId) {
    return gStore.resetVal(optionId);
}

int APS5_VABI sceRegMgrSetInitLevel(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrSrvCnvRegionInt(const char* str) {
    if (str == nullptr) {
        return -1;
    }
    if (std::strcmp(str, "JP") == 0) return 0;
    if (std::strcmp(str, "US") == 0) return 1;
    if (std::strcmp(str, "EU") == 0) return 2;
    if (std::strcmp(str, "KR") == 0) return 3;
    if (std::strcmp(str, "AS") == 0) return 4;
    if (std::strcmp(str, "RU") == 0) return 5;
    if (std::strcmp(str, "CN") == 0) return 6;
    return -1;
}

const char* APS5_VABI sceRegMgrSrvCnvRegionStr(int region) {
    switch (region) {
        case 0: return "JP";
        case 1: return "US";
        case 2: return "EU";
        case 3: return "KR";
        case 4: return "AS";
        case 5: return "RU";
        case 6: return "CN";
        default: return "";
    }
}

int APS5_VABI sceRegMgrSrvGetMachineType(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrSrvGetQAFforReg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrSrvGetRealMachineType(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrSrvGetRegion(void) {
    return 1;
}

const char* APS5_VABI sceRegMgrSrvGetRegionStr(void) {
    return "US";
}

int APS5_VABI sceRegMgrBackupNeedMem(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrBackupPullData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrBackupPushData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrBackupPullDataGkp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrBackupPushDataGkp(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrCntlStart(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrCntlDeleteReg(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrCntlGetFilesCount(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrCntlGetFileName(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrCntSetInitReinst(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrIsInitOK(void) {
    return 1;
}

int APS5_VABI sceRegMgrGetInitError(void) {
    return 0;
}

int APS5_VABI sceRegMgrRecoverRegNvs(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrNonSysCheckError(int error) {
    return error == 0 ? 1 : 0;
}

int APS5_VABI sceRegMgrNonSysGetBin(std::int64_t key, std::int32_t unk, void* data, std::size_t size) {
    (void)unk;
    return gStore.nonSysGetBin(key, data, size);
}

int APS5_VABI sceRegMgrNonSysGetStr(std::int64_t key, std::int32_t unk, char* outString, std::size_t stringLength) {
    (void)unk;
    return gStore.nonSysGetStr(key, outString, stringLength);
}

int APS5_VABI sceRegMgrNonSysSetBin(std::int64_t key, std::int32_t unk, const void* data, std::size_t size) {
    (void)unk;
    return gStore.nonSysSetBin(key, data, size);
}

int APS5_VABI sceRegMgrNonSysSetInt(std::int64_t key, std::int32_t unk, int32_t inInt) {
    (void)unk;
    return gStore.nonSysSetInt(key, inInt);
}

int APS5_VABI sceRegMgrNonSysSetStr(std::int64_t key, std::int32_t unk, const char* inString, std::size_t stringLength) {
    (void)unk;
    return gStore.nonSysSetStr(key, inString, stringLength);
}

int APS5_VABI sceRegMgrGetVersionForPS4(void) {
    return 0x09000000;
}

int APS5_VABI sceRegMgrBackupNeedMemForPS4(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrBackupPullDataForPS4(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrBackupPushDataForPS4(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrEvtGetCntForPS4(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrEvtGetRegIdForPS4(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

char* APS5_VABI sceRegMgrStrncpy(char* dst, const char* src, std::size_t len) {
    if (dst == nullptr) {
        return nullptr;
    }
    if (src == nullptr || len == 0) {
        if (len > 0) dst[0] = '\0';
        return dst;
    }
    std::size_t i = 0;
    for (; i < len - 1 && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
    return dst;
}

void* APS5_VABI sceRegMgrMemset(void* dst, int val, std::size_t len) {
    if (dst == nullptr) {
        return nullptr;
    }
    return std::memset(dst, val, len);
}

int APS5_VABI sceRegMgrSetbin(std::uint32_t optionId, const void* data, std::size_t size) {
    return gStore.setBin(optionId, data, size);
}

int APS5_VABI sceRegMgrToolGetUpdateCnt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrToolGetEntryCnt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrToolDataCheckGet(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrDrvGetUpdateCnt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrDrvGetEntCnt(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrDrvDataOpen(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrDrvDataCheckGet(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrDrvDataClose(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrToolGetInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrLogPull(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrCntlStopHdd(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRegMgrPrintInfo(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}

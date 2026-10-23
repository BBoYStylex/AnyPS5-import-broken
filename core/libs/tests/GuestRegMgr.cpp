#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" {
int APS5_VABI sceRegMgrGetBin(std::uint32_t optionId, void* data, std::size_t size);
int APS5_VABI sceRegMgrGetInt(int32_t optionId, int32_t* outInt);
int APS5_VABI sceRegMgrGetStr(int32_t optionId, char* outString, std::size_t stringLength);
int APS5_VABI sceRegMgrSetBin(std::uint32_t optionId, const void* data, std::size_t size);
int APS5_VABI sceRegMgrSetInt(int32_t optionId, int32_t inInt);
int APS5_VABI sceRegMgrSetStr(int32_t optionId, const char* inString, std::size_t stringLength);
int APS5_VABI sceRegMgrSetbin(std::uint32_t optionId, const void* data, std::size_t size);
int APS5_VABI sceRegMgrGetIntInitVal(int32_t optionId, int32_t* outInt);
int APS5_VABI sceRegMgrGetStrInitVal(int32_t optionId, char* outString, std::size_t stringLength);
int APS5_VABI sceRegMgrGetBinInitVal(std::uint32_t optionId, void* data, std::size_t size);
int APS5_VABI sceRegMgrResetVal(std::uint32_t optionId);
std::int32_t APS5_VABI sceRegMgrNonSysGetInt(std::int64_t key, std::int32_t unk, std::int32_t* data);
int APS5_VABI sceRegMgrNonSysGetStr(std::int64_t key, std::int32_t unk, char* outString, std::size_t stringLength);
int APS5_VABI sceRegMgrNonSysGetBin(std::int64_t key, std::int32_t unk, void* data, std::size_t size);
int APS5_VABI sceRegMgrNonSysSetInt(std::int64_t key, std::int32_t unk, int32_t inInt);
int APS5_VABI sceRegMgrNonSysSetStr(std::int64_t key, std::int32_t unk, const char* inString, std::size_t stringLength);
int APS5_VABI sceRegMgrNonSysSetBin(std::int64_t key, std::int32_t unk, const void* data, std::size_t size);
int APS5_VABI sceRegMgrCheckError(int error);
int APS5_VABI sceRegMgrNonSysCheckError(int error);
int APS5_VABI sceRegMgrCheckIntVal(int val);
int APS5_VABI sceRegMgrIsInitOK(void);
int APS5_VABI sceRegMgrGetInitError(void);
int APS5_VABI sceRegMgrGetVersion(void);
int APS5_VABI sceRegMgrGetVersionForPS4(void);
int APS5_VABI sceRegMgrSrvGetRegion(void);
const char* APS5_VABI sceRegMgrSrvGetRegionStr(void);
int APS5_VABI sceRegMgrSrvCnvRegionInt(const char* str);
const char* APS5_VABI sceRegMgrSrvCnvRegionStr(int region);
char* APS5_VABI sceRegMgrStrncpy(char* dst, const char* src, std::size_t len);
void* APS5_VABI sceRegMgrMemset(void* dst, int val, std::size_t len);
int APS5_VABI sceRegMgrCntlStart(void);
int APS5_VABI sceRegMgrBackupNeedMem(void);
int APS5_VABI sceRegMgrIsChange(void);
}

static void Require(bool value) {
    if (!value) std::abort();
}

template <typename TCall>
static bool ReportsNotImplemented(const char* name, TCall call) {
    try {
        call();
    } catch (const std::runtime_error& error) {
        return std::string(error.what()) == std::string(name) + " not implemented";
    }
    return false;
}

int main() {
    int32_t intVal = 0;
    char strBuf[64] = {};
    uint8_t binBuf[32] = {};

    Require(sceRegMgrGetInt(17825792, &intVal) == 0);
    Require(intVal == 0x03200000);
    Require(sceRegMgrGetInt(16842752, &intVal) == 0);
    Require(intVal == 0x09000000);
    Require(sceRegMgrGetInt(33685504, &intVal) == 0);
    Require(intVal == 1);
    Require(sceRegMgrGetStr(33882112, strBuf, sizeof(strBuf)) == 0);
    Require(std::strcmp(strBuf, "PS5") == 0);
    Require(sceRegMgrGetInt(33619968, &intVal) == 0);
    Require(intVal == 0);
    Require(sceRegMgrGetInt(33947648, &intVal) == 0);
    Require(intVal == 15);
    Require(sceRegMgrGetInt(42075904, &intVal) == 0);
    Require(intVal == 60);
    Require(sceRegMgrGetInt(117637120, &intVal) == 0);
    Require(intVal == 1);

    Require(sceRegMgrGetIntInitVal(33685504, &intVal) == 0);
    Require(intVal == 1);
    Require(sceRegMgrGetStrInitVal(33882112, strBuf, sizeof(strBuf)) == 0);
    Require(std::strcmp(strBuf, "PS5") == 0);
    Require(sceRegMgrGetIntInitVal(999999, &intVal) == static_cast<int>(0x80820002));

    Require(sceRegMgrSetInt(0x5000, 12345) == 0);
    Require(sceRegMgrGetInt(0x5000, &intVal) == 0);
    Require(intVal == 12345);

    Require(sceRegMgrSetStr(0x5001, "CustomTest", 0) == 0);
    Require(sceRegMgrGetStr(0x5001, strBuf, sizeof(strBuf)) == 0);
    Require(std::strcmp(strBuf, "CustomTest") == 0);

    const uint8_t sampleBin[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    Require(sceRegMgrSetBin(0x5002, sampleBin, sizeof(sampleBin)) == 0);
    Require(sceRegMgrGetBin(0x5002, binBuf, sizeof(binBuf)) == 0);
    Require(std::memcmp(binBuf, sampleBin, sizeof(sampleBin)) == 0);

    const uint8_t sampleBin2[4] = {0xCA, 0xFE, 0xBA, 0xBE};
    Require(sceRegMgrSetbin(0x5003, sampleBin2, sizeof(sampleBin2)) == 0);
    Require(sceRegMgrGetBin(0x5003, binBuf, sizeof(binBuf)) == 0);
    Require(std::memcmp(binBuf, sampleBin2, sizeof(sampleBin2)) == 0);

    Require(sceRegMgrSetInt(33685504, 42) == 0);
    Require(sceRegMgrGetInt(33685504, &intVal) == 0);
    Require(intVal == 42);
    Require(sceRegMgrResetVal(33685504) == 0);
    Require(sceRegMgrGetInt(33685504, &intVal) == 0);
    Require(intVal == 1);

    Require(sceRegMgrSetStr(33685504, "wrong-type", 0) == 0);
    Require(sceRegMgrGetStr(33685504, strBuf, sizeof(strBuf)) == 0);
    Require(std::strcmp(strBuf, "wrong-type") == 0);
    Require(sceRegMgrGetInt(33685504, &intVal) == static_cast<int>(0x80820006));
    Require(sceRegMgrGetIntInitVal(33685504, &intVal) == 0);
    Require(intVal == 1);
    Require(sceRegMgrResetVal(33685504) == 0);
    Require(sceRegMgrGetInt(33685504, &intVal) == 0);
    Require(intVal == 1);
    Require(sceRegMgrGetIntInitVal(33685504, &intVal) == 0);
    Require(intVal == 1);
    Require(sceRegMgrGetStr(33685504, strBuf, sizeof(strBuf)) == static_cast<int>(0x80820006));

    Require(sceRegMgrSetBin(33685504, sampleBin, sizeof(sampleBin)) == 0);
    Require(sceRegMgrGetBin(33685504, binBuf, sizeof(binBuf)) == 0);
    Require(sceRegMgrGetIntInitVal(33685504, &intVal) == 0);
    Require(intVal == 1);
    Require(sceRegMgrResetVal(33685504) == 0);
    Require(sceRegMgrGetInt(33685504, &intVal) == 0);
    Require(intVal == 1);

    Require(sceRegMgrSetInt(33882112, 999) == 0);
    Require(sceRegMgrGetInt(33882112, &intVal) == 0);
    Require(intVal == 999);
    Require(sceRegMgrGetStrInitVal(33882112, strBuf, sizeof(strBuf)) == 0);
    Require(std::strcmp(strBuf, "PS5") == 0);
    Require(sceRegMgrResetVal(33882112) == 0);
    Require(sceRegMgrGetStr(33882112, strBuf, sizeof(strBuf)) == 0);
    Require(std::strcmp(strBuf, "PS5") == 0);
    Require(sceRegMgrGetInt(33882112, &intVal) == static_cast<int>(0x80820006));

    Require(sceRegMgrResetVal(0x5000) == 0);
    Require(sceRegMgrGetInt(0x5000, &intVal) == static_cast<int>(0x80820002));

    Require(sceRegMgrNonSysSetInt(0x1000, 0, 777) == 0);
    Require(sceRegMgrNonSysGetInt(0x1000, 0, &intVal) == 0);
    Require(intVal == 777);

    Require(sceRegMgrNonSysSetStr(0x1001, 0, "NonSysValue", 0) == 0);
    Require(sceRegMgrNonSysGetStr(0x1001, 0, strBuf, sizeof(strBuf)) == 0);
    Require(std::strcmp(strBuf, "NonSysValue") == 0);

    Require(sceRegMgrNonSysSetBin(0x1002, 0, sampleBin, sizeof(sampleBin)) == 0);
    Require(sceRegMgrNonSysGetBin(0x1002, 0, binBuf, sizeof(binBuf)) == 0);
    Require(std::memcmp(binBuf, sampleBin, sizeof(sampleBin)) == 0);

    Require(sceRegMgrGetInt(17825792, nullptr) == static_cast<int>(0x80820001));
    Require(sceRegMgrGetStr(33882112, nullptr, 10) == static_cast<int>(0x80820001));
    Require(sceRegMgrGetStr(33882112, strBuf, 0) == static_cast<int>(0x80820001));
    Require(sceRegMgrGetStr(33882112, strBuf, 3) == static_cast<int>(0x80820004));
    Require(sceRegMgrGetBin(0x5002, nullptr, 10) == static_cast<int>(0x80820001));
    Require(sceRegMgrGetBin(0x5002, binBuf, 0) == static_cast<int>(0x80820001));
    Require(sceRegMgrGetBin(0x5002, binBuf, 2) == static_cast<int>(0x80820004));

    Require(sceRegMgrSetStr(0x5001, nullptr, 0) == static_cast<int>(0x80820001));
    Require(sceRegMgrSetBin(0x5002, nullptr, 4) == static_cast<int>(0x80820001));
    Require(sceRegMgrSetBin(0x5002, sampleBin, 0) == static_cast<int>(0x80820001));

    Require(sceRegMgrNonSysGetInt(0x1000, 0, nullptr) == static_cast<int>(0x80820001));
    Require(sceRegMgrNonSysGetStr(0x1001, 0, nullptr, 10) == static_cast<int>(0x80820001));
    Require(sceRegMgrNonSysGetStr(0x1001, 0, strBuf, 0) == static_cast<int>(0x80820001));
    Require(sceRegMgrNonSysGetBin(0x1002, 0, nullptr, 4) == static_cast<int>(0x80820001));
    Require(sceRegMgrNonSysGetBin(0x1002, 0, binBuf, 0) == static_cast<int>(0x80820001));
    Require(sceRegMgrNonSysSetStr(0x1001, 0, nullptr, 0) == static_cast<int>(0x80820001));
    Require(sceRegMgrNonSysSetBin(0x1002, 0, nullptr, 4) == static_cast<int>(0x80820001));
    Require(sceRegMgrNonSysSetBin(0x1002, 0, sampleBin, 0) == static_cast<int>(0x80820001));

    Require(sceRegMgrGetInt(33882112, &intVal) == static_cast<int>(0x80820006));
    Require(sceRegMgrGetStr(17825792, strBuf, sizeof(strBuf)) == static_cast<int>(0x80820006));
    Require(sceRegMgrGetInt(888888, &intVal) == static_cast<int>(0x80820002));
    Require(sceRegMgrGetStr(888888, strBuf, sizeof(strBuf)) == static_cast<int>(0x80820002));
    Require(sceRegMgrGetBin(888888, binBuf, sizeof(binBuf)) == static_cast<int>(0x80820002));
    Require(sceRegMgrNonSysGetInt(0x8888, 0, &intVal) == static_cast<int>(0x80820002));
    Require(sceRegMgrNonSysGetStr(0x8888, 0, strBuf, sizeof(strBuf)) == static_cast<int>(0x80820002));
    Require(sceRegMgrNonSysGetBin(0x8888, 0, binBuf, sizeof(binBuf)) == static_cast<int>(0x80820002));

    Require(sceRegMgrCheckError(0) == 1);
    Require(sceRegMgrCheckError(-1) == 0);
    Require(sceRegMgrCheckError(static_cast<int>(0x80820001)) == 0);
    Require(sceRegMgrNonSysCheckError(0) == 1);
    Require(sceRegMgrNonSysCheckError(-1) == 0);
    Require(sceRegMgrCheckIntVal(10) == 0);
    Require(sceRegMgrCheckIntVal(-5) == static_cast<int>(0x80820001));

    Require(sceRegMgrIsInitOK() == 1);
    Require(sceRegMgrGetInitError() == 0);
    Require(sceRegMgrGetVersion() == 0x03200000);
    Require(sceRegMgrGetVersionForPS4() == 0x09000000);

    Require(sceRegMgrSrvGetRegion() == 1);
    Require(std::strcmp(sceRegMgrSrvGetRegionStr(), "US") == 0);
    Require(sceRegMgrSrvCnvRegionInt("JP") == 0);
    Require(sceRegMgrSrvCnvRegionInt("US") == 1);
    Require(sceRegMgrSrvCnvRegionInt("EU") == 2);
    Require(sceRegMgrSrvCnvRegionInt("KR") == 3);
    Require(sceRegMgrSrvCnvRegionInt("AS") == 4);
    Require(sceRegMgrSrvCnvRegionInt("RU") == 5);
    Require(sceRegMgrSrvCnvRegionInt("CN") == 6);
    Require(sceRegMgrSrvCnvRegionInt("UNKNOWN") == -1);
    Require(sceRegMgrSrvCnvRegionInt(nullptr) == -1);

    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(0), "JP") == 0);
    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(1), "US") == 0);
    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(2), "EU") == 0);
    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(3), "KR") == 0);
    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(4), "AS") == 0);
    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(5), "RU") == 0);
    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(6), "CN") == 0);
    Require(std::strcmp(sceRegMgrSrvCnvRegionStr(99), "") == 0);

    char copyBuf[16] = {};
    Require(sceRegMgrStrncpy(copyBuf, "Hello", sizeof(copyBuf)) == copyBuf);
    Require(std::strcmp(copyBuf, "Hello") == 0);
    Require(sceRegMgrStrncpy(copyBuf, "LongStringToTruncate", 5) == copyBuf);
    Require(std::strcmp(copyBuf, "Long") == 0);
    Require(sceRegMgrStrncpy(nullptr, "Hello", 5) == nullptr);

    uint8_t memBuf[8] = {};
    Require(sceRegMgrMemset(memBuf, 0xAB, sizeof(memBuf)) == memBuf);
    Require(memBuf[0] == 0xAB && memBuf[7] == 0xAB);
    Require(sceRegMgrMemset(nullptr, 0, 8) == nullptr);

    Require(ReportsNotImplemented("sceRegMgrCntlStart", [] { sceRegMgrCntlStart(); }));
    Require(ReportsNotImplemented("sceRegMgrBackupNeedMem", [] { sceRegMgrBackupNeedMem(); }));
    Require(ReportsNotImplemented("sceRegMgrIsChange", [] { sceRegMgrIsChange(); }));

    return 0;
}

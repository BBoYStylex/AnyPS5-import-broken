#include "SceTypes.hpp"
#include "prx/libSceSystemService/SystemService.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>

extern "C" int APS5_VABI sceSystemServiceParamGetInt(int paramId, int* value);

namespace {

void Require(bool value) { if (!value) std::abort(); }

void SetLanguage(const char* value) {
#ifdef _WIN32
    Require(_putenv_s("ANYPS5_LANGUAGE", value != nullptr ? value : "") == 0);
#else
    Require((value != nullptr ? ::setenv("ANYPS5_LANGUAGE", value, 1) : ::unsetenv("ANYPS5_LANGUAGE")) == 0);
#endif
}

int Language() {
    int value = -1;
    Require(sceSystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_LANG, &value) == SYSTEM_SERVICE_OK);
    int sameValue = -1;
    Require(sceSystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_400, &sameValue) == SYSTEM_SERVICE_OK);
    Require(sameValue == value);
    return value;
}

bool ParameterRejected(int paramId) {
    int language = -1;
    try {
        static_cast<void>(sceSystemServiceParamGetInt(paramId, &language));
    } catch (const std::runtime_error& error) {
        return language == -1 && std::string(error.what()).find("ANYPS5_LANGUAGE") != std::string::npos;
    }
    return false;
}

bool Rejected(const char* value) {
    SetLanguage(value);
    return ParameterRejected(SYSTEM_SERVICE_PARAM_ID_LANG) && ParameterRejected(SYSTEM_SERVICE_PARAM_ID_400);
}

}

int main() {
    for (int paramId = -1; paramId <= 1001; ++paramId) {
        Require(sceSystemServiceParamGetInt(paramId, nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER);
        if ((paramId >= 1 && paramId <= 5) || paramId == SYSTEM_SERVICE_PARAM_ID_100 || paramId == 400) continue;
        int value = -12345;
        if ((paramId >= 101 && paramId <= 111) || (paramId >= 200 && paramId <= 213) || paramId == 300) {
            static constexpr int settings101To111[] = {1, 2, 100, 0x20202, 0, 0, 100, 0, 100, 0x20202, 0};
            static constexpr int settings200To213[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 71, 0, 0, 0};
            const int expected = paramId <= 111 ? settings101To111[paramId - 101] : paramId <= 213 ? settings200To213[paramId - 200] : 0;
            Require(sceSystemServiceParamGetInt(paramId, &value) == SYSTEM_SERVICE_OK && value == expected);
            continue;
        }
        Require(sceSystemServiceParamGetInt(paramId, &value) == SYSTEM_SERVICE_ERROR_PARAMETER);
        Require(value == -12345);
    }
    for (int paramId : {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        int value = -12345;
        Require(sceSystemServiceParamGetInt(paramId, &value) == SYSTEM_SERVICE_ERROR_PARAMETER);
        Require(value == -12345);
    }
    SetLanguage(nullptr);
    Require(Language() == SYSTEM_SERVICE_PARAM_LANG_ENGLISH_US);
    SetLanguage("");
    Require(Language() == SYSTEM_SERVICE_PARAM_LANG_ENGLISH_US);
    SetLanguage("21");
    Require(Language() == SYSTEM_SERVICE_PARAM_LANG_ARABIC);
    SetLanguage("0");
    Require(Language() == SYSTEM_SERVICE_PARAM_LANG_JAPANESE);
    SetLanguage("30");
    Require(Language() == SYSTEM_SERVICE_PARAM_LANG_UKRAINIAN);
    for (const char* value : {"31", "-1", "+21", " 21", "21 ", "21a", "0x15", "arabic", "4294967317"}) {
        Require(Rejected(value));
    }

    SetLanguage("arabic");
    Require(sceSystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_LANG, nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER);
    Require(sceSystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_400, nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER);
    for (int paramId : {0, SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME, SYSTEM_SERVICE_PARAM_ID_GAME_PARENTAL_LEVEL, 8, 20, 401, 999,
                        SYSTEM_SERVICE_PARAM_ID_ENTER_BUTTON_ASSIGN, 1001, 1008}) {
        int unknown = -1;
        Require(sceSystemServiceParamGetInt(paramId, &unknown) == SYSTEM_SERVICE_ERROR_PARAMETER);
        Require(unknown == -1);
        Require(sceSystemServiceParamGetInt(paramId, nullptr) == SYSTEM_SERVICE_ERROR_PARAMETER);
    }
    int param100 = -1;
    Require(sceSystemServiceParamGetInt(SYSTEM_SERVICE_PARAM_ID_100, &param100) == SYSTEM_SERVICE_OK);
    Require(param100 == 0);
}

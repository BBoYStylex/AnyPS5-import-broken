```cpp
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Libraries::SaveDataDialog {

using s32 = std::int32_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

constexpr s32 SCE_OK = 0;
constexpr s32 SCE_COMMON_DIALOG_ERROR_NOT_SYSTEM_INITIALIZED = static_cast<s32>(0x80B80001);
constexpr s32 SCE_COMMON_DIALOG_ERROR_ALREADY_SYSTEM_INITIALIZED = static_cast<s32>(0x80B80002);
constexpr s32 SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED = static_cast<s32>(0x80B80003);
constexpr s32 SCE_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED = static_cast<s32>(0x80B80004);
constexpr s32 SCE_COMMON_DIALOG_ERROR_NOT_FINISHED = static_cast<s32>(0x80B80005);
constexpr s32 SCE_COMMON_DIALOG_ERROR_INVALID_STATE = static_cast<s32>(0x80B80006);
constexpr s32 SCE_COMMON_DIALOG_ERROR_RESULT_NONE = static_cast<s32>(0x80B80007);
constexpr s32 SCE_COMMON_DIALOG_ERROR_BUSY = static_cast<s32>(0x80B80008);
constexpr s32 SCE_COMMON_DIALOG_ERROR_PARAM_INVALID = static_cast<s32>(0x80B8000A);
constexpr s32 SCE_COMMON_DIALOG_ERROR_NOT_RUNNING = static_cast<s32>(0x80B8000B);
constexpr s32 SCE_COMMON_DIALOG_ERROR_ARG_NULL = static_cast<s32>(0x80B8000C);

constexpr u32 SCE_SAVE_DATA_DIALOG_DIRNAME_DATA_MAXSIZE = 32;
constexpr u32 SCE_SAVE_DATA_DIALOG_DIRNAME_MAX_COUNT = 1024;
constexpr u32 SCE_SAVE_DATA_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT = 0;
constexpr u32 SCE_SAVE_DATA_DIALOG_PROGRESSBAR_MAX_VALUE = 100;

enum SceSaveDataDialogMode : u32 {
    SCE_SAVE_DATA_DIALOG_MODE_INVALID = 0,
    SCE_SAVE_DATA_DIALOG_MODE_LIST = 1,
    SCE_SAVE_DATA_DIALOG_MODE_USER_MSG = 2,
    SCE_SAVE_DATA_DIALOG_MODE_SYSTEM_MSG = 3,
    SCE_SAVE_DATA_DIALOG_MODE_ERROR_CODE = 4,
    SCE_SAVE_DATA_DIALOG_MODE_PROGRESS_BAR = 5,
};

enum SceCommonDialogStatus : u32 {
    SCE_COMMON_DIALOG_STATUS_NONE = 0,
    SCE_COMMON_DIALOG_STATUS_INITIALIZED = 1,
    SCE_COMMON_DIALOG_STATUS_RUNNING = 2,
    SCE_COMMON_DIALOG_STATUS_FINISHED = 3,
};

enum SceSaveDataDialogButtonType : u32 {
    SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_YESNO = 0,
    SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OK = 1,
    SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_NONE = 2,
    SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OKCANCEL = 3,
};

enum SceSaveDataDialogButtonId : u32 {
    SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID = 0,
    SCE_SAVE_DATA_DIALOG_BUTTON_ID_OK = 1,
    SCE_SAVE_DATA_DIALOG_BUTTON_ID_YES = 1,
    SCE_SAVE_DATA_DIALOG_BUTTON_ID_NO = 2,
};

enum SceSaveDataDialogResult : s32 {
    SCE_SAVE_DATA_DIALOG_RESULT_OK = 0,
    SCE_SAVE_DATA_DIALOG_RESULT_USER_CANCELED = 1,
};

struct SceSaveDataDialogDirName {
    char data[SCE_SAVE_DATA_DIALOG_DIRNAME_DATA_MAXSIZE];
};

struct SceSaveDataDialogItems {
    s32 userId;
    const char* titleId;
    const SceSaveDataDialogDirName* dirName;
    u32 dirNameNum;
    u32 focusPos;
    u32 itemStyle;
    u32 reserved;
};

struct SceSaveDataDialogUserMessageParam {
    u32 buttonType;
    u32 msgType;
    const char* msg;
};

struct SceSaveDataDialogSystemMessageParam {
    u32 sysMsgType;
    u64 value;
};

struct SceSaveDataDialogErrorCodeParam {
    s32 errorCode;
};

struct SceSaveDataDialogProgressBarParam {
    u32 barType;
    u32 sysMsgType;
    const char* msg;
};

struct SceSaveDataDialogParam {
    u64 size;
    u32 mode;
    u32 dispType;
    s32 userId;
    const SceSaveDataDialogItems* items;
    const SceSaveDataDialogUserMessageParam* userMsgParam;
    const SceSaveDataDialogSystemMessageParam* sysMsgParam;
    const SceSaveDataDialogErrorCodeParam* errorCodeParam;
    const SceSaveDataDialogProgressBarParam* progressBarParam;
    void* userData;
};

struct SceSaveDataDialogResultData {
    u32 mode;
    s32 result;
    u32 buttonId;
    SceSaveDataDialogDirName* dirName;
    void* userData;
};

struct SaveDataDialogView {
    u32 mode;
    u32 itemCount;
    u32 focusPos;
    u32 buttonType;
};

struct SaveDataDialogDecision {
    bool cancelled;
    u32 buttonId;
    u32 selectedIndex;
};

using SaveDataDialogInputProvider =
    std::function<std::optional<SaveDataDialogDecision>(const SaveDataDialogView&)>;

struct SaveDataDialogTimings {
    std::chrono::milliseconds minimumDisplay{400};
    std::chrono::milliseconds interactionTimeout{60000};
    std::chrono::milliseconds progressStallTimeout{30000};
};

class SaveDataDialogContext {
public:
    static SaveDataDialogContext& Instance() {
        static SaveDataDialogContext instance;
        return instance;
    }

    s32 Initialize() {
        std::lock_guard lock(m_mutex);
        if (m_status != SCE_COMMON_DIALOG_STATUS_NONE) {
            return SCE_COMMON_DIALOG_ERROR_ALREADY_INITIALIZED;
        }
        m_status = SCE_COMMON_DIALOG_STATUS_INITIALIZED;
        return SCE_OK;
    }

    s32 Terminate() {
        std::lock_guard lock(m_mutex);
        if (m_status == SCE_COMMON_DIALOG_STATUS_NONE) {
            return SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
        }
        Evaluate(Clock::now());
        if (m_status == SCE_COMMON_DIALOG_STATUS_RUNNING) {
            return SCE_COMMON_DIALOG_ERROR_NOT_FINISHED;
        }
        m_session = Session{};
        m_status = SCE_COMMON_DIALOG_STATUS_NONE;
        return SCE_OK;
    }

    s32 Open(const SceSaveDataDialogParam* param) {
        if (param == nullptr) {
            return SCE_COMMON_DIALOG_ERROR_ARG_NULL;
        }
        std::lock_guard lock(m_mutex);
        if (m_status == SCE_COMMON_DIALOG_STATUS_NONE) {
            return SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
        }
        Evaluate(Clock::now());
        if (m_status == SCE_COMMON_DIALOG_STATUS_RUNNING) {
            return SCE_COMMON_DIALOG_ERROR_BUSY;
        }
        if (m_status != SCE_COMMON_DIALOG_STATUS_INITIALIZED) {
            return SCE_COMMON_DIALOG_ERROR_INVALID_STATE;
        }
        Session session{};
        const s32 validation = BuildSession(*param, session);
        if (validation != SCE_OK) {
            return validation;
        }
        session.openedAt = Clock::now();
        session.lastProgressAt = session.openedAt;
        m_session = std::move(session);
        m_status = SCE_COMMON_DIALOG_STATUS_RUNNING;
        return SCE_OK;
    }

    s32 Close() {
        std::lock_guard lock(m_mutex);
        if (m_status == SCE_COMMON_DIALOG_STATUS_NONE) {
            return SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
        }
        Evaluate(Clock::now());
        if (m_status != SCE_COMMON_DIALOG_STATUS_RUNNING) {
            return SCE_COMMON_DIALOG_ERROR_NOT_RUNNING;
        }
        if (m_session.mode == SCE_SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) {
            FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_OK, SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID,
                       std::nullopt);
        } else {
            FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_USER_CANCELED,
                       SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID, std::nullopt);
        }
        return SCE_OK;
    }

    u32 UpdateStatus() {
        std::lock_guard lock(m_mutex);
        Evaluate(Clock::now());
        return m_status;
    }

    u32 GetStatus() {
        return UpdateStatus();
    }

    s32 GetResult(SceSaveDataDialogResultData* out) {
        if (out == nullptr) {
            return SCE_COMMON_DIALOG_ERROR_ARG_NULL;
        }
        std::lock_guard lock(m_mutex);
        if (m_status == SCE_COMMON_DIALOG_STATUS_NONE) {
            return SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
        }
        Evaluate(Clock::now());
        if (m_status == SCE_COMMON_DIALOG_STATUS_RUNNING) {
            return SCE_COMMON_DIALOG_ERROR_NOT_FINISHED;
        }
        if (m_status != SCE_COMMON_DIALOG_STATUS_FINISHED) {
            return SCE_COMMON_DIALOG_ERROR_RESULT_NONE;
        }
        if (m_session.selectedDirName.has_value() && out->dirName == nullptr) {
            return SCE_COMMON_DIALOG_ERROR_ARG_NULL;
        }
        out->mode = m_session.mode;
        out->result = m_session.resultCode;
        out->buttonId = m_session.buttonId;
        out->userData = m_session.userData;
        if (m_session.selectedDirName.has_value()) {
            std::memset(out->dirName->data, 0, SCE_SAVE_DATA_DIALOG_DIRNAME_DATA_MAXSIZE);
            std::memcpy(out->dirName->data, m_session.selectedDirName->data(),
                        m_session.selectedDirName->size());
        }
        return SCE_OK;
    }

    s32 ProgressBarSetValue(u32 target, u32 value) {
        std::lock_guard lock(m_mutex);
        if (m_status != SCE_COMMON_DIALOG_STATUS_NONE) {
            Evaluate(Clock::now());
        }
        const s32 gate = RequireProgressMode(target);
        if (gate != SCE_OK) {
            return gate;
        }
        if (value > SCE_SAVE_DATA_DIALOG_PROGRESSBAR_MAX_VALUE) {
            return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
        }
        m_session.progress = value;
        m_session.lastProgressAt = Clock::now();
        return SCE_OK;
    }

    s32 ProgressBarInc(u32 target, u32 delta) {
        std::lock_guard lock(m_mutex);
        if (m_status != SCE_COMMON_DIALOG_STATUS_NONE) {
            Evaluate(Clock::now());
        }
        const s32 gate = RequireProgressMode(target);
        if (gate != SCE_OK) {
            return gate;
        }
        const u32 headroom = SCE_SAVE_DATA_DIALOG_PROGRESSBAR_MAX_VALUE - m_session.progress;
        m_session.progress += std::min(delta, headroom);
        m_session.lastProgressAt = Clock::now();
        return SCE_OK;
    }

    void SetInputProvider(SaveDataDialogInputProvider provider) {
        std::lock_guard lock(m_mutex);
        m_provider = std::move(provider);
    }

    void SetTimings(const SaveDataDialogTimings& timings) {
        std::lock_guard lock(m_mutex);
        if (m_status == SCE_COMMON_DIALOG_STATUS_RUNNING) {
            throw std::runtime_error("SaveDataDialog timings changed while dialog is running");
        }
        m_timings = timings;
    }

private:
    using Clock = std::chrono::steady_clock;
    using DirName = std::array<char, SCE_SAVE_DATA_DIALOG_DIRNAME_DATA_MAXSIZE>;

    struct Session {
        u32 mode{SCE_SAVE_DATA_DIALOG_MODE_INVALID};
        u32 buttonType{SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OK};
        u32 focusPos{0};
        u32 progress{0};
        void* userData{nullptr};
        std::vector<DirName> dirNames;
        Clock::time_point openedAt{};
        Clock::time_point lastProgressAt{};
        s32 resultCode{SCE_SAVE_DATA_DIALOG_RESULT_OK};
        u32 buttonId{SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID};
        std::optional<DirName> selectedDirName;
    };

    SaveDataDialogContext() = default;

    static bool IsEntryEmpty(const DirName& name) {
        return name[0] == '\0';
    }

    s32 BuildSession(const SceSaveDataDialogParam& param, Session& session) const {
        session.mode = param.mode;
        session.userData = param.userData;
        switch (param.mode) {
        case SCE_SAVE_DATA_DIALOG_MODE_LIST: {
            if (param.items == nullptr) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            const SceSaveDataDialogItems& items = *param.items;
            if (items.dirNameNum > SCE_SAVE_DATA_DIALOG_DIRNAME_MAX_COUNT) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            if (items.dirNameNum > 0 && items.dirName == nullptr) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            session.dirNames.reserve(items.dirNameNum);
            for (u32 i = 0; i < items.dirNameNum; ++i) {
                const char* raw = items.dirName[i].data;
                if (std::memchr(raw, '\0', SCE_SAVE_DATA_DIALOG_DIRNAME_DATA_MAXSIZE) == nullptr) {
                    return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
                }
                DirName copy{};
                std::memcpy(copy.data(), raw, copy.size());
                session.dirNames.push_back(copy);
            }
            session.focusPos = items.focusPos;
            session.buttonType = SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OK;
            return SCE_OK;
        }
        case SCE_SAVE_DATA_DIALOG_MODE_USER_MSG:
            if (param.userMsgParam == nullptr) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            if (param.userMsgParam->buttonType > SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OKCANCEL) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            session.buttonType = param.userMsgParam->buttonType;
            return SCE_OK;
        case SCE_SAVE_DATA_DIALOG_MODE_SYSTEM_MSG:
            if (param.sysMsgParam == nullptr) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            session.buttonType = SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OK;
            return SCE_OK;
        case SCE_SAVE_DATA_DIALOG_MODE_ERROR_CODE:
            if (param.errorCodeParam == nullptr) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            session.buttonType = SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OK;
            return SCE_OK;
        case SCE_SAVE_DATA_DIALOG_MODE_PROGRESS_BAR:
            if (param.progressBarParam == nullptr) {
                return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
            }
            session.buttonType = SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_NONE;
            session.progress = 0;
            return SCE_OK;
        default:
            return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
        }
    }

    s32 RequireProgressMode(u32 target) const {
        if (m_status == SCE_COMMON_DIALOG_STATUS_NONE) {
            return SCE_COMMON_DIALOG_ERROR_NOT_INITIALIZED;
        }
        if (m_status != SCE_COMMON_DIALOG_STATUS_RUNNING) {
            return SCE_COMMON_DIALOG_ERROR_NOT_RUNNING;
        }
        if (m_session.mode != SCE_SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) {
            return SCE_COMMON_DIALOG_ERROR_INVALID_STATE;
        }
        if (target != SCE_SAVE_DATA_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT) {
            return SCE_COMMON_DIALOG_ERROR_PARAM_INVALID;
        }
        return SCE_OK;
    }

    void FinishWith(s32 resultCode, u32 buttonId, std::optional<DirName> selected) {
        if (m_status != SCE_COMMON_DIALOG_STATUS_RUNNING) {
            throw std::runtime_error("SaveDataDialog finished from a non-running state");
        }
        m_session.resultCode = resultCode;
        m_session.buttonId = buttonId;
        m_session.selectedDirName = selected;
        m_status = SCE_COMMON_DIALOG_STATUS_FINISHED;
    }

    u32 DefaultAffirmativeButton() const {
        switch (m_session.buttonType) {
        case SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_YESNO:
            return SCE_SAVE_DATA_DIALOG_BUTTON_ID_YES;
        case SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OK:
        case SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_OKCANCEL:
            return SCE_SAVE_DATA_DIALOG_BUTTON_ID_OK;
        default:
            throw std::runtime_error("SaveDataDialog has no affirmative button for this type");
        }
    }

    std::optional<SaveDataDialogDecision> ResolveDecision() const {
        const SaveDataDialogView view{m_session.mode,
                                      static_cast<u32>(m_session.dirNames.size()),
                                      m_session.focusPos, m_session.buttonType};
        if (m_provider) {
            return m_provider(view);
        }
        if (m_session.mode == SCE_SAVE_DATA_DIALOG_MODE_LIST) {
            const auto& names = m_session.dirNames;
            if (m_session.focusPos < names.size() && !IsEntryEmpty(names[m_session.focusPos])) {
                return SaveDataDialogDecision{false, SCE_SAVE_DATA_DIALOG_BUTTON_ID_OK,
                                              m_session.focusPos};
            }
            for (u32 i = 0; i < names.size(); ++i) {
                if (!IsEntryEmpty(names[i])) {
                    return SaveDataDialogDecision{false, SCE_SAVE_DATA_DIALOG_BUTTON_ID_OK, i};
                }
            }
            return SaveDataDialogDecision{true, SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID, 0};
        }
        if (m_session.buttonType == SCE_SAVE_DATA_DIALOG_BUTTON_TYPE_NONE) {
            return std::nullopt;
        }
        return SaveDataDialogDecision{false, DefaultAffirmativeButton(), 0};
    }

    void ApplyDecision(const SaveDataDialogDecision& decision) {
        if (decision.cancelled) {
            FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_USER_CANCELED,
                       SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID, std::nullopt);
            return;
        }
        if (m_session.mode == SCE_SAVE_DATA_DIALOG_MODE_LIST) {
            if (decision.selectedIndex >= m_session.dirNames.size() ||
                IsEntryEmpty(m_session.dirNames[decision.selectedIndex])) {
                throw std::runtime_error("SaveDataDialog decision selected an invalid entry");
            }
            FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_OK, decision.buttonId,
                       m_session.dirNames[decision.selectedIndex]);
            return;
        }
        FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_OK, decision.buttonId, std::nullopt);
    }

    void Evaluate(Clock::time_point now) {
        if (m_status != SCE_COMMON_DIALOG_STATUS_RUNNING) {
            return;
        }
        if (m_session.mode == SCE_SAVE_DATA_DIALOG_MODE_INVALID) {
            throw std::runtime_error("SaveDataDialog running without an active session");
        }
        const auto displayed = now - m_session.openedAt;
        if (m_session.mode == SCE_SAVE_DATA_DIALOG_MODE_PROGRESS_BAR) {
            if (m_session.progress >= SCE_SAVE_DATA_DIALOG_PROGRESSBAR_MAX_VALUE &&
                displayed >= m_timings.minimumDisplay) {
                FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_OK, SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID,
                           std::nullopt);
                return;
            }
            if (now - m_session.lastProgressAt >= m_timings.progressStallTimeout) {
                FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_USER_CANCELED,
                           SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID, std::nullopt);
            }
            return;
        }
        if (displayed >= m_timings.minimumDisplay) {
            const std::optional<SaveDataDialogDecision> decision = ResolveDecision();
            if (decision.has_value()) {
                ApplyDecision(*decision);
                return;
            }
        }
        if (displayed >= m_timings.interactionTimeout) {
            FinishWith(SCE_SAVE_DATA_DIALOG_RESULT_USER_CANCELED,
                       SCE_SAVE_DATA_DIALOG_BUTTON_ID_INVALID, std::nullopt);
        }
    }

    std::mutex m_mutex;
    u32 m_status{SCE_COMMON_DIALOG_STATUS_NONE};
    Session m_session{};
    SaveDataDialogInputProvider m_provider{};
    SaveDataDialogTimings m_timings{};
};

} // namespace Libraries::SaveDataDialog

extern "C" {

Libraries::SaveDataDialog::s32 sceSaveDataDialogInitialize() {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().Initialize();
}

Libraries::SaveDataDialog::s32 sceSaveDataDialogTerminate() {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().Terminate();
}

Libraries::SaveDataDialog::s32 sceSaveDataDialogOpen(
    const Libraries::SaveDataDialog::SceSaveDataDialogParam* param) {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().Open(param);
}

Libraries::SaveDataDialog::s32 sceSaveDataDialogClose() {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().Close();
}

Libraries::SaveDataDialog::u32 sceSaveDataDialogUpdateStatus() {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().UpdateStatus();
}

Libraries::SaveDataDialog::u32 sceSaveDataDialogGetStatus() {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().GetStatus();
}

Libraries::SaveDataDialog::s32 sceSaveDataDialogGetResult(
    Libraries::SaveDataDialog::SceSaveDataDialogResultData* result) {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().GetResult(result);
}

Libraries::SaveDataDialog::s32 sceSaveDataDialogProgressBarSetValue(
    Libraries::SaveDataDialog::u32 target, Libraries::SaveDataDialog::u32 value) {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().ProgressBarSetValue(
        target, value);
}

Libraries::SaveDataDialog::s32 sceSaveDataDialogProgressBarInc(
    Libraries::SaveDataDialog::u32 target, Libraries::SaveDataDialog::u32 delta) {
    return Libraries::SaveDataDialog::SaveDataDialogContext::Instance().ProgressBarInc(target,
                                                                                       delta);
}

}
```

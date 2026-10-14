#include "SceTypes.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C" {
int APS5_VABI sceContentDeleteInitialize(const ContentDeleteInitParam* init_param);
int APS5_VABI sceContentDeleteTerminate(void);
int APS5_VABI sceContentDeleteById(std::int64_t contentId);
int APS5_VABI sceContentDeleteByPath(const char* contentPath);
}

namespace {

void Require(bool value) { if (!value) std::abort(); }

template <typename TException, typename TAction>
bool Throws(TAction action) {
    try {
        action();
    } catch (const TException&) {
        return true;
    }
    return false;
}

}

int main() {
    Require(Throws<std::logic_error>([] { sceContentDeleteById(1); }));
    Require(Throws<std::logic_error>([] { sceContentDeleteByPath("/user/av_contents/photo/1.png"); }));
    Require(Throws<std::invalid_argument>([] { sceContentDeleteByPath(nullptr); }));

    ContentDeleteInitParam param{};
    param.heap_size = 0x10000;
    Require(sceContentDeleteInitialize(&param) == 0);
    Require(Throws<std::runtime_error>([] { sceContentDeleteById(1); }));
    Require(Throws<std::runtime_error>([] { sceContentDeleteById(-1); }));
    Require(Throws<std::runtime_error>([] { sceContentDeleteByPath("/user/av_contents/photo/1.png"); }));
    Require(Throws<std::invalid_argument>([] { sceContentDeleteByPath(nullptr); }));
    Require(sceContentDeleteTerminate() == 0);
    Require(Throws<std::logic_error>([] { sceContentDeleteById(1); }));
}

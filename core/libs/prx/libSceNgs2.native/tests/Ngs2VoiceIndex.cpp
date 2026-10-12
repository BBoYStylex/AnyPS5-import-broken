#include "Ngs2Test.hpp"

#include <array>
#include <limits>

int main() {
    const auto system = CreateSystem();
    for (const auto rackId : std::array{SCE_NGS2_RACK_ID_SAMPLER, SCE_NGS2_RACK_ID_SUBMIXER,
                                      SCE_NGS2_RACK_ID_REVERB, SCE_NGS2_RACK_ID_MASTERING}) {
        const auto rack = CreateRack(system, rackId);
        Ngs2RackInfo info{};
        Require(sceNgs2RackGetInfo(rack, &info, sizeof(info)) == SCE_NGS2_OK);
        Require(info.max_voices != 0);
        uintptr_t last = 0;
        Require(sceNgs2RackGetVoiceHandle(rack, info.max_voices - 1, &last) == SCE_NGS2_OK && last != 0);
        for (const auto index : std::array{info.max_voices, info.max_voices + 1,
                                          std::numeric_limits<std::uint32_t>::max()}) {
            uintptr_t output = last;
            Require(sceNgs2RackGetVoiceHandle(rack, index, &output) == static_cast<int>(0x804A8305));
            Require(output == last);
        }
        uintptr_t repeated = 0;
        Require(sceNgs2RackGetVoiceHandle(rack, info.max_voices - 1, &repeated) == SCE_NGS2_OK);
        Require(repeated == last);
        Require(sceNgs2RackDestroy(rack, nullptr) == SCE_NGS2_OK);
    }
    Require(sceNgs2SystemDestroy(system, nullptr) == SCE_NGS2_OK);
    std::puts("NGS2 voice index: PASS");
}

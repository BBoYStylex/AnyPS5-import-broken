#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace SaveDialog {

struct View {
    int Mode = 0;
    int DisplayType = 0;
    std::vector<std::string> Names;
    std::string Message;
};

extern "C" bool SaveDataDialogUiEnabled_nid_no_patch();
extern "C" void SaveDataDialogUiOpen_nid_no_patch(View view, std::function<void(bool, std::size_t)> complete);
extern "C" void SaveDataDialogUiClose_nid_no_patch();
extern "C" void SaveDataDialogUiProgress_nid_no_patch(std::uint32_t value);

}

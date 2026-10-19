#include <relinker/analysis/MonoAotRanges.hpp>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Domain::VirtualAddress;
using Ranges = std::vector<std::pair<VirtualAddress, VirtualAddress>>;

constexpr VirtualAddress Text = 0x1000;
constexpr VirtualAddress TextOffset = 0x100;
constexpr VirtualAddress Data = 0x4000;
constexpr VirtualAddress DataOffset = 0x400;
constexpr VirtualAddress SegmentSize = 0x200;

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void RequireThrows(const std::function<void()>& action, const std::string& message) {
    try {
        action();
    } catch (const Domain::RelinkerException&) {
        return;
    }
    throw std::runtime_error(message);
}

struct Fixture {
    std::vector<std::uint8_t> Bytes = std::vector<std::uint8_t>(DataOffset + SegmentSize, 0);
    std::vector<Domain::ProgramHeader> Headers{
        {1, 5, TextOffset, Text, Text, SegmentSize, SegmentSize, 0x10},
        {1, 6, DataOffset, Data, Data, SegmentSize, SegmentSize, 0x10},
    };
    std::map<VirtualAddress, VirtualAddress> Relative;
    VirtualAddress NextString = Text + 0x90;
    VirtualAddress NextEntry = Data;

    std::uint8_t& At(VirtualAddress address) {
        return Bytes[address >= Data ? DataOffset + (address - Data) : TextOffset + (address - Text)];
    }

    void Write(VirtualAddress address, std::initializer_list<std::uint8_t> values) {
        for (const auto value : values) At(address++) = value;
    }

    VirtualAddress String(const std::string& text) {
        const auto address = NextString;
        for (const auto character : text) At(NextString++) = static_cast<std::uint8_t>(character);
        At(NextString++) = 0;
        return address;
    }

    void Global(const std::string& name, VirtualAddress value) {
        Relative[NextEntry] = String(name);
        Relative[NextEntry + 8] = value;
        NextEntry += 16;
    }
};

Fixture MakeImage(bool withPltEnd = true) {
    Fixture image;
    image.Write(Text + 0x20, {0x0f, 0x05});
    image.Write(Text + 0x60, {0xff, 0x25, 0, 0, 0, 0, 0x0f, 0x07, 0, 0});
    image.Write(Text + 0x6a, {0xff, 0x25, 0, 0, 0, 0, 0x0f, 0x05, 0, 0});
    const auto name = image.NextString = Text + 0x80;
    image.String("Test");
    image.NextString = Text + 0x90;
    image.Global("methods", Text);
    image.Global("methods_end", Text + 0x20);
    image.Global("generic_trampoline_0", Text + 0x30);
    image.Global("generic_trampoline_0_p", Text + 0x38);
    image.Global("specific_trampolines", Text + 0x40);
    image.Global("class_info_offsets", Text + 0x50);
    image.Global("plt", Text + 0x60);
    if (withPltEnd) image.Global("plt_end", Text + 0x74);
    image.Global("mono_aot_file_info", Data + 0x100);
    image.Global("mono_aot_assembly_name", name);
    image.Relative[image.NextEntry + 8] = Data;
    return image;
}

void CheckRanges() {
    auto image = MakeImage();
    const auto stringsEnd = image.NextString;
    const auto ranges = Relinker::MonoAotDataRanges(image.Bytes, image.Headers, image.Relative);
    const Ranges expected{
        {Text + 0x20, Text + 0x30},
        {Text + 0x38, Text + 0x40},
        {Text + 0x50, Text + 0x60},
        {Text + 0x66, Text + 0x6a},
        {Text + 0x70, Text + 0x74},
        {Text + 0x74, stringsEnd},
    };
    Require(ranges == expected, "Unexpected Mono AOT data ranges");
}

void CheckNoImage() {
    Fixture image;
    image.Relative[Data] = image.String("not_mono");
    image.Relative[Data + 8] = Text;
    Require(Relinker::MonoAotDataRanges(image.Bytes, image.Headers, image.Relative).empty(), "A table without mono_aot_file_info was taken as Mono AOT");
}

void CheckRejections() {
    RequireThrows([] {
        auto image = MakeImage(false);
        Relinker::MonoAotDataRanges(image.Bytes, image.Headers, image.Relative);
    }, "A globals table without plt_end was accepted");
    RequireThrows([] {
        auto image = MakeImage();
        image.Write(Text + 0x6a, {0xe9});
        Relinker::MonoAotDataRanges(image.Bytes, image.Headers, image.Relative);
    }, "A PLT entry that is not an indirect jump was accepted");
    RequireThrows([] {
        auto image = MakeImage();
        image.Write(image.NextEntry, {1});
        Relinker::MonoAotDataRanges(image.Bytes, image.Headers, image.Relative);
    }, "An unterminated globals table was accepted");
    RequireThrows([] {
        auto image = MakeImage();
        image.Relative.erase(Data + 8);
        Relinker::MonoAotDataRanges(image.Bytes, image.Headers, image.Relative);
    }, "A global without a relocated value was accepted");
}

void CheckFill() {
    std::vector<std::uint8_t> code(0x10, 0xcc);
    Relinker::FillDataRanges(code, Text, {{Text - 4, Text + 2}, {Text + 8, Text + 9}, {Text + 0x0e, Text + 0x20}});
    const std::vector<std::uint8_t> expected{0x90, 0x90, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x90, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x90, 0x90};
    Require(code == expected, "Data ranges were not clipped to the code span");
}

}

int main() {
    try {
        CheckRanges();
        CheckNoImage();
        CheckRejections();
        CheckFill();
        std::cout << "Mono AOT range tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "mono_aot_ranges_tests: " << error.what() << '\n';
        return 1;
    }
}

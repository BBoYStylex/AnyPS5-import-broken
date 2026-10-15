#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "tiling.h"

static std::vector<unsigned char> Read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open capture");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    if (argc != 4) return 2;
    auto actual = Read(argv[1]);
    auto reference = Read(argv[2]);
    if (actual.size() != tiled_pixels(1920,1080)*4 || reference.size() != 320*200+256*4) return 3;
    std::uint32_t palette[256];
    std::memcpy(palette, reference.data()+320*200, sizeof(palette));
    std::ofstream image(argv[3], std::ios::binary);
    image << "P6\n1920 1080\n255\n";
    std::uint64_t different = 0;
    for (unsigned y=0;y<1080;++y) {
        for (unsigned x=0;x<1920;++x) {
            auto offset = tiled_index(x,y,1920)*4;
            std::uint32_t packed;
            std::memcpy(&packed,actual.data()+offset,4);
            unsigned char rgb[3]={static_cast<unsigned char>(packed),static_cast<unsigned char>(packed>>8),static_cast<unsigned char>(packed>>16)};
            image.write(reinterpret_cast<const char*>(rgb),3);
            std::uint32_t expected=0;
            if(x>=240 && x<1680) expected=palette[reference[(y*200/1080)*320+(x-240)*320/1440]];
            if (rgb[0] != ((expected>>16)&255) || rgb[1] != ((expected>>8)&255) || rgb[2] != (expected&255)) ++different;
        }
    }
    std::printf("{\"width\":1920,\"height\":1080,\"different_rgb_pixels\":%llu,\"alpha_compared\":false}\n",static_cast<unsigned long long>(different));
    return different?1:0;
}

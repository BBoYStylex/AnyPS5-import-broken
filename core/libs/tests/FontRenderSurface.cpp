#include "prx/libSceFont/include/FontTypes.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

extern "C" {
int APS5_VABI sceFontMemoryInit(FontMemory*, void*, std::uint32_t, const FontMemoryInterface*, void*, FontMemoryDestroyFunction, void*);
int APS5_VABI sceFontMemoryTerm(FontMemory*);
int APS5_VABI sceFontCreateLibrary(const FontMemory*, const void*, FontLibrary*);
int APS5_VABI sceFontDestroyLibrary(FontLibrary*);
int APS5_VABI sceFontSupportExternalFonts(FontLibrary, std::uint32_t, std::uint32_t);
int APS5_VABI sceFontOpenFontMemory(FontLibrary, const void*, std::uint32_t, const FontOpenDetail*, FontHandle*);
int APS5_VABI sceFontCloseFont(FontHandle);
int APS5_VABI sceFontCreateRenderer(const FontMemory*, const void*, FontRenderer*);
int APS5_VABI sceFontDestroyRenderer(FontRenderer*);
int APS5_VABI sceFontBindRenderer(FontHandle, FontRenderer);
int APS5_VABI sceFontUnbindRenderer(FontHandle);
int APS5_VABI sceFontSetupRenderScalePixel(FontHandle, float, float);
void APS5_VABI sceFontRenderSurfaceInit(FontRenderSurface*, void*, int, int, int, int);
int APS5_VABI sceFontRenderCharGlyphImage(FontHandle, std::uint32_t, FontRenderSurface*, float, float, FontGlyphMetrics*, FontRenderOutput*);
int APS5_VABI sceFontRenderCharGlyphImageHorizontal(FontHandle, std::uint32_t, FontRenderSurface*, float, float, FontGlyphMetrics*, FontRenderOutput*);
int APS5_VABI sceFontRenderCharGlyphImageVertical(FontHandle, std::uint32_t, FontRenderSurface*, float, float, FontGlyphMetrics*, FontRenderOutput*);
const void* APS5_VABI sceFontSelectLibraryFt(int);
const void* APS5_VABI sceFontSelectRendererFt(int);
}

static void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "Font surface check failed at line %d\n", line);
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

static int allocations = 0;
static void* APS5_VABI Allocate(void*, std::uint32_t size) {
    ++allocations;
    return std::malloc(size);
}
static void APS5_VABI Release(void*, void* pointer) {
    if (pointer) --allocations;
    std::free(pointer);
}

static void Put16(std::vector<unsigned char>& out, std::uint32_t value) {
    out.push_back(static_cast<unsigned char>(value >> 8));
    out.push_back(static_cast<unsigned char>(value));
}

static void Put32(std::vector<unsigned char>& out, std::uint32_t value) {
    Put16(out, value >> 16);
    Put16(out, value & 0xFFFFu);
}

static void PutAll16(std::vector<unsigned char>& out, std::initializer_list<int> values) {
    for (const int value : values) Put16(out, static_cast<std::uint32_t>(static_cast<std::uint16_t>(value)));
}

static std::vector<unsigned char> SquareGlyphFont(std::uint32_t codepoint, int width) {
    std::vector<unsigned char> head;
    Put32(head, 0x00010000u);
    Put32(head, 0x00010000u);
    Put32(head, 0);
    Put32(head, 0x5F0F3CF5u);
    PutAll16(head, {0x000B, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, width, 800, 0, 8, 2, 0, 0});
    std::vector<unsigned char> hhea;
    Put32(hhea, 0x00010000u);
    PutAll16(hhea, {800, -200, 0, width, 0, 0, width, 1, 0, 0, 0, 0, 0, 0, 0, 2});
    std::vector<unsigned char> maxp;
    Put32(maxp, 0x00010000u);
    PutAll16(maxp, {2, 4, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0});
    std::vector<unsigned char> hmtx;
    PutAll16(hmtx, {width, 0, width, 100});
    std::vector<unsigned char> glyf;
    PutAll16(glyf, {1, 100, 0, width - 100, 700, 3, 0});
    glyf.insert(glyf.end(), {1, 1, 1, 1});
    PutAll16(glyf, {100, 0, width - 200, 0, 0, 700, 0, -700});
    std::vector<unsigned char> loca;
    PutAll16(loca, {0, 0, static_cast<int>(glyf.size() / 2)});
    std::vector<unsigned char> cmap;
    PutAll16(cmap, {0, 1, 3, 1});
    Put32(cmap, 12);
    PutAll16(cmap, {4, 32, 0, 4, 4, 1, 0, static_cast<int>(codepoint), 0xFFFF, 0, static_cast<int>(codepoint), 0xFFFF, 1 - static_cast<int>(codepoint), 1, 0, 0});
    const std::vector<std::pair<const char*, const std::vector<unsigned char>*>> tables = {
        {"cmap", &cmap}, {"glyf", &glyf}, {"head", &head}, {"hhea", &hhea}, {"hmtx", &hmtx}, {"loca", &loca}, {"maxp", &maxp}};
    std::vector<unsigned char> font;
    Put32(font, 0x00010000u);
    PutAll16(font, {static_cast<int>(tables.size()), 64, 2, static_cast<int>(tables.size()) * 16 - 64});
    std::uint32_t offset = 12 + static_cast<std::uint32_t>(tables.size()) * 16;
    for (const auto& [tag, data] : tables) {
        font.insert(font.end(), tag, tag + 4);
        Put32(font, 0);
        Put32(font, offset);
        Put32(font, static_cast<std::uint32_t>(data->size()));
        offset += (static_cast<std::uint32_t>(data->size()) + 3u) & ~3u;
    }
    for (const auto& [tag, data] : tables) {
        font.insert(font.end(), data->begin(), data->end());
        font.resize((font.size() + 3u) & ~std::size_t{3});
    }
    return font;
}

static void CheckRenderStrides(const FontMemory* memory, FontHandle font) {
    FontRenderer renderer = nullptr;
    Require(sceFontCreateRenderer(memory, sceFontSelectRendererFt(0), &renderer) == SCE_FONT_OK);
    Require(sceFontBindRenderer(font, renderer) == SCE_FONT_OK);
    Require(sceFontSetupRenderScalePixel(font, 100.0f, 100.0f) == SCE_FONT_OK);
    for (const auto render : {sceFontRenderCharGlyphImage, sceFontRenderCharGlyphImageHorizontal, sceFontRenderCharGlyphImageVertical}) {
        for (const int bpp : {1, 4}) {
            for (const int invalid : {0, 1, 2}) {
                std::vector<unsigned char> pixels(1024, 0xA5);
                FontRenderSurface surface{};
                sceFontRenderSurfaceInit(&surface, pixels.data(), 16 * bpp, bpp, 80, 1);
                if (invalid == 0) surface.buffer = nullptr;
                else surface.height = invalid == 1 ? 0 : -1;
                FontGlyphMetrics metrics{};
                FontRenderOutput output{};
                Require(render(font, 'A', &surface, 0.0f, 70.0f, &metrics, &output) == SCE_FONT_ERROR_NO_SUPPORT_SURFACE);
                Require(std::all_of(pixels.begin(), pixels.end(), [](unsigned char value) { return value == 0xA5; }));
            }
            for (const auto [width, stride, clipped] : {std::array<int, 3>{80, 16 * bpp, 0},
                    {80, 80 * bpp - 1, 0}, {80, 16 * bpp, 1}, {INT_MAX, 200, 0}}) {
                std::vector<unsigned char> pixels(1024, 0xA5);
                FontRenderSurface surface{};
                sceFontRenderSurfaceInit(&surface, pixels.data(), stride, bpp, width, 1);
                if (clipped) surface.sc_x1 = 8;
                FontGlyphMetrics metrics;
                FontRenderOutput output;
                std::memset(&metrics, 0x5A, sizeof(metrics));
                std::memset(&output, 0x5A, sizeof(output));
                bool threw = false;
                try {
                    const float y = render == sceFontRenderCharGlyphImage ? -10.0f : 70.0f;
                    render(font, 'A', &surface, 0.0f, y, &metrics, &output);
                } catch (const std::runtime_error& error) {
                    threw = std::string(error.what()).find("stride") != std::string::npos;
                }
                if (!threw) {
                    const bool escaped = std::any_of(pixels.begin() + stride, pixels.end(), [](unsigned char value) { return value != 0xA5; });
                    std::fprintf(stderr, "Undersized Font stride: bpp=%d width=%d stride=%d wrote_beyond_buffer=%d\n", bpp, width, stride, escaped);
                }
                Require(threw);
                Require(std::all_of(pixels.begin(), pixels.end(), [](unsigned char value) { return value == 0xA5; }));
                const auto* metricBytes = reinterpret_cast<const unsigned char*>(&metrics);
                const auto* outputBytes = reinterpret_cast<const unsigned char*>(&output);
                Require(std::all_of(metricBytes, metricBytes + sizeof(metrics), [](unsigned char value) { return value == 0; }));
                Require(std::all_of(outputBytes, outputBytes + sizeof(output), [](unsigned char value) { return value == 0; }));
                for (const int padding : {0, 3}) {
                    const int validStride = 80 * bpp + padding;
                    std::vector<unsigned char> validPixels(100 * validStride + 128, 0);
                    sceFontRenderSurfaceInit(&surface, validPixels.data(), validStride, bpp, 80, 100);
                    const float validY = render == sceFontRenderCharGlyphImage ? 0.0f : 70.0f;
                    Require(render(font, 'A', &surface, 0.0f, validY, &metrics, &output) == SCE_FONT_OK);
                    Require(std::any_of(validPixels.begin(), validPixels.begin() + 100 * validStride, [](unsigned char value) { return value != 0; }));
                    Require(std::all_of(validPixels.begin() + 100 * validStride, validPixels.end(), [](unsigned char value) { return value == 0; }));
                    for (int row = 0; row < 100; ++row) {
                        Require(std::all_of(validPixels.begin() + row * validStride + 80 * bpp,
                            validPixels.begin() + (row + 1) * validStride, [](unsigned char value) { return value == 0; }));
                    }
                }
            }
        }
    }
    Require(sceFontUnbindRenderer(font) == SCE_FONT_OK);
    Require(sceFontDestroyRenderer(&renderer) == SCE_FONT_OK);
}

int main() {
    const FontMemoryInterface iface{Allocate, Release, nullptr, nullptr, nullptr, nullptr};
    FontMemory memory{};
    Require(sceFontMemoryInit(&memory, nullptr, 0, &iface, nullptr, nullptr, nullptr) == SCE_FONT_OK);
    FontLibrary library = nullptr;
    Require(sceFontCreateLibrary(&memory, sceFontSelectLibraryFt(0), &library) == SCE_FONT_OK);
    Require(sceFontSupportExternalFonts(library, 1, 0x52) == SCE_FONT_OK);
    const auto bytes = SquareGlyphFont('A', 900);
    FontHandle font = nullptr;
    Require(sceFontOpenFontMemory(library, bytes.data(), static_cast<std::uint32_t>(bytes.size()), nullptr, &font) == SCE_FONT_OK);
    CheckRenderStrides(&memory, font);
    Require(sceFontCloseFont(font) == SCE_FONT_OK);
    Require(sceFontDestroyLibrary(&library) == SCE_FONT_OK);
    Require(sceFontMemoryTerm(&memory) == SCE_FONT_OK);
    Require(allocations == 0);
}

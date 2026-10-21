#include "FreeTypeFontFixture.hpp"
#include "prx/libSceFreeType/include/GuestFreeType.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

using namespace FreeType;

extern "C" {
int APS5_VABI FT_Outline_Copy_nid_postfix(const GuestOutline*, GuestOutline*);
int APS5_VABI FT_Init_FreeType_nid_postfix(void**);
int APS5_VABI FT_Done_FreeType_nid_postfix(void*);
int APS5_VABI FT_New_Memory_Face_nid_postfix(void*, const unsigned char*, std::int64_t, std::int64_t, GuestFaceRec**);
int APS5_VABI FT_Done_Face_nid_postfix(GuestFaceRec*);
int APS5_VABI FT_Set_Pixel_Sizes_nid_postfix(GuestFaceRec*, std::uint32_t, std::uint32_t);
int APS5_VABI FT_Load_Char_nid_postfix(GuestFaceRec*, std::uint64_t, std::int32_t);
int APS5_VABI FT_Render_Glyph_nid_postfix(GuestGlyphSlotRec*, std::int32_t);
}

static void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "FreeType outline copy check failed at line %d\n", line);
        std::abort();
    }
}

#define Require(value) Check((value), __LINE__)

static bool Equal(GuestVector a, GuestVector b) {
    return a.x == b.x && a.y == b.y;
}

static bool Equal(const GuestOutline& a, const GuestOutline& b) {
    return a.n_points == b.n_points && a.n_contours == b.n_contours && a.points == b.points &&
           a.tags == b.tags && a.contours == b.contours && a.flags == b.flags;
}

static void CopyCase(std::uint16_t pointCount, std::uint16_t contourCount, int sourceFlags, int targetFlags, bool wide) {
    const GuestVector guard{0x123456789ABCDEF, -0x123456789ABCDEF};
    std::vector<GuestVector> points(pointCount + 2, guard), copied(pointCount + 2, guard);
    std::vector<unsigned char> tags(pointCount + 2, 0xA5), copiedTags(pointCount + 2, 0x5A);
    std::vector<std::uint16_t> contours(contourCount + 2, 0xABCD), copiedContours(contourCount + 2, 0xDCBA);
    std::vector<FT_Vector> hostPoints(pointCount), hostCopied(pointCount);
    std::vector<unsigned char> hostTags(pointCount), hostCopiedTags(pointCount);
    std::vector<unsigned short> hostContours(contourCount), hostCopiedContours(contourCount);
    for (unsigned index = 0; index < pointCount; ++index) {
        const auto value = static_cast<std::int64_t>(index) * 65537 - 100000;
        points[index + 1] = wide ? GuestVector{std::numeric_limits<std::int64_t>::min() + index,
                                             std::numeric_limits<std::int64_t>::max() - index}
                                : GuestVector{value / 4, -value / 4};
        tags[index + 1] = static_cast<unsigned char>(index);
        hostPoints[index] = {static_cast<FT_Pos>(points[index + 1].x), static_cast<FT_Pos>(points[index + 1].y)};
        hostTags[index] = tags[index + 1];
    }
    for (unsigned index = 0; index < contourCount; ++index) {
        contours[index + 1] = static_cast<std::uint16_t>(index);
        hostContours[index] = contours[index + 1];
    }
    GuestOutline source{contourCount, pointCount, pointCount ? points.data() + 1 : nullptr,
                        pointCount ? tags.data() + 1 : nullptr, contourCount ? contours.data() + 1 : nullptr, sourceFlags};
    GuestOutline target{contourCount, pointCount, pointCount ? copied.data() + 1 : nullptr,
                        pointCount ? copiedTags.data() + 1 : nullptr, contourCount ? copiedContours.data() + 1 : nullptr, targetFlags};
    const auto originalSource = source;
    auto expectedTarget = target;
    const auto originalPoints = points;
    const auto originalTags = tags;
    const auto originalContours = contours;
    FT_Outline hostSource{contourCount, pointCount, hostPoints.data(), hostTags.data(), hostContours.data(), sourceFlags};
    FT_Outline hostTarget{contourCount, pointCount, hostCopied.data(), hostCopiedTags.data(), hostCopiedContours.data(), targetFlags};
    Require(FT_Outline_Copy_nid_postfix(&source, &target) == FT_Outline_Copy(&hostSource, &hostTarget));
    expectedTarget.flags = hostTarget.flags;
    Require(Equal(source, originalSource) && Equal(target, expectedTarget));
    Require(Equal(copied.front(), guard) && Equal(copied.back(), guard));
    Require(copiedTags.front() == 0x5A && copiedTags.back() == 0x5A);
    Require(copiedContours.front() == 0xDCBA && copiedContours.back() == 0xDCBA);
    for (unsigned index = 0; index < pointCount; ++index) {
        Require(Equal(copied[index + 1], originalPoints[index + 1]));
        if (!wide) Require(copied[index + 1].x == hostCopied[index].x && copied[index + 1].y == hostCopied[index].y);
        Require(copiedTags[index + 1] == hostCopiedTags[index]);
    }
    for (unsigned index = 0; index < contourCount; ++index) Require(copiedContours[index + 1] == hostCopiedContours[index]);
    if (pointCount) {
        target.points[0] = {321, -654};
        target.tags[0] ^= 0xFF;
    }
    if (contourCount) target.contours[0] ^= 0xFFFF;
    for (std::size_t index = 0; index < points.size(); ++index) Require(Equal(points[index], originalPoints[index]));
    Require(tags == originalTags && contours == originalContours);
}

static void InvalidAndSelfCopies() {
    GuestOutline source{1, 4, nullptr, nullptr, nullptr, 0x1234};
    auto target = source;
    Require(FT_Outline_Copy_nid_postfix(nullptr, nullptr) == FT_Outline_Copy(nullptr, nullptr));
    Require(FT_Outline_Copy_nid_postfix(nullptr, &target) == FT_Err_Invalid_Outline);
    Require(Equal(target, source));
    Require(FT_Outline_Copy_nid_postfix(&source, nullptr) == FT_Err_Invalid_Outline);
    Require(FT_Outline_Copy_nid_postfix(&source, &source) == 0);
    for (const auto counts : std::array<std::array<std::uint16_t, 2>, 3>{{{1, 3}, {0, 4}, {0, 0}}}) {
        target.n_contours = counts[0];
        target.n_points = counts[1];
        const auto before = target;
        FT_Outline nativeSource{1, 4, nullptr, nullptr, nullptr, source.flags};
        FT_Outline nativeTarget{target.n_contours, target.n_points, nullptr, nullptr, nullptr, target.flags};
        Require(FT_Outline_Copy_nid_postfix(&source, &target) == FT_Outline_Copy(&nativeSource, &nativeTarget));
        Require(Equal(target, before));
    }
}

static void RenderCopy() {
    const auto font = SquareGlyphFont('A', 700);
    void* library = nullptr;
    GuestFaceRec* face = nullptr;
    FT_Library nativeLibrary = nullptr;
    FT_Face nativeFace = nullptr;
    Require(FT_Init_FreeType_nid_postfix(&library) == 0 && FT_Init_FreeType(&nativeLibrary) == 0);
    Require(FT_New_Memory_Face_nid_postfix(library, font.data(), font.size(), 0, &face) == 0);
    Require(FT_New_Memory_Face(nativeLibrary, font.data(), static_cast<FT_Long>(font.size()), 0, &nativeFace) == 0);
    Require(FT_Set_Pixel_Sizes_nid_postfix(face, 0, 32) == 0 && FT_Set_Pixel_Sizes(nativeFace, 0, 32) == 0);
    for (const int offset : {0, 37, -129}) {
        Require(FT_Load_Char_nid_postfix(face, 'A', FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) == 0);
        Require(FT_Load_Char(nativeFace, 'A', FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) == 0);
        auto& target = face->glyph->outline;
        std::vector<GuestVector> points(target.points, target.points + target.n_points);
        std::vector<FT_Vector> nativePoints(nativeFace->glyph->outline.points,
                                            nativeFace->glyph->outline.points + nativeFace->glyph->outline.n_points);
        for (std::size_t index = 0; index < points.size(); ++index) {
            points[index].x += offset + points[index].y / 4;
            nativePoints[index].x += offset + nativePoints[index].y / 4;
        }
        auto source = target;
        source.points = points.data();
        source.flags ^= FT_OUTLINE_OWNER;
        auto nativeSource = nativeFace->glyph->outline;
        nativeSource.points = nativePoints.data();
        nativeSource.flags ^= FT_OUTLINE_OWNER;
        Require(FT_Outline_Copy_nid_postfix(&source, &target) == 0);
        Require(FT_Outline_Copy(&nativeSource, &nativeFace->glyph->outline) == 0);
        points[0] = {};
        nativePoints[0] = {};
        Require(FT_Render_Glyph_nid_postfix(face->glyph, FT_RENDER_MODE_NORMAL) == 0);
        Require(FT_Render_Glyph(nativeFace->glyph, FT_RENDER_MODE_NORMAL) == 0);
        const auto& actual = face->glyph->bitmap;
        const auto& expected = nativeFace->glyph->bitmap;
        Require(actual.rows > 0 && actual.width > 0);
        Require(actual.rows == expected.rows && actual.width == expected.width && actual.pitch == expected.pitch);
        Require(actual.pixel_mode == expected.pixel_mode && actual.num_grays == expected.num_grays);
        Require(face->glyph->bitmap_left == nativeFace->glyph->bitmap_left && face->glyph->bitmap_top == nativeFace->glyph->bitmap_top);
        const auto size = static_cast<std::size_t>(actual.rows) * std::abs(actual.pitch);
        Require(std::memcmp(actual.buffer, expected.buffer, size) == 0);
    }
    Require(FT_Done_Face_nid_postfix(face) == 0 && FT_Done_Face(nativeFace) == 0);
    Require(FT_Done_FreeType_nid_postfix(library) == 0 && FT_Done_FreeType(nativeLibrary) == 0);
}

int main() {
    InvalidAndSelfCopies();
    for (const int sourceFlags : {0, FT_OUTLINE_OWNER, FT_OUTLINE_REVERSE_FILL, -1}) {
        for (const int targetFlags : {0, FT_OUTLINE_OWNER, FT_OUTLINE_EVEN_ODD_FILL, -1}) {
            CopyCase(0, 0, sourceFlags, targetFlags, false);
            CopyCase(4, 2, sourceFlags, targetFlags, false);
            CopyCase(4, 2, sourceFlags, targetFlags, true);
        }
    }
    CopyCase(65535, 65535, FT_OUTLINE_REVERSE_FILL, FT_OUTLINE_OWNER, false);
    CopyCase(65535, 65535, FT_OUTLINE_OWNER, 0, true);
    std::mt19937 random(0xF7C0);
    for (unsigned index = 0; index < 128; ++index) {
        const auto count = static_cast<std::uint16_t>(random() % 256 + 1);
        const auto contours = static_cast<std::uint16_t>(random() % (count + 1));
        const auto sourceFlags = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(random()));
        const auto targetFlags = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(random()));
        CopyCase(count, contours, sourceFlags, targetFlags, false);
    }
    RenderCopy();
    std::puts("FreeType outline copy: 178 copy cases, validation, self-copy and 3 rendered copies passed");
}

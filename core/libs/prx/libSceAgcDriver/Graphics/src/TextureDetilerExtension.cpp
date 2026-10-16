#include <cstdint>
#include <algorithm>

namespace Sce::AgcDriver::Graphics {

enum class ExtendedBlockFormat : uint32_t {
    BC1_UNORM = 0x47,
    BC2_UNORM = 0x49,
    BC3_UNORM = 0x4B,
    BC4_UNORM = 0x4D,
    BC5_UNORM = 0x4F,
    BC6H_UFLOAT = 0x51,
    BC6H_SFLOAT = 0x52,
    BC7_UNORM = 0x53,
    BC7_SRGB = 0x54,
    YUV420_NV12 = 0x60,
    YUV420_P010 = 0x61
};

struct DetileParameters {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t pitch;
    uint32_t blockWidth;
    uint32_t blockHeight;
    uint32_t bytesPerBlock;
};

inline DetileParameters GetFormatDetileParams(ExtendedBlockFormat format, uint32_t width, uint32_t height, uint32_t depth) {
    DetileParameters params{};
    params.width = width;
    params.height = height;
    params.depth = std::max(1u, depth);

    switch (format) {
    case ExtendedBlockFormat::BC6H_UFLOAT:
    case ExtendedBlockFormat::BC6H_SFLOAT:
    case ExtendedBlockFormat::BC7_UNORM:
    case ExtendedBlockFormat::BC7_SRGB:
        params.blockWidth = 4;
        params.blockHeight = 4;
        params.bytesPerBlock = 16;
        break;

    case ExtendedBlockFormat::BC1_UNORM:
    case ExtendedBlockFormat::BC4_UNORM:
        params.blockWidth = 4;
        params.blockHeight = 4;
        params.bytesPerBlock = 8;
        break;

    case ExtendedBlockFormat::BC2_UNORM:
    case ExtendedBlockFormat::BC3_UNORM:
    case ExtendedBlockFormat::BC5_UNORM:
        params.blockWidth = 4;
        params.blockHeight = 4;
        params.bytesPerBlock = 16;
        break;

    case ExtendedBlockFormat::YUV420_NV12:
        params.blockWidth = 2;
        params.blockHeight = 2;
        params.bytesPerBlock = 3;
        break;

    case ExtendedBlockFormat::YUV420_P010:
        params.blockWidth = 2;
        params.blockHeight = 2;
        params.bytesPerBlock = 6;
        break;

    default:
        params.blockWidth = 1;
        params.blockHeight = 1;
        params.bytesPerBlock = 4;
        break;
    }

    uint32_t blocksX = (params.width + params.blockWidth - 1) / params.blockWidth;
    params.pitch = blocksX * params.bytesPerBlock;
    return params;
}

bool ComputeDetiledOffset(ExtendedBlockFormat format, uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint64_t& outOffset) {
    const DetileParameters p = GetFormatDetileParams(format, width, height, depth);
    if (x >= width || y >= height || z >= depth) {
        return false;
    }

    uint32_t bx = x / p.blockWidth;
    uint32_t by = y / p.blockHeight;
    uint32_t blocksX = (width + p.blockWidth - 1) / p.blockWidth;
    uint32_t blocksY = (height + p.blockHeight - 1) / p.blockHeight;

    uint64_t sliceSize = static_cast<uint64_t>(blocksX) * blocksY * p.bytesPerBlock;
    outOffset = (static_cast<uint64_t>(z) * sliceSize) + (static_cast<uint64_t>(by) * p.pitch) + (static_cast<uint64_t>(bx) * p.bytesPerBlock);
    return true;
}

} // namespace Sce::AgcDriver::Graphics

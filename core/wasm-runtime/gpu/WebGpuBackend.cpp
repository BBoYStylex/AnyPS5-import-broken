#include "Backend.hpp"
#include "DrawStages.hpp"
#include "ShaderCapture.hpp"
#include "WgslBackend/WgslEmitter.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DepthSurface.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestSamplerResource.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestTextureResource.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ShaderInputState.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureFormat.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureTiling.hpp"
#include "prx/libSceAgcDriver/Graphics/include/VertexInput.hpp"
#include "wasm-runtime/Guest.hpp"
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <webgpu/webgpu_cpp.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <future>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <pthread.h>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace WasmGpu {
namespace {

using ShaderRecompiler::DescriptorRole;

constexpr std::uint32_t CanvasWidth = 1920;
constexpr std::uint32_t CanvasHeight = 1080;
constexpr std::uint64_t BufferLimit = 64ull << 20u;

void Require(bool condition, const std::string& reason) {
    if (!condition) throw std::runtime_error("WebGPU: " + reason);
}

std::vector<std::byte> ReadGuest(std::uint64_t address, std::uint64_t bytes) {
    Require(address != 0 || bytes == 0, "read of a null guest range");
    std::vector<std::byte> result(static_cast<std::size_t>((bytes + 3u) & ~std::uint64_t{3}));
    std::memcpy(result.data(), reinterpret_cast<const void*>(static_cast<std::uintptr_t>(address)), static_cast<std::size_t>(bytes));
    return result;
}

std::uint64_t Hash(std::uint64_t address, std::uint64_t bytes) {
    const auto* data = reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(address));
    std::uint64_t hash = 0xcbf29ce484222325ull ^ bytes;
    std::uint64_t offset = 0;
    for (; offset + 8 <= bytes; offset += 8) {
        std::uint64_t word;
        std::memcpy(&word, data + offset, sizeof(word));
        hash = (hash ^ word) * 0x100000001b3ull;
        hash ^= hash >> 29u;
    }
    for (; offset < bytes; ++offset) hash = (hash ^ static_cast<std::uint64_t>(data[offset])) * 0x100000001b3ull;
    return hash;
}

wgpu::TextureFormat TextureFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8G8B8A8_UNORM: return wgpu::TextureFormat::RGBA8Unorm;
    case VK_FORMAT_R8G8B8A8_SRGB: return wgpu::TextureFormat::RGBA8UnormSrgb;
    case VK_FORMAT_B8G8R8A8_UNORM: return wgpu::TextureFormat::BGRA8Unorm;
    case VK_FORMAT_B8G8R8A8_SRGB: return wgpu::TextureFormat::BGRA8UnormSrgb;
    case VK_FORMAT_R8_UNORM: return wgpu::TextureFormat::R8Unorm;
    case VK_FORMAT_R8G8_UNORM: return wgpu::TextureFormat::RG8Unorm;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return wgpu::TextureFormat::RGBA16Float;
    case VK_FORMAT_R16_SFLOAT: return wgpu::TextureFormat::R16Float;
    case VK_FORMAT_R16G16_SFLOAT: return wgpu::TextureFormat::RG16Float;
    case VK_FORMAT_R32_SFLOAT: return wgpu::TextureFormat::R32Float;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return wgpu::TextureFormat::RGBA32Float;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return wgpu::TextureFormat::RGB10A2Unorm;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32: return wgpu::TextureFormat::RGB10A2Uint;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return wgpu::TextureFormat::RG11B10Ufloat;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return wgpu::TextureFormat::RGB9E5Ufloat;
    case VK_FORMAT_R8_SNORM: return wgpu::TextureFormat::R8Snorm;
    case VK_FORMAT_R8_UINT: return wgpu::TextureFormat::R8Uint;
    case VK_FORMAT_R8_SINT: return wgpu::TextureFormat::R8Sint;
    case VK_FORMAT_R8G8_SNORM: return wgpu::TextureFormat::RG8Snorm;
    case VK_FORMAT_R8G8_UINT: return wgpu::TextureFormat::RG8Uint;
    case VK_FORMAT_R8G8_SINT: return wgpu::TextureFormat::RG8Sint;
    case VK_FORMAT_R8G8B8A8_SNORM: return wgpu::TextureFormat::RGBA8Snorm;
    case VK_FORMAT_R8G8B8A8_UINT: return wgpu::TextureFormat::RGBA8Uint;
    case VK_FORMAT_R8G8B8A8_SINT: return wgpu::TextureFormat::RGBA8Sint;
    case VK_FORMAT_R16_UNORM: return wgpu::TextureFormat::R16Unorm;
    case VK_FORMAT_R16_SNORM: return wgpu::TextureFormat::R16Snorm;
    case VK_FORMAT_R16_UINT: return wgpu::TextureFormat::R16Uint;
    case VK_FORMAT_R16_SINT: return wgpu::TextureFormat::R16Sint;
    case VK_FORMAT_R16G16_UNORM: return wgpu::TextureFormat::RG16Unorm;
    case VK_FORMAT_R16G16_SNORM: return wgpu::TextureFormat::RG16Snorm;
    case VK_FORMAT_R16G16_UINT: return wgpu::TextureFormat::RG16Uint;
    case VK_FORMAT_R16G16_SINT: return wgpu::TextureFormat::RG16Sint;
    case VK_FORMAT_R16G16B16A16_UNORM: return wgpu::TextureFormat::RGBA16Unorm;
    case VK_FORMAT_R16G16B16A16_SNORM: return wgpu::TextureFormat::RGBA16Snorm;
    case VK_FORMAT_R16G16B16A16_UINT: return wgpu::TextureFormat::RGBA16Uint;
    case VK_FORMAT_R16G16B16A16_SINT: return wgpu::TextureFormat::RGBA16Sint;
    case VK_FORMAT_R32_UINT: return wgpu::TextureFormat::R32Uint;
    case VK_FORMAT_R32_SINT: return wgpu::TextureFormat::R32Sint;
    case VK_FORMAT_R32G32_SFLOAT: return wgpu::TextureFormat::RG32Float;
    case VK_FORMAT_R32G32_UINT: return wgpu::TextureFormat::RG32Uint;
    case VK_FORMAT_R32G32_SINT: return wgpu::TextureFormat::RG32Sint;
    case VK_FORMAT_R32G32B32A32_UINT: return wgpu::TextureFormat::RGBA32Uint;
    case VK_FORMAT_R32G32B32A32_SINT: return wgpu::TextureFormat::RGBA32Sint;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return wgpu::TextureFormat::BC1RGBAUnorm;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return wgpu::TextureFormat::BC1RGBAUnormSrgb;
    case VK_FORMAT_BC2_UNORM_BLOCK: return wgpu::TextureFormat::BC2RGBAUnorm;
    case VK_FORMAT_BC2_SRGB_BLOCK: return wgpu::TextureFormat::BC2RGBAUnormSrgb;
    case VK_FORMAT_BC3_UNORM_BLOCK: return wgpu::TextureFormat::BC3RGBAUnorm;
    case VK_FORMAT_BC3_SRGB_BLOCK: return wgpu::TextureFormat::BC3RGBAUnormSrgb;
    case VK_FORMAT_BC4_UNORM_BLOCK: return wgpu::TextureFormat::BC4RUnorm;
    case VK_FORMAT_BC4_SNORM_BLOCK: return wgpu::TextureFormat::BC4RSnorm;
    case VK_FORMAT_BC5_UNORM_BLOCK: return wgpu::TextureFormat::BC5RGUnorm;
    case VK_FORMAT_BC5_SNORM_BLOCK: return wgpu::TextureFormat::BC5RGSnorm;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK: return wgpu::TextureFormat::BC6HRGBUfloat;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK: return wgpu::TextureFormat::BC6HRGBFloat;
    case VK_FORMAT_BC7_UNORM_BLOCK: return wgpu::TextureFormat::BC7RGBAUnorm;
    case VK_FORMAT_BC7_SRGB_BLOCK: return wgpu::TextureFormat::BC7RGBAUnormSrgb;
    default: break;
    }
    throw std::runtime_error("WebGPU: VkFormat " + std::to_string(static_cast<int>(format)) + " has no texture format");
}

wgpu::TextureFormat DepthFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_D16_UNORM: return wgpu::TextureFormat::Depth16Unorm;
    case VK_FORMAT_D32_SFLOAT: return wgpu::TextureFormat::Depth32Float;
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return wgpu::TextureFormat::Depth32FloatStencil8;
    case VK_FORMAT_D16_UNORM_S8_UINT: return wgpu::TextureFormat::Depth24PlusStencil8;
    default: break;
    }
    throw std::runtime_error("WebGPU: VkFormat " + std::to_string(static_cast<int>(format)) + " has no depth format");
}

bool HasStencil(wgpu::TextureFormat format) {
    return format == wgpu::TextureFormat::Depth32FloatStencil8 || format == wgpu::TextureFormat::Depth24PlusStencil8;
}

wgpu::CompareFunction CompareFunction(VkCompareOp operation) {
    switch (operation) {
    case VK_COMPARE_OP_NEVER: return wgpu::CompareFunction::Never;
    case VK_COMPARE_OP_LESS: return wgpu::CompareFunction::Less;
    case VK_COMPARE_OP_EQUAL: return wgpu::CompareFunction::Equal;
    case VK_COMPARE_OP_LESS_OR_EQUAL: return wgpu::CompareFunction::LessEqual;
    case VK_COMPARE_OP_GREATER: return wgpu::CompareFunction::Greater;
    case VK_COMPARE_OP_NOT_EQUAL: return wgpu::CompareFunction::NotEqual;
    case VK_COMPARE_OP_GREATER_OR_EQUAL: return wgpu::CompareFunction::GreaterEqual;
    case VK_COMPARE_OP_ALWAYS: return wgpu::CompareFunction::Always;
    default: break;
    }
    throw std::runtime_error("WebGPU: compare operation " + std::to_string(static_cast<int>(operation)) + " is not supported");
}

wgpu::StencilOperation StencilOperation(VkStencilOp operation) {
    switch (operation) {
    case VK_STENCIL_OP_KEEP: return wgpu::StencilOperation::Keep;
    case VK_STENCIL_OP_ZERO: return wgpu::StencilOperation::Zero;
    case VK_STENCIL_OP_REPLACE: return wgpu::StencilOperation::Replace;
    case VK_STENCIL_OP_INCREMENT_AND_CLAMP: return wgpu::StencilOperation::IncrementClamp;
    case VK_STENCIL_OP_DECREMENT_AND_CLAMP: return wgpu::StencilOperation::DecrementClamp;
    case VK_STENCIL_OP_INVERT: return wgpu::StencilOperation::Invert;
    case VK_STENCIL_OP_INCREMENT_AND_WRAP: return wgpu::StencilOperation::IncrementWrap;
    case VK_STENCIL_OP_DECREMENT_AND_WRAP: return wgpu::StencilOperation::DecrementWrap;
    default: break;
    }
    throw std::runtime_error("WebGPU: stencil operation " + std::to_string(static_cast<int>(operation)) + " is not supported");
}

wgpu::StencilFaceState StencilFace(const VkStencilOpState& face) {
    return {CompareFunction(face.compareOp), StencilOperation(face.failOp), StencilOperation(face.depthFailOp), StencilOperation(face.passOp)};
}

std::optional<wgpu::VertexFormat> DirectVertexFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R32_SFLOAT: return wgpu::VertexFormat::Float32;
    case VK_FORMAT_R32G32_SFLOAT: return wgpu::VertexFormat::Float32x2;
    case VK_FORMAT_R32G32B32_SFLOAT: return wgpu::VertexFormat::Float32x3;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return wgpu::VertexFormat::Float32x4;
    case VK_FORMAT_R32_UINT: return wgpu::VertexFormat::Uint32;
    case VK_FORMAT_R32G32_UINT: return wgpu::VertexFormat::Uint32x2;
    case VK_FORMAT_R32G32B32_UINT: return wgpu::VertexFormat::Uint32x3;
    case VK_FORMAT_R32G32B32A32_UINT: return wgpu::VertexFormat::Uint32x4;
    case VK_FORMAT_R32_SINT: return wgpu::VertexFormat::Sint32;
    case VK_FORMAT_R32G32_SINT: return wgpu::VertexFormat::Sint32x2;
    case VK_FORMAT_R32G32B32_SINT: return wgpu::VertexFormat::Sint32x3;
    case VK_FORMAT_R32G32B32A32_SINT: return wgpu::VertexFormat::Sint32x4;
    case VK_FORMAT_R8G8B8A8_UNORM: return wgpu::VertexFormat::Unorm8x4;
    case VK_FORMAT_R8G8B8A8_SNORM: return wgpu::VertexFormat::Snorm8x4;
    case VK_FORMAT_R8G8B8A8_UINT: return wgpu::VertexFormat::Uint8x4;
    case VK_FORMAT_R8G8B8A8_SINT: return wgpu::VertexFormat::Sint8x4;
    case VK_FORMAT_R8G8_UNORM: return wgpu::VertexFormat::Unorm8x2;
    case VK_FORMAT_R16G16_SFLOAT: return wgpu::VertexFormat::Float16x2;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return wgpu::VertexFormat::Float16x4;
    case VK_FORMAT_R16G16_UNORM: return wgpu::VertexFormat::Unorm16x2;
    case VK_FORMAT_R16G16B16A16_UNORM: return wgpu::VertexFormat::Unorm16x4;
    case VK_FORMAT_R8_UNORM: return wgpu::VertexFormat::Unorm8;
    case VK_FORMAT_R8_SNORM: return wgpu::VertexFormat::Snorm8;
    case VK_FORMAT_R8_UINT: return wgpu::VertexFormat::Uint8;
    case VK_FORMAT_R8_SINT: return wgpu::VertexFormat::Sint8;
    case VK_FORMAT_R8G8_SNORM: return wgpu::VertexFormat::Snorm8x2;
    case VK_FORMAT_R8G8_UINT: return wgpu::VertexFormat::Uint8x2;
    case VK_FORMAT_R8G8_SINT: return wgpu::VertexFormat::Sint8x2;
    case VK_FORMAT_R16_UNORM: return wgpu::VertexFormat::Unorm16;
    case VK_FORMAT_R16_SNORM: return wgpu::VertexFormat::Snorm16;
    case VK_FORMAT_R16_UINT: return wgpu::VertexFormat::Uint16;
    case VK_FORMAT_R16_SINT: return wgpu::VertexFormat::Sint16;
    case VK_FORMAT_R16_SFLOAT: return wgpu::VertexFormat::Float16;
    case VK_FORMAT_R16G16_SNORM: return wgpu::VertexFormat::Snorm16x2;
    case VK_FORMAT_R16G16_UINT: return wgpu::VertexFormat::Uint16x2;
    case VK_FORMAT_R16G16_SINT: return wgpu::VertexFormat::Sint16x2;
    case VK_FORMAT_R16G16B16A16_SNORM: return wgpu::VertexFormat::Snorm16x4;
    case VK_FORMAT_R16G16B16A16_UINT: return wgpu::VertexFormat::Uint16x4;
    case VK_FORMAT_R16G16B16A16_SINT: return wgpu::VertexFormat::Sint16x4;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return wgpu::VertexFormat::Unorm10_10_10_2;
    default: break;
    }
    return std::nullopt;
}

enum class ComponentKind { Unorm, Snorm, Uscaled, Sscaled, Uint, Sint, Float };

struct ConvertedVertexFormat {
    ComponentKind kind;
    std::uint32_t bits;
    std::uint32_t components;
};

ConvertedVertexFormat VertexComponents(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_USCALED: return {ComponentKind::Uscaled, 8, 1};
    case VK_FORMAT_R8_SSCALED: return {ComponentKind::Sscaled, 8, 1};
    case VK_FORMAT_R8G8_USCALED: return {ComponentKind::Uscaled, 8, 2};
    case VK_FORMAT_R8G8_SSCALED: return {ComponentKind::Sscaled, 8, 2};
    case VK_FORMAT_R8G8B8_UNORM: return {ComponentKind::Unorm, 8, 3};
    case VK_FORMAT_R8G8B8_SNORM: return {ComponentKind::Snorm, 8, 3};
    case VK_FORMAT_R8G8B8_USCALED: return {ComponentKind::Uscaled, 8, 3};
    case VK_FORMAT_R8G8B8_SSCALED: return {ComponentKind::Sscaled, 8, 3};
    case VK_FORMAT_R8G8B8_UINT: return {ComponentKind::Uint, 8, 3};
    case VK_FORMAT_R8G8B8_SINT: return {ComponentKind::Sint, 8, 3};
    case VK_FORMAT_R8G8B8A8_USCALED: return {ComponentKind::Uscaled, 8, 4};
    case VK_FORMAT_R8G8B8A8_SSCALED: return {ComponentKind::Sscaled, 8, 4};
    case VK_FORMAT_R16_USCALED: return {ComponentKind::Uscaled, 16, 1};
    case VK_FORMAT_R16_SSCALED: return {ComponentKind::Sscaled, 16, 1};
    case VK_FORMAT_R16G16_USCALED: return {ComponentKind::Uscaled, 16, 2};
    case VK_FORMAT_R16G16_SSCALED: return {ComponentKind::Sscaled, 16, 2};
    case VK_FORMAT_R16G16B16_UNORM: return {ComponentKind::Unorm, 16, 3};
    case VK_FORMAT_R16G16B16_SNORM: return {ComponentKind::Snorm, 16, 3};
    case VK_FORMAT_R16G16B16_USCALED: return {ComponentKind::Uscaled, 16, 3};
    case VK_FORMAT_R16G16B16_SSCALED: return {ComponentKind::Sscaled, 16, 3};
    case VK_FORMAT_R16G16B16_UINT: return {ComponentKind::Uint, 16, 3};
    case VK_FORMAT_R16G16B16_SINT: return {ComponentKind::Sint, 16, 3};
    case VK_FORMAT_R16G16B16_SFLOAT: return {ComponentKind::Float, 16, 3};
    case VK_FORMAT_R16G16B16A16_USCALED: return {ComponentKind::Uscaled, 16, 4};
    case VK_FORMAT_R16G16B16A16_SSCALED: return {ComponentKind::Sscaled, 16, 4};
    default: break;
    }
    throw std::runtime_error("WebGPU: VkFormat " + std::to_string(static_cast<int>(format)) + " has no vertex format");
}

float HalfToFloat(std::uint16_t half) {
    const std::uint32_t sign = static_cast<std::uint32_t>(half & 0x8000u) << 16u;
    const std::uint32_t exponent = (half >> 10u) & 0x1fu;
    std::uint32_t mantissa = half & 0x3ffu;
    std::uint32_t bits;
    if (exponent == 0x1fu) {
        bits = sign | 0x7f800000u | (mantissa << 13u);
    } else if (exponent != 0) {
        bits = sign | ((exponent + 112u) << 23u) | (mantissa << 13u);
    } else if (mantissa == 0) {
        bits = sign;
    } else {
        std::uint32_t shift = 0;
        while ((mantissa & 0x400u) == 0) {
            mantissa <<= 1u;
            ++shift;
        }
        bits = sign | ((113u - shift) << 23u) | ((mantissa & 0x3ffu) << 13u);
    }
    return std::bit_cast<float>(bits);
}

std::uint32_t ConvertedComponent(ComponentKind kind, std::uint32_t bits, std::uint32_t raw) {
    const auto signedValue = static_cast<std::int32_t>(raw << (32u - bits)) >> (32u - bits);
    const auto maximum = static_cast<float>((1u << bits) - 1u);
    const auto signedMaximum = static_cast<float>((1u << (bits - 1u)) - 1u);
    switch (kind) {
    case ComponentKind::Unorm: return std::bit_cast<std::uint32_t>(static_cast<float>(raw) / maximum);
    case ComponentKind::Snorm: return std::bit_cast<std::uint32_t>(std::max(static_cast<float>(signedValue) / signedMaximum, -1.0f));
    case ComponentKind::Uscaled: return std::bit_cast<std::uint32_t>(static_cast<float>(raw));
    case ComponentKind::Sscaled: return std::bit_cast<std::uint32_t>(static_cast<float>(signedValue));
    case ComponentKind::Uint: return raw;
    case ComponentKind::Sint: return static_cast<std::uint32_t>(signedValue);
    case ComponentKind::Float: return std::bit_cast<std::uint32_t>(HalfToFloat(static_cast<std::uint16_t>(raw)));
    }
    return 0;
}

std::vector<std::byte> ConvertVertexStream(VkFormat format, std::span<const std::byte> data, std::uint32_t stride, std::uint32_t count, wgpu::VertexFormat& converted) {
    const auto layout = VertexComponents(format);
    const bool integer = layout.kind == ComponentKind::Uint || layout.kind == ComponentKind::Sint;
    converted = layout.kind == ComponentKind::Uint ? wgpu::VertexFormat::Uint32x4 : layout.kind == ComponentKind::Sint ? wgpu::VertexFormat::Sint32x4 : wgpu::VertexFormat::Float32x4;
    const std::uint32_t one = integer ? 1u : 0x3f800000u;
    const auto componentBytes = layout.bits / 8u;
    std::vector<std::byte> result(static_cast<std::size_t>(count) * 16u);
    for (std::uint32_t element = 0; element < count; ++element) {
        std::array<std::uint32_t, 4> values{0u, 0u, 0u, one};
        for (std::uint32_t component = 0; component < layout.components; ++component) {
            const auto offset = static_cast<std::size_t>(element) * stride + component * componentBytes;
            Require(offset + componentBytes <= data.size(), "a converted vertex stream reads past its data");
            std::uint32_t raw = 0;
            std::memcpy(&raw, data.data() + offset, componentBytes);
            values[component] = ConvertedComponent(layout.kind, layout.bits, raw);
        }
        std::memcpy(result.data() + static_cast<std::size_t>(element) * 16u, values.data(), 16u);
    }
    return result;
}

wgpu::BlendFactor BlendFactor(VkBlendFactor factor) {
    switch (factor) {
    case VK_BLEND_FACTOR_ZERO: return wgpu::BlendFactor::Zero;
    case VK_BLEND_FACTOR_ONE: return wgpu::BlendFactor::One;
    case VK_BLEND_FACTOR_SRC_COLOR: return wgpu::BlendFactor::Src;
    case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return wgpu::BlendFactor::OneMinusSrc;
    case VK_BLEND_FACTOR_DST_COLOR: return wgpu::BlendFactor::Dst;
    case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return wgpu::BlendFactor::OneMinusDst;
    case VK_BLEND_FACTOR_SRC_ALPHA: return wgpu::BlendFactor::SrcAlpha;
    case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return wgpu::BlendFactor::OneMinusSrcAlpha;
    case VK_BLEND_FACTOR_DST_ALPHA: return wgpu::BlendFactor::DstAlpha;
    case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return wgpu::BlendFactor::OneMinusDstAlpha;
    case VK_BLEND_FACTOR_CONSTANT_COLOR:
    case VK_BLEND_FACTOR_CONSTANT_ALPHA: return wgpu::BlendFactor::Constant;
    case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
    case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return wgpu::BlendFactor::OneMinusConstant;
    case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE: return wgpu::BlendFactor::SrcAlphaSaturated;
    default: break;
    }
    throw std::runtime_error("WebGPU: blend factor " + std::to_string(static_cast<int>(factor)) + " is not supported");
}

wgpu::BlendOperation BlendOperation(VkBlendOp operation) {
    switch (operation) {
    case VK_BLEND_OP_ADD: return wgpu::BlendOperation::Add;
    case VK_BLEND_OP_SUBTRACT: return wgpu::BlendOperation::Subtract;
    case VK_BLEND_OP_REVERSE_SUBTRACT: return wgpu::BlendOperation::ReverseSubtract;
    case VK_BLEND_OP_MIN: return wgpu::BlendOperation::Min;
    case VK_BLEND_OP_MAX: return wgpu::BlendOperation::Max;
    default: break;
    }
    throw std::runtime_error("WebGPU: blend operation " + std::to_string(static_cast<int>(operation)) + " is not supported");
}

wgpu::AddressMode AddressMode(VkSamplerAddressMode mode) {
    switch (mode) {
    case VK_SAMPLER_ADDRESS_MODE_REPEAT: return wgpu::AddressMode::Repeat;
    case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: return wgpu::AddressMode::MirrorRepeat;
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
    case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: return wgpu::AddressMode::ClampToEdge;
    default: break;
    }
    throw std::runtime_error("WebGPU: sampler address mode " + std::to_string(static_cast<int>(mode)) + " is not supported");
}

const char* StorageFormatName(wgpu::TextureFormat format) {
    switch (format) {
    case wgpu::TextureFormat::RGBA8Unorm: return "rgba8unorm";
    case wgpu::TextureFormat::BGRA8Unorm: return "bgra8unorm";
    case wgpu::TextureFormat::RGBA16Float: return "rgba16float";
    case wgpu::TextureFormat::R32Float: return "r32float";
    case wgpu::TextureFormat::RGBA32Float: return "rgba32float";
    case wgpu::TextureFormat::R8Unorm: return "r8unorm";
    case wgpu::TextureFormat::RG8Unorm: return "rg8unorm";
    case wgpu::TextureFormat::R16Float: return "r16float";
    case wgpu::TextureFormat::RG16Float: return "rg16float";
    case wgpu::TextureFormat::RGB10A2Unorm: return "rgb10a2unorm";
    default: break;
    }
    throw std::runtime_error("WebGPU: texture format " + std::to_string(static_cast<int>(format)) + " cannot be written as a storage image");
}

bool TierOneStorageFormat(wgpu::TextureFormat format) {
    return format == wgpu::TextureFormat::R8Unorm || format == wgpu::TextureFormat::RG8Unorm || format == wgpu::TextureFormat::R16Float || format == wgpu::TextureFormat::RG16Float || format == wgpu::TextureFormat::RGB10A2Unorm;
}

std::uint32_t Norm16Components(wgpu::TextureFormat format) {
    switch (format) {
    case wgpu::TextureFormat::R16Unorm:
    case wgpu::TextureFormat::R16Snorm: return 1u;
    case wgpu::TextureFormat::RG16Unorm:
    case wgpu::TextureFormat::RG16Snorm: return 2u;
    case wgpu::TextureFormat::RGBA16Unorm:
    case wgpu::TextureFormat::RGBA16Snorm: return 4u;
    default: return 0u;
    }
}

bool Float32Format(wgpu::TextureFormat format) {
    return format == wgpu::TextureFormat::R32Float || format == wgpu::TextureFormat::RGBA32Float;
}

bool EightBitColor(wgpu::TextureFormat format) {
    return format == wgpu::TextureFormat::RGBA8Unorm || format == wgpu::TextureFormat::RGBA8UnormSrgb || format == wgpu::TextureFormat::BGRA8Unorm || format == wgpu::TextureFormat::BGRA8UnormSrgb;
}

bool SameTexels(wgpu::TextureFormat target, wgpu::TextureFormat sampled) {
    return target == sampled || (EightBitColor(target) && EightBitColor(sampled));
}

std::string BindingDeclaration(const std::string& source, std::uint32_t binding) {
    const auto found = source.find("@binding(" + std::to_string(binding) + ")");
    if (found == std::string::npos) return {};
    const auto end = source.find('\n', found);
    return source.substr(found, end == std::string::npos ? std::string::npos : end - found);
}

void SpecializeStorageFormat(std::string& source, std::uint32_t binding, const char* format) {
    const auto found = source.find("@binding(" + std::to_string(binding) + ")");
    Require(found != std::string::npos, "a storage image binding is not declared");
    const auto token = source.find("aps5_storage_format", found);
    Require(token != std::string::npos && token < source.find('\n', found), "a storage image declaration has no format to fill in");
    source.replace(token, std::string("aps5_storage_format").size(), format);
}

std::set<std::uint32_t> DeclaredBindings(const std::string& source, bool writableOnly = false) {
    std::set<std::uint32_t> result;
    for (auto found = source.find("@binding("); found != std::string::npos; found = source.find("@binding(", found + 1)) {
        const auto end = source.find('\n', found);
        if (writableOnly && source.substr(found, end == std::string::npos ? std::string::npos : end - found).find("read_write") == std::string::npos) continue;
        result.insert(static_cast<std::uint32_t>(std::stoul(source.substr(found + 9))));
    }
    return result;
}

void WriteChanged(std::uint64_t address, std::span<const std::byte> original, const std::byte* result, std::size_t bytes) {
    auto* guest = reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(address));
    for (std::size_t offset = 0; offset < bytes;) {
        if (result[offset] == original[offset]) {
            ++offset;
            continue;
        }
        auto end = offset + 1;
        while (end < bytes && result[end] != original[end]) ++end;
        std::memcpy(guest + offset, result + offset, end - offset);
        offset = end;
    }
}

std::string MatchVertexOutputs(std::string vertex, const std::string& pixel) {
    for (auto found = pixel.find("@location("); found != std::string::npos; found = pixel.find("@location(", found + 1)) {
        const auto location = static_cast<std::uint32_t>(std::stoul(pixel.substr(found + 10)));
        const auto end = pixel.find('\n', found);
        const auto declaration = pixel.substr(found, end == std::string::npos ? std::string::npos : end - found);
        if (declaration.find("aps5_parameter") == std::string::npos) continue;
        vertex = ShaderRecompiler::SpecializeWgslVertexOutput(std::move(vertex), location, declaration.find("@interpolate(flat)") != std::string::npos, declaration.find("@interpolate(linear") != std::string::npos);
    }
    return vertex;
}

enum class ResourceKind { Buffer, Texture, DepthTexture, StorageTexture, ImageBuffer, Sampler };

struct TextureLevel {
    std::uint32_t mip = 0;
    std::uint32_t layer = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t bytesPerRow = 0;
    std::uint32_t rows = 0;
    std::vector<std::byte> bytes;
};

struct TextureSource {
    std::uint64_t address = 0;
    bool reuse = false;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm;
    std::uint32_t levelCount = 1;
    std::uint32_t baseLevel = 0;
    std::uint32_t viewLevels = 1;
    std::uint32_t layers = 1;
    bool arrayed = false;
    bool volume = false;
    bool depthPlane = false;
    std::uint32_t baseLayer = 0;
    std::uint32_t viewLayers = 1;
    std::vector<TextureLevel> levels;
    std::vector<std::uint64_t> depthSlices;
};

struct SamplerKey {
    wgpu::FilterMode mag = wgpu::FilterMode::Nearest;
    wgpu::FilterMode min = wgpu::FilterMode::Nearest;
    wgpu::MipmapFilterMode mip = wgpu::MipmapFilterMode::Nearest;
    wgpu::AddressMode u = wgpu::AddressMode::ClampToEdge;
    wgpu::AddressMode v = wgpu::AddressMode::ClampToEdge;
    wgpu::AddressMode w = wgpu::AddressMode::ClampToEdge;
    float minLod = 0.0f;
    float maxLod = 32.0f;
    wgpu::CompareFunction compare = wgpu::CompareFunction::Undefined;

    auto Tie() const { return std::tie(mag, min, mip, u, v, w, minLod, maxLod, compare); }
    bool operator<(const SamplerKey& other) const { return Tie() < other.Tie(); }
};

struct Resource {
    std::uint32_t binding = 0;
    ResourceKind kind = ResourceKind::Buffer;
    std::uint64_t address = 0;
    std::uint64_t size = 0;
    bool truncated = false;
    std::uint32_t bytesPerRow = 0;
    std::vector<std::byte> bytes;
    TextureSource texture;
    SamplerKey sampler;
};

struct StageJob {
    std::string source;
    std::vector<Resource> resources;
};

struct VertexStream {
    std::uint32_t location;
    wgpu::VertexFormat format;
    std::uint32_t stride;
    std::vector<std::byte> bytes;
};

struct TargetInfo {
    std::uint64_t address = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm;
    std::uint32_t layers = 1;
};

struct DepthInfo {
    std::uint64_t address = 0;
    std::uint64_t htileAddress = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    wgpu::TextureFormat format = wgpu::TextureFormat::Depth32Float;
    float clearDepth = 0.0f;
    std::uint32_t clearStencil = 0;
    bool depthCleared = false;
    bool stencilCleared = false;
};

struct DrawJob {
    StageJob vertex;
    std::optional<StageJob> pixel;
    std::vector<VertexStream> streams;
    std::vector<std::byte> indices;
    wgpu::IndexFormat indexFormat = wgpu::IndexFormat::Undefined;
    std::uint32_t count = 0;
    std::uint32_t instances = 1;
    std::uint32_t firstVertex = 0;
    std::uint32_t firstInstance = 0;
    bool rect = false;
    wgpu::PrimitiveTopology topology = wgpu::PrimitiveTopology::TriangleList;
    wgpu::CullMode cull = wgpu::CullMode::None;
    wgpu::FrontFace front = wgpu::FrontFace::CCW;
    bool colorTarget = true;
    TargetInfo target;
    std::optional<DepthInfo> depth;
    wgpu::DepthStencilState depthState;
    std::uint32_t stencilReference = 0;
    bool unclippedDepth = false;
    bool blend = false;
    wgpu::BlendComponent colorBlend;
    wgpu::BlendComponent alpha;
    wgpu::ColorWriteMask writeMask = wgpu::ColorWriteMask::All;
    std::array<double, 4> constants{};
    std::array<float, 6> viewport{};
    std::array<std::uint32_t, 4> scissor{};
    bool flipY = false;
    bool depthHalf = false;
};

struct ComputeJob {
    StageJob stage;
    std::set<std::uint32_t> writable;
    std::array<std::uint32_t, 3> groups{};
    std::uint64_t htileClear = 0;
    std::size_t htileBytes = 0;
};

struct PresentJob {
    AgcDriver::DisplayBuffer buffer;
    std::vector<std::byte> pixels;
};

constexpr const char* PresentShader = R"(@group(0) @binding(0) var aps5_source: texture_2d<f32>;
@group(0) @binding(1) var aps5_sampler: sampler;

struct Aps5Varying {
    @builtin(position) position: vec4<f32>,
    @location(0) coordinate: vec2<f32>,
}

@vertex
fn vertexMain(@builtin(vertex_index) index: u32) -> Aps5Varying {
    let coordinate = vec2<f32>(f32((index << 1u) & 2u), f32(index & 2u));
    var result: Aps5Varying;
    result.position = vec4<f32>(coordinate.x * 2.0 - 1.0, 1.0 - coordinate.y * 2.0, 0.0, 1.0);
    result.coordinate = coordinate;
    return result;
}

@fragment
fn fragmentMain(input: Aps5Varying) -> @location(0) vec4<f32> {
    return vec4<f32>(textureSample(aps5_source, aps5_sampler, input.coordinate).rgb, 1.0);
}
)";

class Device {
public:
    static Device& Get() {
        static Device device;
        return device;
    }

    void Post(const char* name, std::function<void()> task) {
        auto* heap = new Task{name, std::move(task)};
        if (!emscripten_proxy_async(queue, thread, [](void* argument) {
                std::unique_ptr<Task> task(static_cast<Task*>(argument));
                const auto started = emscripten_get_now();
                try {
                    task->run();
                } catch (const std::exception& error) {
                    WasmRuntime::GuestFault("WebGPU thread failed: %s", error.what());
                }
                if (const auto elapsed = emscripten_get_now() - started; elapsed > 1000.0) std::fprintf(stderr, "[webgpu] %s took %.0f ms on the GPU thread\n", task->name, elapsed);
            }, heap)) {
            delete heap;
            throw std::runtime_error("WebGPU: cannot post to the GPU thread");
        }
    }

    void NoteTarget(const TargetInfo& target) {
        std::lock_guard lock(mutex);
        targetShapes[target.address] = target;
    }

    bool IsTarget(std::uint64_t address) {
        std::lock_guard lock(mutex);
        return targetShapes.contains(address);
    }

    bool HoldsTarget(const TextureSource& source) {
        std::lock_guard lock(mutex);
        const auto found = targetShapes.find(source.address);
        if (found == targetShapes.end()) return false;
        const auto& target = found->second;
        return target.width == source.width && target.height == source.height && target.layers == source.layers && SameTexels(target.format, source.format);
    }

    void NoteDepthTarget(std::uint64_t address) {
        std::lock_guard lock(mutex);
        depthAddresses.insert(address);
    }

    bool IsDepthTarget(std::uint64_t address) {
        std::lock_guard lock(mutex);
        return depthAddresses.contains(address);
    }

    bool Supports(wgpu::FeatureName feature) const {
        return features.contains(feature);
    }

    std::uint32_t StorageTextureLimit() const {
        return limits.maxStorageTexturesPerShaderStage;
    }

    std::uint32_t StorageBufferLimit() const {
        return limits.maxStorageBuffersPerShaderStage;
    }

    std::uint64_t StorageBufferBindingLimit() const {
        return limits.maxStorageBufferBindingSize;
    }

    void NoteDepthClear(std::uint64_t address, std::size_t bytes) {
        std::lock_guard lock(mutex);
        depthClears.emplace_back(address, bytes);
    }

    bool TakeDepthClear(std::uint64_t htile, std::uint32_t width, std::uint32_t height) {
        std::lock_guard lock(mutex);
        return std::erase_if(depthClears, [&](const auto& clear) { return AgcDriver::Graphics::HtileFillCovers(htile, {width, height}, clear.first, clear.second); }) != 0;
    }

    bool Unchanged(const TextureSource& source, std::uint64_t hash) {
        std::lock_guard lock(mutex);
        auto& known = uploaded[source.address];
        const auto key = std::make_tuple(hash, source.width, source.height, source.format, source.levelCount * 65536u + source.layers + (source.volume ? 0x80000000u : 0u));
        if (known == key) return true;
        known = key;
        return false;
    }

    void Draw(std::shared_ptr<DrawJob> job) {
        Post("a draw", [this, job] { executeDraw(*job); });
    }

    void Compute(std::shared_ptr<const ComputeJob> job) {
        auto done = std::make_shared<std::promise<void>>();
        auto finished = done->get_future();
        Post("a dispatch", [this, job, done] { executeCompute(job, done); });
        finished.get();
    }

    void Present(std::shared_ptr<PresentJob> job) {
        {
            std::unique_lock lock(mutex);
            changed.wait(lock, [&] { return presentsDone + 1 >= presentsPosted; });
            ++presentsPosted;
        }
        Post("a present", [this, job] {
            executePresent(*job);
            gpuQueue.OnSubmittedWorkDone(wgpu::CallbackMode::AllowSpontaneous, [this](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                std::lock_guard lock(mutex);
                ++presentsDone;
                changed.notify_all();
            });
        });
    }

    void AfterWork(std::function<void()> completion) {
        Post("a completion", std::move(completion));
    }

private:
    struct RenderTarget {
        wgpu::Texture texture;
        wgpu::TextureView view;
        TargetInfo info;
        bool initialized = false;
        bool storage = false;
    };

    struct Task {
        const char* name;
        std::function<void()> run;
    };

    struct DepthTexture {
        wgpu::Texture texture;
        wgpu::TextureView view;
        DepthInfo info;
        bool initialized = false;
        std::uint64_t serial = 0;
    };

    struct DepthArray {
        wgpu::Texture texture;
        wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<std::uint64_t> serials;
    };

    struct Upload {
        wgpu::Texture texture;
        wgpu::TextureView view;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;
        std::uint32_t levelCount = 0;
        std::uint32_t layers = 0;
        bool volume = false;
    };

    Device() {
        queue = em_proxying_queue_create();
        pthread_attr_t attributes;
        pthread_attr_init(&attributes);
        emscripten_pthread_attr_settransferredcanvases(&attributes, "#canvas");
        Require(pthread_create(&thread, &attributes, &Device::Run, this) == 0, "cannot create the GPU thread");
        pthread_attr_destroy(&attributes);
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return state != 0; });
        Require(state > 0, "the GPU device is not available: " + failure);
    }

    static void* Run(void* argument) {
        static_cast<Device*>(argument)->initialize();
        emscripten_runtime_keepalive_push();
        return nullptr;
    }

    void finishInitialization(int result, std::string reason = {}) {
        std::lock_guard lock(mutex);
        state = result;
        failure = std::move(reason);
        changed.notify_all();
    }

    void initialize() {
        instance = wgpu::CreateInstance(nullptr);
        wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvas;
        canvas.selector = "#canvas";
        wgpu::SurfaceDescriptor surfaceDescriptor;
        surfaceDescriptor.nextInChain = &canvas;
        surface = instance.CreateSurface(&surfaceDescriptor);
        wgpu::RequestAdapterOptions options;
        options.compatibleSurface = surface;
        options.powerPreference = wgpu::PowerPreference::HighPerformance;
        instance.RequestAdapter(&options, wgpu::CallbackMode::AllowSpontaneous, [this](wgpu::RequestAdapterStatus status, wgpu::Adapter result, wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) return finishInitialization(-1, std::string(message.data, message.length));
            adapter = std::move(result);
            wgpu::Limits supported;
            adapter.GetLimits(&supported);
            wgpu::Limits required;
            required.maxTextureDimension2D = supported.maxTextureDimension2D;
            required.maxStorageTexturesPerShaderStage = supported.maxStorageTexturesPerShaderStage;
            required.maxStorageBuffersPerShaderStage = supported.maxStorageBuffersPerShaderStage;
            required.maxStorageBufferBindingSize = supported.maxStorageBufferBindingSize;
            required.maxBufferSize = supported.maxBufferSize;
            required.maxComputeWorkgroupStorageSize = supported.maxComputeWorkgroupStorageSize;
            required.maxComputeInvocationsPerWorkgroup = supported.maxComputeInvocationsPerWorkgroup;
            required.maxComputeWorkgroupSizeX = supported.maxComputeWorkgroupSizeX;
            required.maxComputeWorkgroupSizeY = supported.maxComputeWorkgroupSizeY;
            required.maxComputeWorkgroupSizeZ = supported.maxComputeWorkgroupSizeZ;
            std::vector<wgpu::FeatureName> wanted;
            for (const auto feature : {wgpu::FeatureName::Depth32FloatStencil8, wgpu::FeatureName::DepthClipControl, wgpu::FeatureName::TextureCompressionBC, wgpu::FeatureName::BGRA8UnormStorage, wgpu::FeatureName::Subgroups, wgpu::FeatureName::TextureFormatsTier1, wgpu::FeatureName::Float32Filterable}) {
                if (!adapter.HasFeature(feature)) continue;
                wanted.push_back(feature);
                features.insert(feature);
            }
            wgpu::DeviceDescriptor descriptor;
            descriptor.requiredLimits = &required;
            descriptor.requiredFeatureCount = wanted.size();
            descriptor.requiredFeatures = wanted.data();
            descriptor.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
                static std::mutex reported;
                static std::set<std::string> seen;
                const std::string text(message.data, message.length);
                std::lock_guard lock(reported);
                if (seen.size() < 200 && seen.insert(text).second) std::fprintf(stderr, "[webgpu] error %d: %s\n", static_cast<int>(type), text.c_str());
            });
            adapter.RequestDevice(&descriptor, wgpu::CallbackMode::AllowSpontaneous, [this](wgpu::RequestDeviceStatus status, wgpu::Device result, wgpu::StringView message) {
                if (status != wgpu::RequestDeviceStatus::Success) return finishInitialization(-1, std::string(message.data, message.length));
                device = std::move(result);
                gpuQueue = device.GetQueue();
                wgpu::SurfaceCapabilities capabilities;
                surface.GetCapabilities(adapter, &capabilities);
                surfaceFormat = capabilities.formatCount != 0 ? capabilities.formats[0] : wgpu::TextureFormat::BGRA8Unorm;
                wgpu::SurfaceConfiguration configuration;
                configuration.device = device;
                configuration.format = surfaceFormat;
                configuration.usage = wgpu::TextureUsage::RenderAttachment;
                configuration.width = CanvasWidth;
                configuration.height = CanvasHeight;
                configuration.alphaMode = wgpu::CompositeAlphaMode::Opaque;
                configuration.presentMode = wgpu::PresentMode::Fifo;
                surface.Configure(&configuration);
                device.GetLimits(&limits);
                std::fprintf(stderr, "[webgpu] device ready, canvas format %d, %zu features, limits: %u storage textures, %u storage buffers, %u invocations, %u bytes of workgroup memory\n", static_cast<int>(surfaceFormat), features.size(), limits.maxStorageTexturesPerShaderStage, limits.maxStorageBuffersPerShaderStage, limits.maxComputeInvocationsPerWorkgroup, limits.maxComputeWorkgroupStorageSize);
                finishInitialization(1);
            });
        });
    }

    wgpu::ShaderModule module(const std::string& source) {
        if (const auto found = modules.find(source); found != modules.end()) return found->second;
        wgpu::ShaderSourceWGSL wgsl;
        wgsl.code = source.c_str();
        wgpu::ShaderModuleDescriptor descriptor;
        descriptor.nextInChain = &wgsl;
        auto result = device.CreateShaderModule(&descriptor);
        modules.emplace(source, result);
        return result;
    }

    RenderTarget& renderTarget(const TargetInfo& info, bool storage = false) {
        auto& target = targets[info.address];
        const bool same = target.texture && target.info.width == info.width && target.info.height == info.height && target.info.format == info.format && target.info.layers == info.layers;
        if (same && (target.storage || !storage)) return target;
        wgpu::TextureDescriptor descriptor;
        descriptor.size = {info.width, info.height, info.layers};
        descriptor.format = info.format;
        descriptor.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
        if (storage || (same && target.storage)) descriptor.usage |= wgpu::TextureUsage::StorageBinding;
        auto texture = device.CreateTexture(&descriptor);
        wgpu::Texture previous = same && target.initialized ? target.texture : nullptr;
        if (!previous && storage) {
            if (const auto upload = uploads.find(info.address); upload != uploads.end() && upload->second.width == info.width && upload->second.height == info.height && upload->second.format == info.format && upload->second.layers == info.layers && upload->second.levelCount == 1u) previous = upload->second.texture;
        }
        if (previous) {
            wgpu::TexelCopyTextureInfo from;
            from.texture = previous;
            wgpu::TexelCopyTextureInfo to;
            to.texture = texture;
            const wgpu::Extent3D extent{info.width, info.height, info.layers};
            auto encoder = device.CreateCommandEncoder();
            encoder.CopyTextureToTexture(&from, &to, &extent);
            auto commands = encoder.Finish();
            gpuQueue.Submit(1, &commands);
        }
        target.texture = texture;
        target.view = target.texture.CreateView();
        target.info = info;
        target.initialized = previous != nullptr;
        target.storage = storage || (same && target.storage);
        return target;
    }

    DepthTexture& depthTexture(const DepthInfo& info) {
        auto& depth = depthTextures[info.address];
        if (depth.texture && depth.info.width == info.width && depth.info.height == info.height && depth.info.format == info.format) return depth;
        wgpu::TextureDescriptor descriptor;
        descriptor.size = {info.width, info.height, 1};
        descriptor.format = info.format;
        descriptor.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc;
        depth.texture = device.CreateTexture(&descriptor);
        depth.view = depth.texture.CreateView();
        depth.info = info;
        depth.initialized = false;
        depth.serial = ++depthWrites;
        return depth;
    }

    static wgpu::TextureView sourceView(const wgpu::Texture& texture, const wgpu::TextureView& whole, std::uint32_t levels, std::uint32_t layers, const TextureSource& source) {
        if (source.baseLevel == 0 && source.viewLevels == levels && !source.arrayed && (source.volume || layers == 1u)) return whole;
        wgpu::TextureViewDescriptor descriptor;
        descriptor.dimension = source.volume ? wgpu::TextureViewDimension::e3D : source.arrayed ? wgpu::TextureViewDimension::e2DArray : wgpu::TextureViewDimension::e2D;
        descriptor.baseMipLevel = source.baseLevel;
        descriptor.mipLevelCount = source.viewLevels;
        descriptor.baseArrayLayer = source.volume ? 0u : source.baseLayer;
        descriptor.arrayLayerCount = source.volume ? 1u : source.viewLayers;
        return texture.CreateView(&descriptor);
    }

    wgpu::TextureView depthArrayView(const TextureSource& source) {
        std::vector<const DepthTexture*> slices;
        for (const auto address : source.depthSlices) {
            const auto found = depthTextures.find(address);
            Require(found != depthTextures.end() && found->second.texture, "a depth comparison samples a layer whose depth target was never drawn");
            Require(found->second.info.width == source.width && found->second.info.height == source.height, "a depth comparison samples a layer whose depth target has another size");
            Require(slices.empty() || found->second.info.format == slices.front()->info.format, "a depth comparison samples layers of different depth formats");
            slices.push_back(&found->second);
        }
        const auto format = slices.front()->info.format;
        const auto layers = static_cast<std::uint32_t>(slices.size());
        auto& array = depthArrays[source.address];
        if (!array.texture || array.format != format || array.width != source.width || array.height != source.height || array.serials.size() != layers) {
            wgpu::TextureDescriptor descriptor;
            descriptor.size = {source.width, source.height, layers};
            descriptor.format = format;
            descriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
            array = {device.CreateTexture(&descriptor), format, source.width, source.height, std::vector<std::uint64_t>(layers, 0u)};
        }
        auto encoder = device.CreateCommandEncoder();
        bool copied = false;
        for (std::uint32_t layer = 0; layer < layers; ++layer) {
            if (array.serials[layer] == slices[layer]->serial) continue;
            wgpu::TexelCopyTextureInfo from;
            from.texture = slices[layer]->texture;
            wgpu::TexelCopyTextureInfo to;
            to.texture = array.texture;
            to.origin = {0, 0, layer};
            const wgpu::Extent3D extent{source.width, source.height, 1};
            encoder.CopyTextureToTexture(&from, &to, &extent);
            array.serials[layer] = slices[layer]->serial;
            copied = true;
        }
        if (copied) {
            auto commands = encoder.Finish();
            gpuQueue.Submit(1, &commands);
        }
        wgpu::TextureViewDescriptor descriptor;
        descriptor.dimension = wgpu::TextureViewDimension::e2DArray;
        descriptor.aspect = wgpu::TextureAspect::DepthOnly;
        descriptor.baseMipLevel = 0;
        descriptor.mipLevelCount = 1;
        descriptor.baseArrayLayer = source.baseLayer;
        descriptor.arrayLayerCount = source.viewLayers;
        return array.texture.CreateView(&descriptor);
    }

    wgpu::TextureView depthView(const TextureSource& source) {
        if (!source.depthSlices.empty()) return depthArrayView(source);
        const auto found = depthTextures.find(source.address);
        Require(found != depthTextures.end() && found->second.texture, "a depth comparison samples a depth target that was never drawn");
        Require(found->second.info.width == source.width && found->second.info.height == source.height, "a depth comparison samples a depth target with another size");
        wgpu::TextureViewDescriptor descriptor;
        descriptor.dimension = source.arrayed ? wgpu::TextureViewDimension::e2DArray : wgpu::TextureViewDimension::e2D;
        descriptor.aspect = wgpu::TextureAspect::DepthOnly;
        descriptor.baseMipLevel = 0;
        descriptor.mipLevelCount = 1;
        descriptor.baseArrayLayer = 0;
        descriptor.arrayLayerCount = 1;
        return found->second.texture.CreateView(&descriptor);
    }

    wgpu::TextureView depthPlaneView(const TextureSource& source) {
        const auto found = depthTextures.find(source.address);
        Require(found != depthTextures.end() && found->second.texture, "a sampled depth target was never drawn");
        const auto& depth = found->second;
        Require(depth.info.width == source.width && depth.info.height == source.height, "a depth target is sampled with another size");
        auto& copy = depthCopies[source.address];
        if (!copy.texture || copy.width != source.width || copy.height != source.height) {
            wgpu::TextureDescriptor descriptor;
            descriptor.size = {source.width, source.height, 1};
            descriptor.format = wgpu::TextureFormat::R32Float;
            descriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::StorageBinding;
            copy.texture = device.CreateTexture(&descriptor);
            copy.view = copy.texture.CreateView();
            copy.width = source.width;
            copy.height = source.height;
        }
        if (!depthCopyPipeline) {
            static constexpr const char* program =
                "@group(0) @binding(0) var depth: texture_depth_2d;\n"
                "@group(0) @binding(1) var plane: texture_storage_2d<r32float, write>;\n"
                "@compute @workgroup_size(8, 8, 1)\n"
                "fn main(@builtin(global_invocation_id) id: vec3<u32>) {\n"
                "    let size = textureDimensions(depth);\n"
                "    if (id.x >= size.x || id.y >= size.y) {\n"
                "        return;\n"
                "    }\n"
                "    textureStore(plane, vec2<i32>(id.xy), vec4<f32>(textureLoad(depth, vec2<i32>(id.xy), 0), 0.0, 0.0, 1.0));\n"
                "}\n";
            wgpu::ComputePipelineDescriptor descriptor;
            descriptor.compute.module = module(program);
            descriptor.compute.entryPoint = "main";
            depthCopyPipeline = device.CreateComputePipeline(&descriptor);
        }
        wgpu::TextureViewDescriptor depthViewDescriptor;
        depthViewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
        depthViewDescriptor.aspect = wgpu::TextureAspect::DepthOnly;
        std::array<wgpu::BindGroupEntry, 2> entries;
        entries[0].binding = 0;
        entries[0].textureView = depth.texture.CreateView(&depthViewDescriptor);
        entries[1].binding = 1;
        entries[1].textureView = copy.view;
        wgpu::BindGroupDescriptor group;
        group.layout = depthCopyPipeline.GetBindGroupLayout(0);
        group.entryCount = entries.size();
        group.entries = entries.data();
        auto encoder = device.CreateCommandEncoder();
        auto pass = encoder.BeginComputePass();
        pass.SetPipeline(depthCopyPipeline);
        pass.SetBindGroup(0, device.CreateBindGroup(&group));
        pass.DispatchWorkgroups((source.width + 7u) / 8u, (source.height + 7u) / 8u, 1u);
        pass.End();
        auto commands = encoder.Finish();
        gpuQueue.Submit(1, &commands);
        return copy.view;
    }

    wgpu::TextureView uploadTexture(const TextureSource& source) {
        if (source.depthPlane) return depthPlaneView(source);
        if (source.reuse) {
            const auto found = uploads.find(source.address);
            Require(found != uploads.end() && found->second.width == source.width && found->second.height == source.height && found->second.format == source.format && found->second.levelCount == source.levelCount && found->second.layers == source.layers && found->second.volume == source.volume, "a reused texture upload is missing");
            return sourceView(found->second.texture, found->second.view, found->second.levelCount, found->second.layers, source);
        }
        if (source.levels.empty()) {
            Require(!source.volume, "3D textures drawn by the GPU are not implemented");
            auto found = targets.find(source.address);
            auto& target = found != targets.end() ? found->second : renderTarget({source.address, source.width, source.height, source.format, source.layers});
            return sourceView(target.texture, target.view, 1u, target.info.layers, source);
        }
        auto& upload = uploads[source.address];
        if (!upload.texture || upload.width != source.width || upload.height != source.height || upload.format != source.format || upload.levelCount != source.levelCount || upload.layers != source.layers || upload.volume != source.volume) {
            wgpu::TextureDescriptor descriptor;
            descriptor.dimension = source.volume ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D;
            descriptor.size = {source.width, source.height, source.layers};
            descriptor.format = source.format;
            descriptor.mipLevelCount = source.levelCount;
            descriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;
            upload.texture = device.CreateTexture(&descriptor);
            upload.view = upload.texture.CreateView();
            upload.width = source.width;
            upload.height = source.height;
            upload.format = source.format;
            upload.levelCount = source.levelCount;
            upload.layers = source.layers;
            upload.volume = source.volume;
        }
        for (const auto& level : source.levels) {
            wgpu::TexelCopyTextureInfo destination;
            destination.texture = upload.texture;
            destination.mipLevel = level.mip;
            destination.origin = {0, 0, level.layer};
            wgpu::TexelCopyBufferLayout layout;
            layout.bytesPerRow = level.bytesPerRow;
            layout.rowsPerImage = level.rows;
            const wgpu::Extent3D extent{level.width, level.height, 1};
            gpuQueue.WriteTexture(&destination, level.bytes.data(), level.bytes.size(), &layout, &extent);
        }
        return sourceView(upload.texture, upload.view, upload.levelCount, upload.layers, source);
    }

    wgpu::Sampler sampler(const SamplerKey& key) {
        if (const auto found = samplers.find(key); found != samplers.end()) return found->second;
        wgpu::SamplerDescriptor descriptor;
        descriptor.addressModeU = key.u;
        descriptor.addressModeV = key.v;
        descriptor.addressModeW = key.w;
        descriptor.magFilter = key.mag;
        descriptor.minFilter = key.min;
        descriptor.mipmapFilter = key.mip;
        descriptor.lodMinClamp = key.minLod;
        descriptor.lodMaxClamp = std::max(key.minLod, key.maxLod);
        descriptor.compare = key.compare;
        auto result = device.CreateSampler(&descriptor);
        samplers.emplace(key, result);
        return result;
    }

    wgpu::Buffer buffer(const std::vector<std::byte>& bytes, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = std::max<std::uint64_t>((bytes.size() + 3u) & ~std::size_t{3}, 16u);
        descriptor.usage = usage | wgpu::BufferUsage::CopyDst;
        auto result = device.CreateBuffer(&descriptor);
        if (!bytes.empty()) gpuQueue.WriteBuffer(result, 0, bytes.data(), bytes.size());
        return result;
    }

    std::string pipelineKey(const DrawJob& job) const {
        std::string key = job.vertex.source;
        key += '\x01';
        if (job.pixel) key += job.pixel->source;
        key += '\x01';
        for (const auto& stream : job.streams) key += std::to_string(stream.location) + ":" + std::to_string(static_cast<int>(stream.format)) + ":" + std::to_string(stream.stride) + ";";
        key += std::to_string(job.colorTarget) + "/" + std::to_string(static_cast<int>(job.target.format)) + "/" + std::to_string(static_cast<int>(job.topology)) + "/" + std::to_string(static_cast<int>(job.indexFormat)) + "/" + std::to_string(job.rect) + "/" + std::to_string(static_cast<int>(job.cull)) + "/" + std::to_string(static_cast<int>(job.front)) + "/" + std::to_string(job.flipY) + "/" + std::to_string(job.depthHalf) + "/" + std::to_string(static_cast<std::uint64_t>(job.writeMask));
        if (job.blend) {
            for (const auto& component : {job.colorBlend, job.alpha}) key += "/" + std::to_string(static_cast<int>(component.operation)) + "," + std::to_string(static_cast<int>(component.srcFactor)) + "," + std::to_string(static_cast<int>(component.dstFactor));
        }
        if (job.depth) {
            const auto& depth = job.depthState;
            key += "/depth " + std::to_string(static_cast<int>(depth.format)) + "," + std::to_string(static_cast<int>(depth.depthWriteEnabled)) + "," + std::to_string(static_cast<int>(depth.depthCompare)) + "," + std::to_string(depth.depthBias) + "," + std::to_string(depth.depthBiasSlopeScale) + "," + std::to_string(depth.depthBiasClamp) + "," + std::to_string(job.unclippedDepth);
            for (const auto& face : {depth.stencilFront, depth.stencilBack}) key += "," + std::to_string(static_cast<int>(face.compare)) + "," + std::to_string(static_cast<int>(face.failOp)) + "," + std::to_string(static_cast<int>(face.depthFailOp)) + "," + std::to_string(static_cast<int>(face.passOp));
            key += "," + std::to_string(depth.stencilReadMask) + "," + std::to_string(depth.stencilWriteMask);
        }
        return key;
    }

    wgpu::RenderPipeline pipeline(const DrawJob& job) {
        const auto key = pipelineKey(job);
        if (const auto found = pipelines.find(key); found != pipelines.end()) return found->second;
        std::vector<wgpu::VertexAttribute> attributes(job.streams.size());
        std::vector<wgpu::VertexBufferLayout> layouts(job.streams.size());
        for (std::size_t index = 0; index < job.streams.size(); ++index) {
            attributes[index].format = job.streams[index].format;
            attributes[index].offset = 0;
            attributes[index].shaderLocation = job.streams[index].location;
            layouts[index].arrayStride = job.streams[index].stride;
            layouts[index].stepMode = wgpu::VertexStepMode::Vertex;
            layouts[index].attributeCount = 1;
            layouts[index].attributes = &attributes[index];
        }
        const std::array<wgpu::ConstantEntry, 2> constants{wgpu::ConstantEntry{nullptr, "aps5_flip_y", job.flipY ? 1.0 : 0.0}, wgpu::ConstantEntry{nullptr, "aps5_depth_half", job.depthHalf ? 1.0 : 0.0}};
        wgpu::RenderPipelineDescriptor descriptor;
        descriptor.vertex.module = module(job.vertex.source);
        descriptor.vertex.entryPoint = job.rect ? "aps5_rect" : "main";
        descriptor.vertex.constantCount = constants.size();
        descriptor.vertex.constants = constants.data();
        descriptor.vertex.bufferCount = layouts.size();
        descriptor.vertex.buffers = layouts.data();
        descriptor.primitive.topology = job.rect ? wgpu::PrimitiveTopology::TriangleList : job.topology;
        if (job.topology == wgpu::PrimitiveTopology::TriangleStrip || job.topology == wgpu::PrimitiveTopology::LineStrip) descriptor.primitive.stripIndexFormat = job.indexFormat;
        descriptor.primitive.cullMode = job.cull;
        descriptor.primitive.frontFace = job.front;
        descriptor.primitive.unclippedDepth = job.unclippedDepth;
        if (job.depth) descriptor.depthStencil = &job.depthState;
        wgpu::BlendState blend{job.colorBlend, job.alpha};
        wgpu::ColorTargetState target;
        target.format = job.target.format;
        target.blend = job.blend ? &blend : nullptr;
        target.writeMask = job.writeMask;
        wgpu::FragmentState fragment;
        if (job.pixel) {
            fragment.module = module(job.pixel->source);
            fragment.entryPoint = "main";
            fragment.targetCount = job.colorTarget ? 1 : 0;
            fragment.targets = &target;
            descriptor.fragment = &fragment;
        }
        auto result = device.CreateRenderPipeline(&descriptor);
        pipelines.emplace(key, result);
        if (pipelines.size() <= 16) std::fprintf(stderr, "[webgpu] pipeline %zu: target %ux%u%s, %zu streams, %s\n", pipelines.size(), job.target.width, job.target.height, job.depth ? (job.colorTarget ? " with depth" : " depth only") : "", job.streams.size(), job.rect ? "rect list" : "triangles");
        return result;
    }

    wgpu::BindGroup bindGroup(const wgpu::RenderPipeline& renderPipeline, std::uint32_t group, const StageJob* stage) {
        std::vector<wgpu::BindGroupEntry> entries;
        if (stage != nullptr) {
            for (const auto& resource : stage->resources) {
                wgpu::BindGroupEntry entry;
                entry.binding = resource.binding;
                switch (resource.kind) {
                case ResourceKind::Buffer: {
                    entry.buffer = buffer(resource.bytes, wgpu::BufferUsage::Storage);
                    break;
                }
                case ResourceKind::Texture:
                    entry.textureView = uploadTexture(resource.texture);
                    break;
                case ResourceKind::DepthTexture:
                    entry.textureView = depthView(resource.texture);
                    break;
                case ResourceKind::StorageTexture:
                case ResourceKind::ImageBuffer:
                    throw std::runtime_error("WebGPU: draws do not write storage images");
                case ResourceKind::Sampler:
                    entry.sampler = sampler(resource.sampler);
                    break;
                }
                entries.push_back(entry);
            }
        }
        wgpu::BindGroupDescriptor descriptor;
        descriptor.layout = renderPipeline.GetBindGroupLayout(group);
        descriptor.entryCount = entries.size();
        descriptor.entries = entries.data();
        return device.CreateBindGroup(&descriptor);
    }

    void executeDraw(const DrawJob& job) {
        auto renderPipeline = pipeline(job);
        const bool pixelResources = job.pixel && !job.pixel->resources.empty();
        std::vector<wgpu::BindGroup> groups;
        if (!job.vertex.resources.empty() || pixelResources) groups.push_back(bindGroup(renderPipeline, 0, &job.vertex));
        if (pixelResources) groups.push_back(bindGroup(renderPipeline, 1, &*job.pixel));
        std::vector<wgpu::Buffer> streams;
        for (const auto& stream : job.streams) streams.push_back(buffer(stream.bytes, wgpu::BufferUsage::Vertex));
        wgpu::Buffer indices;
        if (job.indexFormat != wgpu::IndexFormat::Undefined) indices = buffer(job.indices, wgpu::BufferUsage::Index);
        wgpu::RenderPassDescriptor passDescriptor{};
        wgpu::RenderPassColorAttachment attachment;
        RenderTarget* target = nullptr;
        if (job.colorTarget) {
            target = &renderTarget(job.target);
            attachment.view = target->view;
            attachment.loadOp = target->initialized ? wgpu::LoadOp::Load : wgpu::LoadOp::Clear;
            attachment.storeOp = wgpu::StoreOp::Store;
            attachment.clearValue = {0.0, 0.0, 0.0, 0.0};
            passDescriptor.colorAttachmentCount = 1;
            passDescriptor.colorAttachments = &attachment;
        }
        wgpu::RenderPassDepthStencilAttachment depthAttachment;
        DepthTexture* depth = nullptr;
        if (job.depth) {
            depth = &depthTexture(*job.depth);
            depth->serial = ++depthWrites;
            const bool cleared = job.depth->htileAddress != 0 && TakeDepthClear(job.depth->htileAddress, job.depth->width, job.depth->height);
            const auto load = [&](bool planeCleared) { return depth->initialized && !cleared && !planeCleared ? wgpu::LoadOp::Load : wgpu::LoadOp::Clear; };
            depthAttachment.view = depth->view;
            depthAttachment.depthLoadOp = load(job.depth->depthCleared);
            depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
            depthAttachment.depthClearValue = job.depth->clearDepth;
            if (HasStencil(job.depth->format)) {
                depthAttachment.stencilLoadOp = load(job.depth->stencilCleared);
                depthAttachment.stencilStoreOp = wgpu::StoreOp::Store;
                depthAttachment.stencilClearValue = job.depth->clearStencil;
            }
            passDescriptor.depthStencilAttachment = &depthAttachment;
        }
        auto encoder = device.CreateCommandEncoder();
        auto pass = encoder.BeginRenderPass(&passDescriptor);
        pass.SetPipeline(renderPipeline);
        for (std::uint32_t index = 0; index < groups.size(); ++index) pass.SetBindGroup(index, groups[index]);
        for (std::uint32_t index = 0; index < streams.size(); ++index) pass.SetVertexBuffer(index, streams[index]);
        pass.SetViewport(job.viewport[0], job.viewport[1], job.viewport[2], job.viewport[3], job.viewport[4], job.viewport[5]);
        pass.SetScissorRect(job.scissor[0], job.scissor[1], job.scissor[2], job.scissor[3]);
        const wgpu::Color constants{job.constants[0], job.constants[1], job.constants[2], job.constants[3]};
        pass.SetBlendConstant(&constants);
        if (job.depth && HasStencil(job.depth->format)) pass.SetStencilReference(job.stencilReference);
        if (job.rect) {
            pass.Draw(job.count / 3u * 6u, job.instances, 0, 0);
        } else if (indices) {
            pass.SetIndexBuffer(indices, job.indexFormat);
            pass.DrawIndexed(job.count, job.instances, 0, static_cast<std::int32_t>(job.firstVertex), job.firstInstance);
        } else {
            pass.Draw(job.count, job.instances, job.firstVertex, job.firstInstance);
        }
        pass.End();
        auto commands = encoder.Finish();
        gpuQueue.Submit(1, &commands);
        if (target != nullptr) target->initialized = true;
        if (depth != nullptr) depth->initialized = true;
        ++draws;
    }

    wgpu::ComputePipeline computePipeline(const std::string& source) {
        if (const auto found = computePipelines.find(source); found != computePipelines.end()) return found->second;
        wgpu::ComputePipelineDescriptor descriptor;
        descriptor.compute.module = module(source);
        descriptor.compute.entryPoint = "main";
        auto result = device.CreateComputePipeline(&descriptor);
        computePipelines.emplace(source, result);
        return result;
    }

    void executeCompute(std::shared_ptr<const ComputeJob> job, std::shared_ptr<std::promise<void>> done) {
        auto pipeline = computePipeline(job->stage.source);
        struct WriteBack {
            wgpu::Buffer storage;
            wgpu::Buffer staging;
            const Resource* resource;
        };
        std::vector<WriteBack> writeBacks;
        struct ImageBuffer {
            wgpu::Buffer buffer;
            wgpu::Texture texture;
            const Resource* resource;
        };
        std::vector<ImageBuffer> imageBuffers;
        std::vector<wgpu::BindGroupEntry> entries;
        for (const auto& resource : job->stage.resources) {
            wgpu::BindGroupEntry entry;
            entry.binding = resource.binding;
            switch (resource.kind) {
            case ResourceKind::Buffer:
                entry.buffer = buffer(resource.bytes, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);
                if (job->writable.contains(resource.binding)) {
                    wgpu::BufferDescriptor staging;
                    staging.size = entry.buffer.GetSize();
                    staging.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
                    writeBacks.push_back({entry.buffer, device.CreateBuffer(&staging), &resource});
                }
                break;
            case ResourceKind::Texture:
                entry.textureView = uploadTexture(resource.texture);
                break;
            case ResourceKind::DepthTexture:
                entry.textureView = depthView(resource.texture);
                break;
            case ResourceKind::StorageTexture: {
                const auto& texture = resource.texture;
                auto& target = renderTarget({texture.address, texture.width, texture.height, texture.format, texture.layers}, true);
                target.initialized = true;
                entry.textureView = sourceView(target.texture, target.view, 1u, target.info.layers, texture);
                break;
            }
            case ResourceKind::ImageBuffer: {
                const auto& texture = resource.texture;
                auto& target = renderTarget({texture.address, texture.width, texture.height, texture.format, texture.layers}, true);
                target.initialized = true;
                wgpu::BufferDescriptor descriptor;
                descriptor.size = static_cast<std::uint64_t>(resource.bytesPerRow) * texture.height * (texture.arrayed ? texture.viewLayers : 1u);
                descriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
                entry.buffer = device.CreateBuffer(&descriptor);
                imageBuffers.push_back({entry.buffer, target.texture, &resource});
                break;
            }
            case ResourceKind::Sampler:
                entry.sampler = sampler(resource.sampler);
                break;
            }
            entries.push_back(entry);
        }
        const auto imageCopy = [](const ImageBuffer& image) {
            const auto& texture = image.resource->texture;
            wgpu::TexelCopyTextureInfo textureCopy;
            textureCopy.texture = image.texture;
            textureCopy.origin = {0, 0, texture.baseLayer};
            wgpu::TexelCopyBufferInfo bufferCopy;
            bufferCopy.buffer = image.buffer;
            bufferCopy.layout.bytesPerRow = image.resource->bytesPerRow;
            bufferCopy.layout.rowsPerImage = texture.height;
            const wgpu::Extent3D extent{texture.width, texture.height, texture.arrayed ? texture.viewLayers : 1u};
            return std::tuple{textureCopy, bufferCopy, extent};
        };
        auto encoder = device.CreateCommandEncoder();
        for (const auto& image : imageBuffers) {
            const auto [textureCopy, bufferCopy, extent] = imageCopy(image);
            encoder.CopyTextureToBuffer(&textureCopy, &bufferCopy, &extent);
        }
        auto pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        if (!entries.empty()) {
            wgpu::BindGroupDescriptor descriptor;
            descriptor.layout = pipeline.GetBindGroupLayout(0);
            descriptor.entryCount = entries.size();
            descriptor.entries = entries.data();
            pass.SetBindGroup(0, device.CreateBindGroup(&descriptor));
        }
        pass.DispatchWorkgroups(job->groups[0], job->groups[1], job->groups[2]);
        pass.End();
        for (const auto& image : imageBuffers) {
            const auto [textureCopy, bufferCopy, extent] = imageCopy(image);
            encoder.CopyBufferToTexture(&bufferCopy, &textureCopy, &extent);
        }
        for (const auto& writeBack : writeBacks) encoder.CopyBufferToBuffer(writeBack.storage, 0, writeBack.staging, 0, writeBack.staging.GetSize());
        auto commands = encoder.Finish();
        gpuQueue.Submit(1, &commands);
        ++dispatches;
        if (writeBacks.empty()) {
            done->set_value();
            return;
        }
        auto pending = std::make_shared<std::atomic<std::size_t>>(writeBacks.size());
        for (const auto& writeBack : writeBacks) {
            const auto staging = writeBack.staging;
            const auto* resource = writeBack.resource;
            staging.MapAsync(wgpu::MapMode::Read, 0, staging.GetSize(), wgpu::CallbackMode::AllowSpontaneous, [job, done, pending, staging, resource](wgpu::MapAsyncStatus status, wgpu::StringView message) {
                if (status != wgpu::MapAsyncStatus::Success) WasmRuntime::GuestFault("WebGPU: reading back a compute buffer failed: %.*s", static_cast<int>(message.length), message.data);
                const auto* mapped = static_cast<const std::byte*>(staging.GetConstMappedRange(0, staging.GetSize()));
                WriteChanged(resource->address, resource->bytes, mapped, static_cast<std::size_t>(resource->size));
                staging.Unmap();
                if (pending->fetch_sub(1) == 1) done->set_value();
            });
        }
    }

    void executePresent(const PresentJob& job) {
        wgpu::TextureView source;
        if (const auto found = targets.find(job.buffer.address); found != targets.end() && found->second.initialized) {
            source = found->second.view;
        } else {
            TextureSource texture{job.buffer.address, false, job.buffer.width, job.buffer.height, wgpu::TextureFormat::BGRA8Unorm};
            texture.levels.push_back({0u, 0u, job.buffer.width, job.buffer.height, job.buffer.width * 4u, job.buffer.height, job.pixels});
            source = uploadTexture(texture);
        }
        if (!presentPipeline) {
            wgpu::ColorTargetState target;
            target.format = surfaceFormat;
            wgpu::FragmentState fragment;
            fragment.module = module(PresentShader);
            fragment.entryPoint = "fragmentMain";
            fragment.targetCount = 1;
            fragment.targets = &target;
            wgpu::RenderPipelineDescriptor descriptor;
            descriptor.vertex.module = module(PresentShader);
            descriptor.vertex.entryPoint = "vertexMain";
            descriptor.fragment = &fragment;
            presentPipeline = device.CreateRenderPipeline(&descriptor);
            presentSampler = sampler({wgpu::FilterMode::Linear, wgpu::FilterMode::Linear, wgpu::MipmapFilterMode::Nearest, wgpu::AddressMode::ClampToEdge, wgpu::AddressMode::ClampToEdge, wgpu::AddressMode::ClampToEdge, 0.0f, 0.0f});
        }
        std::array<wgpu::BindGroupEntry, 2> entries;
        entries[0].binding = 0;
        entries[0].textureView = source;
        entries[1].binding = 1;
        entries[1].sampler = presentSampler;
        wgpu::BindGroupDescriptor groupDescriptor;
        groupDescriptor.layout = presentPipeline.GetBindGroupLayout(0);
        groupDescriptor.entryCount = entries.size();
        groupDescriptor.entries = entries.data();
        auto group = device.CreateBindGroup(&groupDescriptor);
        wgpu::SurfaceTexture surfaceTexture;
        surface.GetCurrentTexture(&surfaceTexture);
        Require(surfaceTexture.texture != nullptr, "the canvas has no current texture");
        wgpu::RenderPassColorAttachment attachment;
        attachment.view = surfaceTexture.texture.CreateView();
        attachment.loadOp = wgpu::LoadOp::Clear;
        attachment.storeOp = wgpu::StoreOp::Store;
        attachment.clearValue = {0.0, 0.0, 0.0, 1.0};
        wgpu::RenderPassDescriptor passDescriptor;
        passDescriptor.colorAttachmentCount = 1;
        passDescriptor.colorAttachments = &attachment;
        auto encoder = device.CreateCommandEncoder();
        auto pass = encoder.BeginRenderPass(&passDescriptor);
        pass.SetPipeline(presentPipeline);
        pass.SetBindGroup(0, group);
        pass.Draw(3);
        pass.End();
        auto commands = encoder.Finish();
        gpuQueue.Submit(1, &commands);
        ++presents;
        if ((presents & (presents - 1)) == 0) std::fprintf(stderr, "[webgpu] present %llu: buffer 0x%llx %ux%u, %llu draws, %llu dispatches\n", static_cast<unsigned long long>(presents), static_cast<unsigned long long>(job.buffer.address), job.buffer.width, job.buffer.height, static_cast<unsigned long long>(draws), static_cast<unsigned long long>(dispatches));
    }

    pthread_t thread{};
    em_proxying_queue* queue = nullptr;
    std::mutex mutex;
    std::condition_variable changed;
    int state = 0;
    std::string failure;
    std::map<std::uint64_t, TargetInfo> targetShapes;
    std::set<std::uint64_t> depthAddresses;
    std::map<std::uint64_t, std::tuple<std::uint64_t, std::uint32_t, std::uint32_t, wgpu::TextureFormat, std::uint32_t>> uploaded;
    std::uint64_t presentsPosted = 0;
    std::uint64_t presentsDone = 0;

    wgpu::Instance instance;
    wgpu::Surface surface;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue gpuQueue;
    wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::BGRA8Unorm;
    std::unordered_map<std::string, wgpu::ShaderModule> modules;
    std::unordered_map<std::string, wgpu::RenderPipeline> pipelines;
    std::unordered_map<std::string, wgpu::ComputePipeline> computePipelines;
    std::map<std::uint64_t, RenderTarget> targets;
    std::map<std::uint64_t, DepthTexture> depthTextures;
    struct DepthCopy {
        wgpu::Texture texture;
        wgpu::TextureView view;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };
    std::map<std::uint64_t, DepthCopy> depthCopies;
    std::map<std::uint64_t, DepthArray> depthArrays;
    std::uint64_t depthWrites = 0;
    wgpu::ComputePipeline depthCopyPipeline;
    std::set<wgpu::FeatureName> features;
    wgpu::Limits limits;
    std::vector<std::pair<std::uint64_t, std::size_t>> depthClears;
    std::map<std::uint64_t, Upload> uploads;
    std::map<SamplerKey, wgpu::Sampler> samplers;
    wgpu::RenderPipeline presentPipeline;
    wgpu::Sampler presentSampler;
    std::uint64_t draws = 0;
    std::uint64_t dispatches = 0;
    std::uint64_t presents = 0;
};

std::uint32_t MemoryOffset(const ShaderRecompiler::RecompileResult& result, std::uint32_t element) {
    const auto data = std::ranges::find_if(result.bindings, [](const auto& binding) { return binding.role == DescriptorRole::ShaderData; });
    if (data == result.bindings.end()) return 0;
    const auto index = result.memoryOffsetDword + element / 4u;
    Require(index < data->guestDescriptor.size(), "the shader data has no memory offset for buffer element " + std::to_string(element));
    return (data->guestDescriptor[index] >> ((element % 4u) * 8u)) & 0xffu;
}

TextureSource DescribeTextureView(const AgcDriver::Graphics::GuestTextureResource& texture) {
    namespace Graphics = AgcDriver::Graphics;
    Require(texture.dimension == Graphics::TextureDimension::k2D || texture.dimension == Graphics::TextureDimension::k2DArray || texture.dimension == Graphics::TextureDimension::kCube || texture.dimension == Graphics::TextureDimension::k3D, "only 2D, 2D array, cube and 3D textures are implemented");
    TextureSource source;
    source.address = texture.baseAddress;
    source.width = texture.width;
    source.height = texture.height;
    source.format = TextureFormat(Graphics::ResolveTextureFormat(texture.format));
    source.arrayed = texture.dimension == Graphics::TextureDimension::k2DArray || texture.dimension == Graphics::TextureDimension::kCube;
    source.volume = texture.dimension == Graphics::TextureDimension::k3D;
    source.layers = source.arrayed || source.volume ? texture.depthOrLastArray + 1u : 1u;
    source.baseLayer = source.arrayed ? texture.baseArray : 0u;
    source.viewLayers = source.arrayed ? texture.depthOrLastArray - texture.baseArray + 1u : 1u;
    return source;
}

void WidenNorm16(TextureLevel& level, std::uint32_t components, bool signedValues) {
    const auto values = static_cast<std::size_t>(level.width) * components;
    std::vector<std::byte> widened(values * 4u * level.rows);
    for (std::uint32_t row = 0; row < level.rows; ++row) {
        for (std::size_t index = 0; index < values; ++index) {
            std::uint16_t raw = 0;
            std::memcpy(&raw, level.bytes.data() + static_cast<std::size_t>(row) * level.bytesPerRow + index * 2u, 2u);
            const float value = signedValues ? std::max(static_cast<float>(static_cast<std::int16_t>(raw)) / 32767.0f, -1.0f) : static_cast<float>(raw) / 65535.0f;
            std::memcpy(widened.data() + (static_cast<std::size_t>(row) * values + index) * 4u, &value, 4u);
        }
    }
    level.bytes = std::move(widened);
    level.bytesPerRow = static_cast<std::uint32_t>(values * 4u);
}

TextureSource DescribeTexture(std::span<const std::uint32_t> words, Device& gpu) {
    namespace Graphics = AgcDriver::Graphics;
    Require(!Graphics::IsNullTextureResource(words), "null textures are not implemented");
    const auto texture = Graphics::DecodeTextureResource(words);
    TextureSource source = DescribeTextureView(texture);
    Require(!Float32Format(source.format) || gpu.Supports(wgpu::FeatureName::Float32Filterable), "the GPU cannot filter 32-bit float textures");
    if (gpu.HoldsTarget(source)) return source;
    if (gpu.IsDepthTarget(texture.baseAddress)) {
        Require(source.format == wgpu::TextureFormat::R32Float || source.format == wgpu::TextureFormat::R16Unorm, "a depth target sampled as format " + std::to_string(static_cast<int>(source.format)) + " is not implemented");
        Require(!source.arrayed && !source.volume && texture.baseLevel == 0u && texture.lastLevel == 0u, "a layered or mipmapped view of a depth target is not implemented");
        Require(gpu.Supports(wgpu::FeatureName::Float32Filterable), "the GPU cannot filter the 32-bit float copy of a depth target");
        source.depthPlane = true;
        source.format = wgpu::TextureFormat::R32Float;
        return source;
    }
    const bool compressed = Graphics::IsBlockCompressed(texture.format);
    Require(!compressed || gpu.Supports(wgpu::FeatureName::TextureCompressionBC), "the GPU cannot sample BC-compressed textures");
    Require(!compressed || (texture.width % Graphics::BlockWidth(texture.format) == 0 && texture.height % Graphics::BlockHeight(texture.format) == 0), "compressed textures whose size is not a whole number of blocks are not implemented");
    const auto geometry = Graphics::DescribeSurface(texture);
    Require(geometry.layers == source.layers, "the texture's surface has another layer count than its view");
    const auto stored = source.format;
    const auto widenedComponents = Norm16Components(stored);
    if (widenedComponents != 0) {
        Require(gpu.Supports(wgpu::FeatureName::Float32Filterable), "the GPU cannot filter the 32-bit float copy of a 16-bit normalized texture");
        source.format = widenedComponents == 1u ? wgpu::TextureFormat::R32Float : widenedComponents == 2u ? wgpu::TextureFormat::RG32Float : wgpu::TextureFormat::RGBA32Float;
    }
    source.levelCount = static_cast<std::uint32_t>(geometry.mips.size());
    Require(texture.baseLevel <= texture.lastLevel && texture.lastLevel < source.levelCount, "the texture view selects levels outside its surface");
    source.baseLevel = texture.baseLevel;
    source.viewLevels = texture.lastLevel - texture.baseLevel + 1u;
    source.reuse = gpu.Unchanged(source, Hash(texture.baseAddress, geometry.guestBytes));
    if (source.reuse) return source;
    const auto bytesPerElement = Graphics::BytesPerElement(texture.format);
    for (std::uint32_t layer = 0; layer < geometry.layers; ++layer) {
        for (std::uint32_t index = 0; index < geometry.mips.size(); ++index) {
            if (!geometry.HasLayer(index, layer)) continue;
            const auto& mip = geometry.mips[index];
            TextureLevel level{index, layer, mip.width * Graphics::BlockWidth(texture.format), mip.height * Graphics::BlockHeight(texture.format), mip.pitchBytes, mip.height, std::vector<std::byte>(mip.linearSize)};
            const std::span tiled(reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(texture.baseAddress + geometry.GuestLayerOffset(layer) + mip.tiledOffset)), static_cast<std::size_t>(mip.tiledSize));
            Graphics::DetileMip(texture.tileMode, bytesPerElement, mip, texture.pipeBankXor, layer, tiled, level.bytes, geometry.thick);
            if (widenedComponents != 0) WidenNorm16(level, widenedComponents, stored == wgpu::TextureFormat::R16Snorm || stored == wgpu::TextureFormat::RG16Snorm || stored == wgpu::TextureFormat::RGBA16Snorm);
            source.levels.push_back(std::move(level));
        }
    }
    return source;
}

TextureSource DescribeDepthTexture(std::span<const std::uint32_t> words, Device& gpu) {
    namespace Graphics = AgcDriver::Graphics;
    Require(!Graphics::IsNullTextureResource(words), "null depth comparison textures are not implemented");
    const auto texture = Graphics::DecodeTextureResource(words);
    Require(texture.baseLevel == 0u && texture.lastLevel == 0u, "depth comparison against a mipmapped depth texture is not implemented");
    TextureSource source = DescribeTextureView(texture);
    if (!source.arrayed) {
        Require(gpu.IsDepthTarget(texture.baseAddress), "depth comparison against a texture that is not a depth target is not implemented");
        return source;
    }
    Require(texture.dimension != Graphics::TextureDimension::kCube || (texture.baseArray == 0u && source.layers == 6u), "depth comparison against a cube array or a cube view past its first face is not implemented");
    const auto geometry = Graphics::DescribeSurface(texture);
    for (std::uint32_t layer = 0; layer < source.layers; ++layer) {
        const auto address = texture.baseAddress + geometry.GuestLayerOffset(layer);
        Require(gpu.IsDepthTarget(address), "depth comparison against a layer that is not a depth target is not implemented");
        source.depthSlices.push_back(address);
    }
    return source;
}

TextureSource DescribeStorageTexture(std::span<const std::uint32_t> words, Device& gpu) {
    namespace Graphics = AgcDriver::Graphics;
    Require(!Graphics::IsNullTextureResource(words), "null storage images are not implemented");
    const auto texture = Graphics::DecodeTextureResource(words);
    Require(texture.mipCount <= 1u && texture.baseLevel == 0u, "mipmapped storage images are not implemented");
    TextureSource source = DescribeTextureView(texture);
    Require(!source.volume, "3D storage images are not implemented");
    Require(source.format != wgpu::TextureFormat::BGRA8Unorm || gpu.Supports(wgpu::FeatureName::BGRA8UnormStorage), "the GPU cannot write BGRA8 storage images");
    Require(!TierOneStorageFormat(source.format) || gpu.Supports(wgpu::FeatureName::TextureFormatsTier1), "the GPU cannot write storage images of format " + std::to_string(static_cast<int>(source.format)));
    gpu.NoteTarget({texture.baseAddress, source.width, source.height, source.format, source.layers});
    return source;
}

SamplerKey DescribeSampler(std::span<const std::uint32_t> words, bool comparison) {
    const auto sampler = AgcDriver::Graphics::DecodeSamplerResource(words);
    Require(!sampler.unnormalizedCoordinates, "unnormalized samplers are not implemented");
    SamplerKey key;
    if (comparison) key.compare = CompareFunction(sampler.compareOp);
    key.mag = sampler.magFilter == VK_FILTER_LINEAR ? wgpu::FilterMode::Linear : wgpu::FilterMode::Nearest;
    key.min = sampler.minFilter == VK_FILTER_LINEAR ? wgpu::FilterMode::Linear : wgpu::FilterMode::Nearest;
    key.mip = sampler.mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR ? wgpu::MipmapFilterMode::Linear : wgpu::MipmapFilterMode::Nearest;
    key.u = AddressMode(sampler.addressModeU);
    key.v = AddressMode(sampler.addressModeV);
    key.w = AddressMode(sampler.addressModeW);
    key.minLod = std::max(sampler.minLod, 0.0f);
    key.maxLod = std::max(sampler.maxLod, key.minLod);
    return key;
}

StageJob DescribeStage(const ShaderRecompiler::RecompileResult& result, std::string source, Device& gpu) {
    Require(source.find("enable subgroups;") == std::string::npos || gpu.Supports(wgpu::FeatureName::Subgroups), "the GPU has no subgroup operations, which this program's quad permutations need");
    StageJob stage;
    const auto declared = DeclaredBindings(source);
    stage.source = std::move(source);
    std::uint32_t number = 0;
    for (const auto& binding : result.bindings) {
        for (std::uint32_t element = 0; element < binding.count; ++element, ++number) {
            if (!declared.contains(number)) continue;
            Resource resource;
            resource.binding = number;
            const auto words = std::span(binding.guestDescriptor);
            switch (binding.role) {
            case DescriptorRole::ShaderData:
            case DescriptorRole::FlattenedSrt:
                resource.bytes.resize(words.size() * sizeof(std::uint32_t));
                std::memcpy(resource.bytes.data(), words.data(), resource.bytes.size());
                break;
            case DescriptorRole::GuestBuffers: {
                Require(words.size() >= (element + 1u) * 4u, "a guest buffer element has no V#");
                const auto v = words.subspan(element * 4u, 4);
                const auto base = static_cast<std::uint64_t>(v[0]) | (static_cast<std::uint64_t>(v[1] & 0xffffu) << 32u);
                const auto stride = (v[1] >> 16u) & 0x3fffu;
                const std::uint64_t size = stride == 0 ? v[2] : static_cast<std::uint64_t>(stride) * v[2];
                const auto offset = MemoryOffset(result, element);
                resource.address = base - offset;
                resource.size = std::min<std::uint64_t>(size + offset, BufferLimit);
                resource.truncated = resource.size != size + offset;
                resource.bytes = ReadGuest(resource.address, resource.size);
                break;
            }
            case DescriptorRole::GuestImages:
                Require(words.size() >= (element + 1u) * 8u, "a guest image element has no T#");
                if (BindingDeclaration(stage.source, number).find("texture_storage") != std::string::npos) {
                    resource.kind = ResourceKind::StorageTexture;
                    resource.texture = DescribeStorageTexture(words.subspan(element * 8u, 8), gpu);
                    SpecializeStorageFormat(stage.source, number, StorageFormatName(resource.texture.format));
                    break;
                }
                if (BindingDeclaration(stage.source, number).find("texture_depth") != std::string::npos) {
                    resource.kind = ResourceKind::DepthTexture;
                    resource.texture = DescribeDepthTexture(words.subspan(element * 8u, 8), gpu);
                    break;
                }
                resource.kind = ResourceKind::Texture;
                resource.texture = DescribeTexture(words.subspan(element * 8u, 8), gpu);
                break;
            case DescriptorRole::GuestSamplers:
                Require(words.size() >= (element + 1u) * 4u, "a guest sampler element has no S#");
                resource.kind = ResourceKind::Sampler;
                resource.sampler = DescribeSampler(words.subspan(element * 4u, 4), BindingDeclaration(stage.source, number).find("sampler_comparison") != std::string::npos);
                break;
            default:
                throw std::runtime_error("WebGPU: binding role " + std::to_string(static_cast<int>(binding.role)) + " is not implemented");
            }
            stage.resources.push_back(std::move(resource));
        }
    }
    return stage;
}

void BufferExcessStorageImages(StageJob& stage, const Device& gpu) {
    std::vector<Resource*> images;
    std::size_t buffers = 0;
    for (auto& resource : stage.resources) {
        if (resource.kind == ResourceKind::StorageTexture) images.push_back(&resource);
        if (resource.kind == ResourceKind::Buffer) ++buffers;
    }
    const auto limit = gpu.StorageTextureLimit();
    if (images.size() <= limit) return;
    const auto layers = [](const TextureSource& texture) { return texture.arrayed ? texture.viewLayers : 1u; };
    const auto texelBytes = [](const TextureSource& texture) { return ShaderRecompiler::WgslStorageTexelBytes(StorageFormatName(texture.format)); };
    const auto bytes = [&](const Resource* resource) { return static_cast<std::uint64_t>(resource->texture.width) * resource->texture.height * layers(resource->texture) * texelBytes(resource->texture); };
    std::stable_sort(images.begin(), images.end(), [&](const Resource* left, const Resource* right) { return bytes(left) < bytes(right); });
    const auto moved = images.size() - limit;
    Require(buffers + moved <= gpu.StorageBufferLimit(), "a program writes " + std::to_string(images.size()) + " storage images and reads " + std::to_string(buffers) + " buffers, more than the GPU's " + std::to_string(limit) + " storage textures and " + std::to_string(gpu.StorageBufferLimit()) + " storage buffers hold");
    for (std::size_t index = 0; index < moved; ++index) {
        auto& resource = *images[index];
        const auto& texture = resource.texture;
        resource.bytesPerRow = (texture.width * texelBytes(texture) + 255u) & ~255u;
        Require(static_cast<std::uint64_t>(resource.bytesPerRow) * texture.height * layers(texture) <= gpu.StorageBufferBindingLimit(), "a storage image kept in a buffer is larger than the GPU's storage buffer binding size");
        stage.source = ShaderRecompiler::BufferWgslStorageImage(std::move(stage.source), resource.binding, {texture.width, texture.height, layers(texture), resource.bytesPerRow});
        resource.kind = ResourceKind::ImageBuffer;
    }
}

VkImageAspectFlags TakeHtileClear(const AgcDriver::Graphics::DepthTarget& depth) {
    if (depth.htileAddress == 0) return 0;
    const auto count = static_cast<std::size_t>(AgcDriver::Graphics::HtileMetadataBytes(depth.extent) / 4u);
    auto* words = reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(depth.htileAddress));
    AgcDriver::GuestMemory::CheckRange(words, count * 4u, 4u, true);
    VkImageAspectFlags cleared = VK_IMAGE_ASPECT_DEPTH_BIT | (depth.htileStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0u);
    bool uniform = true;
    for (std::size_t index = 0; index < count && cleared != 0; ++index) {
        cleared &= AgcDriver::Graphics::HtileFillClears(words[index], depth.htileStencil);
        uniform = uniform && words[index] == words[0];
    }
    if (cleared == 0 || (depth.firstSlice != 0 && !uniform)) return 0;
    for (std::size_t index = 0; index < count; ++index) words[index] = AgcDriver::Graphics::HtileExpandedWord(words[index], depth.htileStencil, cleared);
    return cleared;
}

VertexStream VertexStreamFor(std::uint32_t location, VkFormat format, std::uint32_t stride, std::uint32_t count, std::vector<std::byte> bytes) {
    if (const auto direct = DirectVertexFormat(format)) return {location, *direct, stride, std::move(bytes)};
    wgpu::VertexFormat converted{};
    auto data = ConvertVertexStream(format, bytes, stride, stride == 0 ? 1u : count, converted);
    return {location, converted, stride == 0 ? 0u : 16u, std::move(data)};
}

std::shared_ptr<DrawJob> BuildDrawJob(std::span<const std::uint32_t> packet, const AgcDriver::QueueState& queue, Device& gpu) {
    auto requests = BuildDrawRequests(DecodeDraw(packet, queue));
    const auto& draw = requests->stages;
    const auto& state = draw.state;
    Require(state.colors.size() <= 1, "draws with more than one color target are not implemented");
    auto& compiled = requests->requests;
    Require(compiled.size() > 1 || !state.hasColorTarget, "color draws without a pixel program are not implemented");
    const auto vertex = ShaderRecompiler::Recompile(compiled.front());
    std::optional<ShaderRecompiler::RecompileResult> pixel;
    if (compiled.size() > 1) {
        auto& request = compiled[1];
        request.context.pixel = AgcDriver::Graphics::DefaultUnexportedInputs(*request.context.pixel, vertex.parameterExports);
        pixel = ShaderRecompiler::Recompile(request);
    }
    if (!state.hasColorTarget && !state.depth) return nullptr;
    auto job = std::make_shared<DrawJob>();
    job->colorTarget = state.hasColorTarget;
    if (job->colorTarget) {
        const auto& color = state.color;
        job->target = {color.address, color.extent.width, color.extent.height, TextureFormat(color.format)};
        gpu.NoteTarget(job->target);
    }
    if (state.depth) {
        const auto& depth = *state.depth;
        Require(depth.samples == 1, "multisampled depth targets are not implemented");
        Require(depth.address != 0, "stencil-only depth targets are not implemented");
        const auto htileCleared = TakeHtileClear(depth);
        job->depth = DepthInfo{depth.address, depth.htileAddress, depth.extent.width, depth.extent.height, DepthFormat(depth.format), depth.clearDepth, depth.clearStencil, (htileCleared & VK_IMAGE_ASPECT_DEPTH_BIT) != 0, (htileCleared & VK_IMAGE_ASPECT_STENCIL_BIT) != 0};
        gpu.NoteDepthTarget(depth.address);
        Require(job->depth->format != wgpu::TextureFormat::Depth32FloatStencil8 || gpu.Supports(wgpu::FeatureName::Depth32FloatStencil8), "the GPU has no 32-bit float depth with stencil");
        Require(!job->colorTarget || (job->target.width == depth.extent.width && job->target.height == depth.extent.height), "color and depth targets of different sizes are not implemented");
        Require(!state.depthBoundsTest, "depth bounds tests are not available in WebGPU");
        Require(!state.depthBiasPerFace, "per-face depth bias is not available in WebGPU");
        Require(!state.depthClamp || gpu.Supports(wgpu::FeatureName::DepthClipControl), "the GPU cannot clamp depth instead of clipping it");
        auto& depthState = job->depthState;
        depthState.format = job->depth->format;
        depthState.depthWriteEnabled = state.depthWrite ? wgpu::OptionalBool::True : wgpu::OptionalBool::False;
        depthState.depthCompare = state.depthTest ? CompareFunction(state.depthCompare) : wgpu::CompareFunction::Always;
        if (state.depthBias) {
            const auto unitScale = depth.format == VK_FORMAT_D16_UNORM_S8_UINT ? 256.0f : 1.0f;
            depthState.depthBias = static_cast<std::int32_t>(std::lround(state.depthBiasConstant * unitScale));
            depthState.depthBiasSlopeScale = state.depthBiasSlope;
            depthState.depthBiasClamp = state.depthBiasClamp;
        }
        const wgpu::StencilFaceState keep{wgpu::CompareFunction::Always, wgpu::StencilOperation::Keep, wgpu::StencilOperation::Keep, wgpu::StencilOperation::Keep};
        depthState.stencilFront = keep;
        depthState.stencilBack = keep;
        if (state.stencilTest) {
            const auto& front = state.stencilFront;
            const auto& back = state.stencilBack;
            Require(HasStencil(job->depth->format), "a stencil test needs a stencil plane");
            Require(front.compareMask == back.compareMask && front.writeMask == back.writeMask && front.reference == back.reference, "different front and back stencil masks or references are not available in WebGPU");
            depthState.stencilFront = StencilFace(front);
            depthState.stencilBack = StencilFace(back);
            depthState.stencilReadMask = front.compareMask & 0xffu;
            depthState.stencilWriteMask = front.writeMask & 0xffu;
            job->stencilReference = front.reference & 0xffu;
        }
        job->unclippedDepth = state.depthClamp;
    } else {
        Require(!state.depthTest && !state.depthWrite && !state.stencilTest, "depth or stencil tests without a depth target");
    }
    if (pixel) job->pixel = DescribeStage(*pixel, pixel->wgsl, gpu);
    job->vertex = DescribeStage(vertex, pixel ? MatchVertexOutputs(vertex.wgsl, pixel->wgsl) : vertex.wgsl, gpu);

    const auto& parameters = draw.draw;
    const auto& front = draw.programs.front();
    const auto userWord = [&](std::int32_t sgpr) {
        Require(sgpr >= static_cast<std::int32_t>(front.firstUserSgpr) && static_cast<std::uint32_t>(sgpr) - front.firstUserSgpr < front.userData.size(), "a draw offset SGPR lies outside the user data");
        return front.userData[static_cast<std::uint32_t>(sgpr) - front.firstUserSgpr];
    };
    job->count = parameters.indexCount;
    job->instances = std::max(parameters.instanceCount, 1u);
    job->firstVertex = parameters.firstVertex;
    if (vertex.vertexOffsetSgpr >= 0) {
        const auto offset = userWord(vertex.vertexOffsetSgpr);
        Require(offset <= std::numeric_limits<std::uint32_t>::max() - job->firstVertex, "draw vertex offset overflow");
        job->firstVertex += offset;
    }
    job->firstInstance = vertex.instanceOffsetSgpr >= 0 ? userWord(vertex.instanceOffsetSgpr) : parameters.firstInstance;
    std::uint64_t maxIndex = static_cast<std::uint64_t>(job->firstVertex) + std::max(parameters.indexCount, 1u) - 1u;
    if (parameters.indexed) {
        Require(parameters.indexSize == 2 || parameters.indexSize == 4, "unsupported index size");
        job->indexFormat = parameters.indexSize == 2 ? wgpu::IndexFormat::Uint16 : wgpu::IndexFormat::Uint32;
        job->indices = ReadGuest(parameters.indexAddress, static_cast<std::uint64_t>(parameters.indexCount) * parameters.indexSize);
        const std::uint32_t restart = parameters.indexSize == 2 ? 0xffffu : 0xffffffffu;
        std::uint32_t largest = 0;
        for (std::uint32_t index = 0; index < parameters.indexCount; ++index) {
            std::uint32_t value = 0;
            std::memcpy(&value, job->indices.data() + index * parameters.indexSize, parameters.indexSize);
            if (state.primitiveRestart && value == restart) continue;
            largest = std::max(largest, value);
        }
        maxIndex = static_cast<std::uint64_t>(largest) + job->firstVertex;
    }
    for (const auto& attribute : vertex.vertexAttributes) {
        Require(attribute.fetchIndex == 0, "instanced vertex fetch is not implemented");
        const auto format = AgcDriver::Graphics::DecodeVertexFormat(attribute);
        if (AgcDriver::Graphics::NullVertexDescriptor(attribute) || AgcDriver::Graphics::VertexFetchOutOfRange(attribute)) {
            job->streams.push_back(VertexStreamFor(attribute.location, format.format, 0u, 1u, std::vector<std::byte>((format.bytes + 3u) & ~3u)));
            continue;
        }
        const auto& fields = attribute.resource.fields;
        const auto base = static_cast<std::uint64_t>(fields[0]) | (static_cast<std::uint64_t>(fields[1] & 0xffffu) << 32u);
        const auto stride = (fields[1] >> 16u) & 0x3fffu;
        if (stride == 0) {
            Require(format.bytes <= AgcDriver::Graphics::ZeroStrideAvailableBytes(attribute), "a vertex fetch without a stride reads past its descriptor");
            job->streams.push_back(VertexStreamFor(attribute.location, format.format, 0u, 1u, ReadGuest(base, format.bytes)));
            continue;
        }
        Require(stride % 4u == 0, "vertex stride " + std::to_string(stride) + " is not supported");
        const auto bytes = static_cast<std::uint64_t>(stride) * maxIndex + format.bytes;
        Require(bytes <= BufferLimit, "a draw reads more than " + std::to_string(BufferLimit >> 20u) + " MiB of one vertex stream");
        job->streams.push_back(VertexStreamFor(attribute.location, format.format, stride, static_cast<std::uint32_t>(maxIndex + 1u), ReadGuest(base, bytes)));
    }

    switch (state.topology) {
    case VK_PRIMITIVE_TOPOLOGY_POINT_LIST: job->topology = wgpu::PrimitiveTopology::PointList; break;
    case VK_PRIMITIVE_TOPOLOGY_LINE_LIST: job->topology = wgpu::PrimitiveTopology::LineList; break;
    case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: job->topology = wgpu::PrimitiveTopology::LineStrip; break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: job->topology = wgpu::PrimitiveTopology::TriangleList; break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: job->topology = wgpu::PrimitiveTopology::TriangleStrip; break;
    default:
        Require(state.rectList, "primitive topology " + std::to_string(static_cast<int>(state.topology)) + " is not supported");
    }
    if (state.rectList) {
        Require(!parameters.indexed && job->streams.empty(), "indexed or vertex-buffer rect lists are not implemented");
        Require(job->vertex.source.find("fn aps5_rect(") != std::string::npos, "the vertex program has no rect list entry point");
        job->rect = true;
    }

    auto viewport = state.viewport;
    job->flipY = viewport.height > 0.0f;
    if (viewport.height < 0.0f) {
        viewport.y += viewport.height;
        viewport.height = -viewport.height;
    }
    const auto renderWidth = job->colorTarget ? job->target.width : job->depth->width;
    const auto renderHeight = job->colorTarget ? job->target.height : job->depth->height;
    const auto width = static_cast<float>(renderWidth);
    const auto height = static_cast<float>(renderHeight);
    Require(viewport.minDepth >= 0.0f && viewport.minDepth <= viewport.maxDepth && viewport.maxDepth <= 1.0f, "viewport depth ranges outside [0, 1] or inverted are not available in WebGPU");
    const auto left = std::clamp(viewport.x, 0.0f, width);
    const auto top = std::clamp(viewport.y, 0.0f, height);
    job->viewport = {left, top, std::clamp(viewport.x + viewport.width, left, width) - left, std::clamp(viewport.y + viewport.height, top, height) - top, viewport.minDepth, viewport.maxDepth};
    job->depthHalf = state.negativeOneToOne;
    const auto scissorLeft = std::min(static_cast<std::uint32_t>(std::max(state.scissor.offset.x, 0)), renderWidth);
    const auto scissorTop = std::min(static_cast<std::uint32_t>(std::max(state.scissor.offset.y, 0)), renderHeight);
    job->scissor = {scissorLeft, scissorTop, std::min(state.scissor.extent.width, renderWidth - scissorLeft), std::min(state.scissor.extent.height, renderHeight - scissorTop)};

    job->cull = state.cullMode == VK_CULL_MODE_NONE ? wgpu::CullMode::None : state.cullMode == VK_CULL_MODE_FRONT_BIT ? wgpu::CullMode::Front : state.cullMode == VK_CULL_MODE_BACK_BIT ? wgpu::CullMode::Back : wgpu::CullMode::None;
    Require(state.cullMode != VK_CULL_MODE_FRONT_AND_BACK, "culling both faces is not implemented");
    const bool clockwise = state.frontFace == VK_FRONT_FACE_CLOCKWISE;
    job->front = clockwise != job->flipY ? wgpu::FrontFace::CW : wgpu::FrontFace::CCW;

    const auto& blend = state.blend;
    job->blend = blend.blendEnable != VK_FALSE;
    job->colorBlend = {BlendOperation(blend.colorBlendOp), BlendFactor(blend.srcColorBlendFactor), BlendFactor(blend.dstColorBlendFactor)};
    job->alpha = {BlendOperation(blend.alphaBlendOp), BlendFactor(blend.srcAlphaBlendFactor), BlendFactor(blend.dstAlphaBlendFactor)};
    job->writeMask = static_cast<wgpu::ColorWriteMask>(blend.colorWriteMask & 0xfu);
    job->constants = {state.blendConstants[0], state.blendConstants[1], state.blendConstants[2], state.blendConstants[3]};
    const auto constantAlpha = [](VkBlendFactor factor) { return factor == VK_BLEND_FACTOR_CONSTANT_ALPHA || factor == VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA; };
    const auto constantColor = [](VkBlendFactor factor) { return factor == VK_BLEND_FACTOR_CONSTANT_COLOR || factor == VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR; };
    if (job->blend && (constantAlpha(blend.srcColorBlendFactor) || constantAlpha(blend.dstColorBlendFactor))) {
        const bool grey = job->constants[0] == job->constants[3] && job->constants[1] == job->constants[3] && job->constants[2] == job->constants[3];
        Require(grey || !(constantColor(blend.srcColorBlendFactor) || constantColor(blend.dstColorBlendFactor)), "blending color with both the constant color and the constant alpha is not available in WebGPU");
        job->constants = {job->constants[3], job->constants[3], job->constants[3], job->constants[3]};
    }
    return job;
}

bool TraceGpu() {
    static const bool enabled = std::getenv("APS5_TRACE_GPU") != nullptr;
    return enabled;
}

std::shared_ptr<ComputeJob> BuildComputeJob(std::span<const std::uint32_t> packet, const AgcDriver::QueueState& queue, Device& gpu) {
    const auto dispatch = BuildDispatchRequest(packet, queue);
    if (dispatch->groups[0] == 0 || dispatch->groups[1] == 0 || dispatch->groups[2] == 0) return nullptr;
    const auto result = ShaderRecompiler::Recompile(dispatch->request);
    auto job = std::make_shared<ComputeJob>();
    job->groups = dispatch->groups;
    job->writable = DeclaredBindings(result.wgsl, true);
    const auto& compute = dispatch->compute;
    job->htileClear = AgcDriver::Graphics::HtileDepthClearAddress(std::span(dispatch->shader->code).subspan(dispatch->codeOffset), dispatch->userData, compute.numThreads);
    job->htileBytes = static_cast<std::size_t>(dispatch->groups[0]) * compute.numThreads[0] * 16u;
    job->stage = DescribeStage(result, result.wgsl, gpu);
    BufferExcessStorageImages(job->stage, gpu);
    for (const auto& resource : job->stage.resources) Require(!resource.truncated || !job->writable.contains(resource.binding), "a compute program writes a buffer larger than " + std::to_string(BufferLimit >> 20u) + " MiB");
    if (TraceGpu()) {
        std::string buffers;
        for (const auto& resource : job->stage.resources) {
            if (resource.kind == ResourceKind::Buffer) buffers += " " + std::to_string(resource.bytes.size()) + (job->writable.contains(resource.binding) ? "w" : "");
            if (resource.kind == ResourceKind::ImageBuffer) buffers += " image " + std::to_string(resource.texture.width) + "x" + std::to_string(resource.texture.height);
        }
        std::fprintf(stderr, "[webgpu] dispatch 0x%llx: %u x %u x %u groups of %u x %u x %u, buffers%s%s\n", static_cast<unsigned long long>(dispatch->request.shader.codeAddress), job->groups[0], job->groups[1], job->groups[2], compute.numThreads[0], compute.numThreads[1], compute.numThreads[2], buffers.c_str(), job->htileClear != 0 ? ", HTILE clear" : "");
    }
    return job;
}

class WebGpuBackend final : public IBackend {
public:
    void RegisterShader(const Shader*) override {}

    void Draw(std::uint32_t, std::span<const std::uint32_t> packet, AgcDriver::QueueState& state) override {
        if (ShaderCaptureEnabled()) CaptureDraw(packet, state);
        static const bool null = std::getenv("APS5_WEBGPU_NULL") != nullptr;
        if (null) return;
        auto& gpu = Device::Get();
        if (auto job = BuildDrawJob(packet, state, gpu)) gpu.Draw(std::move(job));
    }

    void Dispatch(std::uint32_t, std::span<const std::uint32_t> packet, AgcDriver::QueueState& state) override {
        if (ShaderCaptureEnabled()) CaptureDispatch(packet, state);
        static const bool null = std::getenv("APS5_WEBGPU_NULL") != nullptr;
        if (null) return;
        auto& gpu = Device::Get();
        auto job = BuildComputeJob(packet, state, gpu);
        if (!job) return;
        if (job->htileClear != 0) gpu.NoteDepthClear(job->htileClear, job->htileBytes);
        gpu.Compute(std::move(job));
    }

    void AfterWork(std::function<void()> completion) override {
        static const bool null = std::getenv("APS5_WEBGPU_NULL") != nullptr;
        if (null) return completion();
        Device::Get().AfterWork(std::move(completion));
    }

    void PresentBuffer(const AgcDriver::DisplayBuffer& buffer) override {
        static const bool null = std::getenv("APS5_WEBGPU_NULL") != nullptr;
        static std::atomic<std::uint64_t> count{0};
        if (null) {
            const auto presented = ++count;
            if ((presented & (presented - 1)) == 0) std::fprintf(stderr, "[webgpu] null present %llu\n", static_cast<unsigned long long>(presented));
            return;
        }
        auto& gpu = Device::Get();
        auto job = std::make_shared<PresentJob>();
        job->buffer = buffer;
        if (!gpu.IsTarget(buffer.address)) job->pixels = AgcDriver::ReadDisplayBuffer(buffer);
        gpu.Present(std::move(job));
    }

    void PresentClear(bool) override {}
};

}  // namespace

IBackend& Backend() {
    static WebGpuBackend backend;
    return backend;
}

}  // namespace WasmGpu

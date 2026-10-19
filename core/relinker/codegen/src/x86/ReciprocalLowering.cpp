#include <codegen/x86/ReciprocalLowering.hpp>
#include <array>
#include <cstring>

namespace Codegen {

namespace {

constexpr std::uint8_t kSqrtps = 0x51;
constexpr std::uint8_t kDivps = 0x5E;
constexpr std::uint8_t kMovaps = 0x28;
constexpr float kOne = 1.0f;

StubConstant _ones() {
    StubConstant constant{};
    for (std::size_t lane = 0; lane < 4; ++lane)
        std::memcpy(constant.data() + lane * sizeof(kOne), &kOne, sizeof(kOne));
    return constant;
}

std::uint8_t _scratch(const ReciprocalOperands& operands) {
    std::uint8_t reg = 0;
    while (reg == operands.Destination || reg == operands.Source)
        ++reg;
    return reg;
}

void _vexMovaps(StubBodyBuilder& body, const std::uint8_t dst, const std::uint8_t src) {
    const std::array<std::uint8_t, 5> bytes{
        0xC4,
        static_cast<std::uint8_t>(((dst & 8) != 0 ? 0x00 : 0x80) | 0x40 | ((src & 8) != 0 ? 0x00 : 0x20) | 0x01),
        0x78,
        kMovaps,
        static_cast<std::uint8_t>(0xC0 | ((dst & 7) << 3) | (src & 7))
    };
    body.Raw(bytes);
}

}

void ReciprocalLowering::EmitOutOfLine(StubBodyBuilder& body, const ReciprocalOperands& operands) const {
    const auto scratch = _scratch(operands);
    const auto dst = operands.Destination;
    body.Spill(scratch);
    body.Raw(std::array<std::uint8_t, 22>{0x48, 0x8D, 0x64, 0x24, 0xF0, 0x0F, 0xAE, 0x1C, 0x24, 0xC7, 0x44, 0x24, 0x04, 0xC0, 0x9F, 0x00, 0x00, 0x0F, 0xAE, 0x54, 0x24, 0x04});
    if (operands.Operation == ReciprocalOperation::ReciprocalSquareRoot) {
        body.SsePlain({0x0F, kSqrtps}, scratch, operands.Source);
        body.RipOperand({0x0F, kMovaps}, dst, _ones());
        body.SsePlain({0x0F, kDivps}, dst, scratch);
        if (operands.Vex)
            _vexMovaps(body, dst, dst);
    } else {
        body.RipOperand({0x0F, kMovaps}, scratch, _ones());
        body.SsePlain({0x0F, kDivps}, scratch, operands.Source);
        if (operands.Vex)
            _vexMovaps(body, dst, scratch);
        else
            body.SsePlain({0x0F, kMovaps}, dst, scratch);
    }
    body.Raw(std::array<std::uint8_t, 9>{0x0F, 0xAE, 0x14, 0x24, 0x48, 0x8D, 0x64, 0x24, 0x10});
    body.Restore(scratch);
}

LoweredBody ReciprocalLowering::LowerOutOfLine(const ReciprocalOperands& operands) const {
    StubBodyBuilder body;
    EmitOutOfLine(body, operands);
    return body.Finish();
}

}

#ifndef CORE_WASM_RUNTIME_INCLUDE_WASM_RUNTIME_GUEST_HPP
#define CORE_WASM_RUNTIME_INCLUDE_WASM_RUNTIME_GUEST_HPP

#include "wasm-runtime/GuestCpu.hpp"
#include <cstddef>
#include <cstdint>
#include <pthread.h>
#include <span>

struct VaList;

namespace WasmRuntime {

enum class ValueKind : std::uint8_t {
    Integer,
    Float32,
    Float64,
};

struct GuestValue {
    ValueKind kind = ValueKind::Integer;
    std::uint64_t bits = 0;
};

struct GuestResult {
    std::uint64_t rax = 0;
    std::uint64_t rdx = 0;
    std::uint64_t xmm0 = 0;
};

GuestCpu& CurrentCpu();
void SetNextThreadStackSize(std::size_t bytes);
void CurrentGuestStack(std::uint64_t& low, std::uint64_t& high);

GuestResult CallGuest(std::uint64_t target, std::span<const GuestValue> arguments);
[[noreturn]] void RunGuestEntry(std::uint64_t entry, std::uint64_t stack);

void TransferGuest(std::uint64_t rip, std::uint64_t rsp);
bool GuestTransferPending();
bool TakeGuestTransfer();

VaList* GuestVaList();
void SetGuestVaList(VaList* list);

[[noreturn]] void GuestFault(const char* format, ...);

using SignalDelivery = void (*)(GuestCpu& cpu, std::uint64_t& rip, std::uint64_t& rsp);
void SetSignalDelivery(SignalDelivery delivery);
bool RaiseSignal(pthread_t thread);
void DeliverPendingSignals(GuestCpu& cpu, std::uint64_t& rip, std::uint64_t& rsp);

}  // namespace WasmRuntime

#endif

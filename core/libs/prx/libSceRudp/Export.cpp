#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <map>
#include <mutex>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

// Offline reliable-UDP: the library initialises and contexts can be created and bound, but no connection can
// ever be established because there is no network.

namespace {
constexpr int RUDP_ERROR_NOT_INITIALIZED = static_cast<int>(0x80770001u);
constexpr int RUDP_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x80770002u);
constexpr int RUDP_ERROR_INVALID_CONTEXT_ID = static_cast<int>(0x80770003u);
constexpr int RUDP_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80770004u);
constexpr int RUDP_ERROR_INVALID_OPTION = static_cast<int>(0x80770005u);
constexpr int RUDP_ERROR_MEMORY = static_cast<int>(0x80770007u);
constexpr int RUDP_ERROR_CONN_RESET = static_cast<int>(0x80770009u);
constexpr int RUDP_ERROR_CONN_REFUSED = static_cast<int>(0x8077000Au);
constexpr int RUDP_ERROR_NOT_ACCEPTABLE = static_cast<int>(0x80770011u);

constexpr int RUDP_OPTION_MAX_PAYLOAD = 1;
constexpr int RUDP_OPTION_SNDBUF = 2;
constexpr int RUDP_OPTION_RCVBUF = 3;
constexpr int RUDP_OPTION_NODELAY = 4;
constexpr int RUDP_OPTION_DELIVERY_CRITICAL = 5;
constexpr int RUDP_OPTION_ORDER_CRITICAL = 6;
constexpr int RUDP_OPTION_NONBLOCK = 7;
constexpr int RUDP_OPTION_STREAM = 8;
constexpr int RUDP_OPTION_CONNECTION_TIMEOUT = 9;
constexpr int RUDP_OPTION_CLOSE_WAIT_TIMEOUT = 10;
constexpr int RUDP_OPTION_AGGREGATION_TIMEOUT = 11;
constexpr int RUDP_OPTION_LAST_ERROR = 14;
constexpr int RUDP_OPTION_READ_TIMEOUT = 15;
constexpr int RUDP_OPTION_WRITE_TIMEOUT = 16;
constexpr int RUDP_OPTION_FLUSH_TIMEOUT = 17;
constexpr int RUDP_OPTION_KEEP_ALIVE_INTERVAL = 18;
constexpr int RUDP_OPTION_KEEP_ALIVE_TIMEOUT = 19;

constexpr int RUDP_POOL_MIN_SIZE = 0xC58;
constexpr std::uintptr_t RUDP_POOL_ALIGNMENT = 8;
constexpr std::size_t RUDP_STATUS_SIZE = 0xF8;
constexpr std::size_t RUDP_STATUS_CURRENT_CONTEXTS = 0x64;
constexpr uint16_t RUDP_MAX_SEGMENT_SIZE = 1410;

using GuestRudpEventHandler = void(APS5_VABI*)(int, int, int, void*);

struct GuestRudpContext {
    bool bound = false;
    std::map<int, std::int32_t> options = {
        {RUDP_OPTION_MAX_PAYLOAD, 1346},
        {RUDP_OPTION_SNDBUF, 65536},
        {RUDP_OPTION_RCVBUF, 65536},
        {RUDP_OPTION_NODELAY, 0},
        {RUDP_OPTION_DELIVERY_CRITICAL, 1},
        {RUDP_OPTION_ORDER_CRITICAL, 1},
        {RUDP_OPTION_NONBLOCK, 1},
        {RUDP_OPTION_STREAM, 0},
        {RUDP_OPTION_CONNECTION_TIMEOUT, 60000},
        {RUDP_OPTION_CLOSE_WAIT_TIMEOUT, 0},
        {RUDP_OPTION_AGGREGATION_TIMEOUT, 30},
        {RUDP_OPTION_LAST_ERROR, 0},
        {RUDP_OPTION_READ_TIMEOUT, 0},
        {RUDP_OPTION_WRITE_TIMEOUT, 0},
        {RUDP_OPTION_FLUSH_TIMEOUT, 0},
        {RUDP_OPTION_KEEP_ALIVE_INTERVAL, 0},
        {RUDP_OPTION_KEEP_ALIVE_TIMEOUT, 0},
    };
};

std::mutex g_mutex;
bool g_inited = false;
int g_next_ctx = 0;
std::map<int, GuestRudpContext> g_ctx;
GuestRudpEventHandler g_handler = nullptr;
void* g_handler_arg = nullptr;

bool valid_ctx(int ctx) {
    return g_ctx.count(ctx) != 0;
}
}  // namespace

extern "C" {

int APS5_VABI sceRudpInit_nid_postfix(void* mem_pool, int mem_pool_size) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_inited) {
        return RUDP_ERROR_ALREADY_INITIALIZED;
    }
    if (mem_pool == nullptr || mem_pool_size <= 0) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    if (mem_pool_size < RUDP_POOL_MIN_SIZE || reinterpret_cast<std::uintptr_t>(mem_pool) % RUDP_POOL_ALIGNMENT != 0) {
        return RUDP_ERROR_MEMORY;
    }
    g_inited = true;
    return 0;
}

int APS5_VABI sceRudpActivate() {
    return 0;
}

int APS5_VABI sceRudpGetStatus(void* status, std::size_t size) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (status == nullptr || size == 0 || size > RUDP_STATUS_SIZE) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    unsigned char report[RUDP_STATUS_SIZE] = {};
    const auto contexts = static_cast<uint32_t>(g_ctx.size());
    std::memcpy(report + RUDP_STATUS_CURRENT_CONTEXTS, &contexts, sizeof(contexts));
    std::memcpy(status, report, size);
    return 0;
}

int APS5_VABI sceRudpEnd(void) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    g_inited = false;
    g_ctx.clear();
    return 0;
}

int APS5_VABI sceRudpTerminate(int ctx_id) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    return g_ctx.erase(ctx_id) != 0 ? 0 : RUDP_ERROR_INVALID_CONTEXT_ID;
}

int APS5_VABI sceRudpEnableInternalIOThread(uint32_t stack_size, uint32_t priority) {
    (void)stack_size;
    (void)priority;
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_inited ? 0 : RUDP_ERROR_NOT_INITIALIZED;
}

int APS5_VABI sceRudpEnableInternalIOThread2() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRudpSetEventHandler(GuestRudpEventHandler handler, void* arg) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_handler = handler;
    g_handler_arg = arg;
    return 0;
}

int APS5_VABI sceRudpCreateContext(GuestRudpEventHandler handler, void* arg, int* ctx_id) {
    (void)handler;
    (void)arg;
    if (ctx_id == nullptr) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    const int id = g_next_ctx++;
    g_ctx[id] = GuestRudpContext{};
    *ctx_id = id;
    return 0;
}

int APS5_VABI sceRudpBind(int ctx_id, uint16_t vport, int transport, uint16_t port) {
    (void)vport;
    (void)transport;
    (void)port;
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (!valid_ctx(ctx_id)) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    g_ctx[ctx_id].bound = true;
    return 0;
}

int APS5_VABI sceRudpSetOption(int ctx_id, int option, const void* value, uint32_t len) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    const auto contextIt = g_ctx.find(ctx_id);
    if (contextIt == g_ctx.end()) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    const auto optionIt = contextIt->second.options.find(option);
    if (optionIt == contextIt->second.options.end()) {
        return RUDP_ERROR_INVALID_OPTION;
    }
    if (value == nullptr || len != sizeof(std::int32_t)) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    std::memcpy(&optionIt->second, value, sizeof(std::int32_t));
    return 0;
}

int APS5_VABI sceRudpGetOption(int ctx_id, int option, void* value, uint32_t len) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    const auto contextIt = g_ctx.find(ctx_id);
    if (contextIt == g_ctx.end()) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    const auto optionIt = contextIt->second.options.find(option);
    if (optionIt == contextIt->second.options.end()) {
        return RUDP_ERROR_INVALID_OPTION;
    }
    if (value == nullptr || len != sizeof(std::int32_t)) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    std::memcpy(value, &optionIt->second, sizeof(std::int32_t));
    return 0;
}

int APS5_VABI sceRudpInitiate(int ctx_id, const void* to, uint32_t tolen, uint16_t vport) {
    (void)to;
    (void)tolen;
    (void)vport;
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (!valid_ctx(ctx_id)) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    return RUDP_ERROR_CONN_REFUSED;
}

int APS5_VABI sceRudpGetContextStatus(int ctx_id, void* status, uint32_t size) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (!valid_ctx(ctx_id)) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    if (status != nullptr && size != 0) {
        std::memset(status, 0, size);  // state 0 = idle, nothing pending
    }
    return 0;
}

int APS5_VABI sceRudpGetRemoteInfo() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRudpGetNumberOfPacketsToRead(int ctx_id) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (ctx_id < 0) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    if (!valid_ctx(ctx_id)) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    return 0;
}

int APS5_VABI sceRudpGetSizeWritable(int ctx_id) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (ctx_id < 0) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    if (!valid_ctx(ctx_id)) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    return RUDP_ERROR_NOT_ACCEPTABLE;
}

int APS5_VABI sceRudpGetSizeReadable(int ctx_id) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (ctx_id < 0) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    if (!valid_ctx(ctx_id)) {
        return RUDP_ERROR_INVALID_CONTEXT_ID;
    }
    return RUDP_ERROR_NOT_ACCEPTABLE;
}

int APS5_VABI sceRudpWrite(int ctx_id, const void* data, uint32_t len, int flags) {
    (void)data;
    (void)len;
    (void)flags;
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    return valid_ctx(ctx_id) ? RUDP_ERROR_CONN_RESET : RUDP_ERROR_INVALID_CONTEXT_ID;
}

int APS5_VABI sceRudpRead(int ctx_id, void* data, uint32_t len, int flags, void* status) {
    (void)data;
    (void)len;
    (void)flags;
    (void)status;
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    return valid_ctx(ctx_id) ? RUDP_ERROR_CONN_RESET : RUDP_ERROR_INVALID_CONTEXT_ID;
}

int APS5_VABI sceRudpNetFlush(void) {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_inited ? 0 : RUDP_ERROR_NOT_INITIALIZED;
}

int APS5_VABI sceRudpProcessEvents(void) {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_inited ? 0 : RUDP_ERROR_NOT_INITIALIZED;
}

int APS5_VABI sceRudpNetReceived(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceRudpGetMaxSegmentSize(uint16_t* size) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_inited) {
        return RUDP_ERROR_NOT_INITIALIZED;
    }
    if (size == nullptr) {
        return RUDP_ERROR_INVALID_ARGUMENT;
    }
    *size = RUDP_MAX_SEGMENT_SIZE;
    return 0;
}

}

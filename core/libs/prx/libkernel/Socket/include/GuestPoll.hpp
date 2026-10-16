#ifndef CORE_LIBS_PRX_LIBKERNEL_SOCKET_GUESTPOLL_HPP
#define CORE_LIBS_PRX_LIBKERNEL_SOCKET_GUESTPOLL_HPP

#include <cstddef>
#include <cstdint>

struct GuestPollDescriptor {
    std::int32_t descriptor;
    std::int16_t events;
    std::int16_t revents;
};
static_assert(sizeof(GuestPollDescriptor) == 8);
static_assert(offsetof(GuestPollDescriptor, events) == 4);
static_assert(offsetof(GuestPollDescriptor, revents) == 6);

#endif

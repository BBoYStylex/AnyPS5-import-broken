#pragma once

#include <cstdint>

struct AVChannelLayout {
    int order;
    int nb_channels;
    union {
        std::uint64_t mask;
    } u;
    void* opaque;
};

#define AV_CHANNEL_LAYOUT_MONO {0, 1, {0x4}, nullptr}
#define AV_CHANNEL_LAYOUT_STEREO {0, 2, {0x3}, nullptr}
#define AV_CHANNEL_LAYOUT_5POINT1_BACK {0, 6, {0x60f}, nullptr}

inline int av_channel_layout_compare(const AVChannelLayout* a, const AVChannelLayout* b) {
    return a->nb_channels == b->nb_channels && a->u.mask == b->u.mask ? 0 : 1;
}

inline void av_channel_layout_default(AVChannelLayout* layout, int channels) {
    *layout = {0, channels, {0}, nullptr};
}

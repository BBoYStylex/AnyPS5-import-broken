#pragma once

#include <cstdint>
#include "libavutil/channel_layout.h"
#include "libavutil/log.h"
#include "libavutil/mem.h"

#define AVERROR(error) (-(error))
#define AV_INPUT_BUFFER_PADDING_SIZE 64
#define AV_PROFILE_AAC_HE 4
#define AV_PROFILE_AAC_HE_V2 28

enum AVCodecID { AV_CODEC_ID_MP3 = 0x15001, AV_CODEC_ID_AAC = 0x15002, AV_CODEC_ID_OPUS = 0x1503c };
enum AVSampleFormat { AV_SAMPLE_FMT_FLT = 3, AV_SAMPLE_FMT_S16P = 6, AV_SAMPLE_FMT_FLTP = 8 };

struct AVCodec {};

struct AVCodecContext {
    AVChannelLayout ch_layout;
    int sample_rate;
    int profile;
    std::uint8_t* extradata;
    int extradata_size;
};

struct AVPacket {
    std::uint8_t* data;
    int size;
};

struct AVFrame {
    std::uint8_t* data[8];
    std::uint8_t** extended_data;
    int nb_samples;
    int format;
    int sample_rate;
    AVChannelLayout ch_layout;
};

inline const AVCodec* avcodec_find_decoder(AVCodecID) { return nullptr; }
inline AVCodecContext* avcodec_alloc_context3(const AVCodec*) { return nullptr; }
inline int avcodec_open2(AVCodecContext*, const AVCodec*, void**) { return AVERROR(38); }
inline void avcodec_free_context(AVCodecContext** context) { *context = nullptr; }
inline void avcodec_flush_buffers(AVCodecContext*) {}
inline int avcodec_send_packet(AVCodecContext*, const AVPacket*) { return AVERROR(38); }
inline int avcodec_receive_frame(AVCodecContext*, AVFrame*) { return AVERROR(38); }
inline AVPacket* av_packet_alloc() { return nullptr; }
inline void av_packet_free(AVPacket** packet) { *packet = nullptr; }
inline AVFrame* av_frame_alloc() { return nullptr; }
inline void av_frame_free(AVFrame** frame) { *frame = nullptr; }
inline void av_frame_unref(AVFrame*) {}

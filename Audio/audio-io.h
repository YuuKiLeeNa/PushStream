#pragma once
#ifdef _cplusplus
extern "C"{
#endif
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
#ifdef _cplusplus
}
#endif
#include "media-io-defs.h"
#include<memory>

/**
 * The speaker layout describes where the speakers are located in the room.
 * For OBS it dictates:
 *  *  how many channels are available and
 *  *  which channels are used for which speakers.
 *
 * Standard channel layouts where retrieved from ffmpeg documentation at:
 *     https://trac.ffmpeg.org/wiki/AudioChannelManipulation
 */

#define MAX_AUDIO_CHANNELS 8

enum speaker_layout {
	SPEAKERS_UNKNOWN = AV_CHAN_NONE,     /**< Unknown setting, fallback is stereo. */
	SPEAKERS_MONO = AV_CH_LAYOUT_MONO,        /**< Channels: MONO */
	SPEAKERS_STEREO = AV_CH_LAYOUT_STEREO,      /**< Channels: FL, FR */
	SPEAKERS_2POINT1 = AV_CH_LAYOUT_2POINT1,     /**< Channels: FL, FR, LFE */
	SPEAKERS_4POINT0 = AV_CH_LAYOUT_4POINT0,     /**< Channels: FL, FR, FC, RC */
	SPEAKERS_4POINT1 = AV_CH_LAYOUT_4POINT1,     /**< Channels: FL, FR, FC, LFE, RC */
	SPEAKERS_5POINT1 = AV_CH_LAYOUT_5POINT1,     /**< Channels: FL, FR, FC, LFE, RL, RR */
	SPEAKERS_7POINT1 = AV_CH_LAYOUT_7POINT1, /**< Channels: FL, FR, FC, LFE, RL, RR, SL, SR */
};


enum audio_format {
	AUDIO_FORMAT_UNKNOWN = AV_SAMPLE_FMT_NONE,

	AUDIO_FORMAT_U8BIT = AV_SAMPLE_FMT_U8,
	AUDIO_FORMAT_16BIT = AV_SAMPLE_FMT_S16,
	AUDIO_FORMAT_32BIT = AV_SAMPLE_FMT_S32,
	AUDIO_FORMAT_FLOAT = AV_SAMPLE_FMT_FLT,

	AUDIO_FORMAT_U8BIT_PLANAR = AV_SAMPLE_FMT_U8P,
	AUDIO_FORMAT_16BIT_PLANAR = AV_SAMPLE_FMT_S16P,
	AUDIO_FORMAT_32BIT_PLANAR = AV_SAMPLE_FMT_S32P,
	AUDIO_FORMAT_FLOAT_PLANAR = AV_SAMPLE_FMT_FLTP,
};


static inline uint32_t get_audio_channels(enum speaker_layout speakers)
{
	switch (speakers) {
	case SPEAKERS_MONO:
		return 1;
	case SPEAKERS_STEREO:
		return 2;
	case SPEAKERS_2POINT1:
		return 3;
	case SPEAKERS_4POINT0:
		return 4;
	case SPEAKERS_4POINT1:
		return 5;
	case SPEAKERS_5POINT1:
		return 6;
	case SPEAKERS_7POINT1:
		return 8;
	case SPEAKERS_UNKNOWN:
		return 0;
	}
	return 0;
}

static inline bool is_audio_planar(enum audio_format format)
{
	switch (format) {
	case AUDIO_FORMAT_U8BIT:
	case AUDIO_FORMAT_16BIT:
	case AUDIO_FORMAT_32BIT:
	case AUDIO_FORMAT_FLOAT:
		return false;

	case AUDIO_FORMAT_U8BIT_PLANAR:
	case AUDIO_FORMAT_FLOAT_PLANAR:
	case AUDIO_FORMAT_16BIT_PLANAR:
	case AUDIO_FORMAT_32BIT_PLANAR:
		return true;

	case AUDIO_FORMAT_UNKNOWN:
		return false;
	}

	return false;
}

static inline size_t get_audio_planes(enum audio_format format,
	enum speaker_layout speakers)
{
	return (is_audio_planar(format) ? get_audio_channels(speakers) : 1);
}


struct audio_data {
	uint8_t* data[MAX_AV_PLANES];
	uint32_t frames;
	uint64_t timestamp;
};


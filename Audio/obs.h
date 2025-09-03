#pragma once
#include<cstdint>
#include "media-io-defs.h"



struct obs_source_audio {
	uint8_t* data[MAX_AV_PLANES];
	uint32_t frames;

	enum speaker_layout speakers;
	enum audio_format format;
	uint32_t samples_per_sec;
	uint32_t capacity;
	uint64_t timestamp;
};





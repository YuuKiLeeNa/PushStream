#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavutil/frame.h"
#include "libavutil/channel_layout.h"

#ifdef __cplusplus
}
#endif

int ff_frame_alloc_ref(AVFrame** dst, AVFrame* src);

int ff_frame_ref(AVFrame* dst, AVFrame* src);

int ff_frame_get_buffer(AVFrame* dst);


//AVChannelLayout ff_channel_layout(uint64_t channel_layout);
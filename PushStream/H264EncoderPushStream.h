#pragma once
#include"H264Encoder.h"
#include "xop/media.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/rational.h"
#ifdef __cplusplus
}
#endif

//https://blog.csdn.net/water1209/article/details/128718647
class H264EncoderPushStream :public H264Encoder
{
public:
	H264EncoderPushStream() = default;
	~H264EncoderPushStream() = default;

	int send_frame(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base = {0,1});
	int send_frame_init(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base = {0,1});

protected:
	bool IsKeyFrame(uint8_t c);
};


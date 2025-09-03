#pragma once
#include "AACEncoder.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/rational.h"
#ifdef __cplusplus
}
#endif
#include<vector>
#include<memory>
#include "xop/media.h"

class AACEncoderPushStream:public AACEncoder
{
public:
	AACEncoderPushStream() = default;
	~AACEncoderPushStream() = default;
	int send_frame(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base = { 0,1 });
	int send_frame_init(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base = { 0,1 });

};
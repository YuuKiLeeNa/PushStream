#include "AACEncoderPushStream.h"


int AACEncoderPushStream::send_frame(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base) 
{
	std::vector<UPTR_PKT>sets;
	int ret = AACEncoder::send_frame(f, sets);
	if (sets.empty())
		return ret;
	pkt.clear();
	sets.reserve(sets.size());
	for (auto& ele : sets) 
	{
		if (ele->size > 0)
		{
			std::shared_ptr<xop::AVFrame>frame = std::make_shared<xop::AVFrame>(ele->size);
			memcpy(frame->buffer.get(), ele->data, ele->size);
			frame->type = xop::FrameType::AUDIO_FRAME;
			if (time_base.num != 0 && time_base.den != 1) 
			{
				int64_t ts = av_rescale_q_rnd(ele->pts, ele->time_base, time_base, AVRounding::AV_ROUND_NEAR_INF);
				frame->timestamp = ts;
			}
			else
				frame->timestamp = ele->pts;
			pkt.emplace_back(std::move(frame));
		}
	}
	return ret;
}

int AACEncoderPushStream::send_frame_init(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base) 
{
	int ret = init(f);
	if (ret < 0)
		return ret;
	return send_frame(f, pkt, time_base);
}
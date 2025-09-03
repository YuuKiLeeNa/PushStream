#include "H264EncoderPushStream.h"
#include<algorithm>
#include<numeric>
#include<memory>


int H264EncoderPushStream::send_frame(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base)
{
	std::vector<UPTR_PKT>sets;
	int ret = H264Encoder::send_frame(f, sets);
	if (sets.size() == 0)
		return ret;
	pkt.clear();
	std::vector<uint8_t>datas = getExtendData();
	uint8_t startcode[] = {0,0,1};
	auto iterSPS_PPS = std::search(datas.cbegin(), datas.cend(), std::cbegin(startcode), std::cend(startcode));
	if (iterSPS_PPS != datas.cend())
		iterSPS_PPS = iterSPS_PPS + 3;

	for (auto& ele : sets) 
	{
		//start code
		int istartcode = 0;
		auto iter = std::search(ele->data, ele->data+ele->size, std::cbegin(startcode), std::cend(startcode));
		if (iter != ele->data + ele->size)
			istartcode = (int)(iter + 3 - ele->data);
		int needsize = 0;
		std::shared_ptr<xop::AVFrame>frame;
		if (IsKeyFrame(ele->data[istartcode]))
		{
			needsize = datas.cend() - iterSPS_PPS + ele->size;
			if (needsize > 0) 
			{
				frame = std::make_shared<xop::AVFrame>(needsize);
				memcpy(frame->buffer.get(), &*iterSPS_PPS, datas.cend() - iterSPS_PPS);
				memcpy(frame->buffer.get() + (datas.cend() - iterSPS_PPS), ele->data, ele->size);
				frame->type = xop:: FrameType::VIDEO_FRAME_I;
			}
		}
		else
		{
			needsize = ele->size - istartcode;
			if (needsize > 0)
			{
				frame = std::make_shared<xop::AVFrame>(needsize);
				memcpy(frame->buffer.get(), ele->data+ istartcode, ele->size - istartcode);
				frame->type = xop::FrameType::VIDEO_FRAME_P;
			}
		}
		if (frame)
		{
			if (time_base.num != 0 && time_base.den != 1)
			{
				int64_t ts64 = av_rescale_q_rnd(ele->pts, ele->time_base, time_base, AVRounding::AV_ROUND_UP);
				frame->timestamp = ts64;
			}
			else
				frame->timestamp = ele->pts;
			pkt.push_back(std::move(frame));
		}
	}
	return ret;
}

int H264EncoderPushStream::send_frame_init(AVFrame* f, std::vector<std::shared_ptr<xop::AVFrame>>& pkt, const AVRational& time_base)
{
	int ret = init(f);
	if (ret < 0)
		return ret;
	
	return send_frame(f, pkt, time_base);
}



bool H264EncoderPushStream::IsKeyFrame(uint8_t c)
{
	//if (size > 4) {
	//	//0x67:sps ,0x65:IDR, 0x6: SEI
	//	if (data[4] == 0x67 || data[4] == 0x65 ||
	//		data[4] == 0x6 || data[4] == 0x27) {
	//		return true;
	//	}
	//}
	if (c == 0x67 || c == 0x65 || c == 0x6 || c == 0x27)
		return true;
	return false;
}
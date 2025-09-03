#include "EncodeBase.h"
#include "Util/logger.h"
#include "Network/Socket.h"
EncodeBase::EncodeBase() {}

EncodeBase::~EncodeBase() 
{
}

int EncodeBase::send_frame(AVFrame* f, std::vector<UPTR_PKT>& pkt)
{
	int ret;
	//if (f)
	//{
		if ((ret = avcodec_send_frame(ctx_, f)) != 0)
		{
			freevpkt(pkt);
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			av_make_error_string(szErr,sizeof(szErr), ret);
			PrintE("avcodec_send_frame error%s", szErr);
			return ret;
		}
	//}
	//else
	//{
	//	if ((ret = avcodec_send_frame(nullptr, f)) != 0)
	//	{
	//		freevpkt(pkt);
	//		return ret;
	//	}
	//}
	int pktsize = pkt.size(), i = 0;
	bool exit = false;
	while (!exit)
	{
		AVPacket* tmpPkt;
		if (i < pktsize)
		{
			tmpPkt = pkt[i].get();
			av_packet_unref(tmpPkt);
		}
		else
		{
			tmpPkt = av_packet_alloc();
			pkt.emplace_back(tmpPkt);
			++pktsize;
		}
		if ((ret = avcodec_receive_packet(ctx_, tmpPkt)) != 0)
		{
			freevpkt(pkt, i);
			exit = true;
			if (!pkt.empty() && (ret == AVERROR(EAGAIN)
				|| ret == AVERROR_EOF))
			{
				ret = 0;
				break;
			}
			///////debug////////////////////
			if (ret != AVERROR(EAGAIN)
				&& ret != AVERROR_EOF)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				av_make_error_string(szErr, sizeof(szErr), ret);
				PrintE("avcodec_receive_packet error%s", szErr);
			}
			/////////////////////////////
		}
		else
		{
			++i;
			tmpPkt->time_base = ctx_->time_base;
		}
	}
	return ret;
}

int EncodeBase::send_frame_init(AVFrame* f, std::vector<UPTR_PKT>& pkt)
{
	int ret = init(f);
	if (ret < 0)
	{
		freevpkt(pkt);
		return ret;
	}
	return send_frame(f, pkt);
}

void EncodeBase::freevpkt(std::vector<UPTR_PKT>& pkt, int index)
{
	int s = pkt.size();
	for (int i = index; i < s; ++i)
		pkt[i].reset();
		//av_packet_free(&pkt[i]);
	if (index > s)
		index = s;
	pkt.erase(std::begin(pkt)+ index, std::end(pkt));
}
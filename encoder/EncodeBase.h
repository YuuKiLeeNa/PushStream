#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#ifdef __cplusplus
}
#endif
#include<vector>
#include<string>
#include<memory>
#include "Pushstream/define_type.h"


class EncodeBase 
{
public:
	EncodeBase();
	virtual ~EncodeBase();
	int send_frame(AVFrame* f, std::vector<UPTR_PKT>& pkt);
	int send_frame_init(AVFrame* f, std::vector<UPTR_PKT>& pkt);
	bool isinited() { return ctx_ != 0; }
protected:
	void freevpkt(std::vector<UPTR_PKT>& pkt, int index = 0);
	virtual int init(AVFrame* f) = 0;
protected:
	AVCodecContext* ctx_ = nullptr;
	AVDictionary* dict_ = nullptr;
	std::string encoder_name_;
};


#pragma once
#include "EncodeBase.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/samplefmt.h"
#ifdef __cplusplus
}
#endif
#include<vector>
#include<tuple>
/////////////////////////https://zhuanlan.zhihu.com/p/649512028
class AACEncoder :public EncodeBase
{
public:
	AACEncoder();
	~AACEncoder();
	int init(int sampleRate, uint64_t ch_layout, AVSampleFormat fmt, const std::string& encoderName="", AVDictionary* dict = nullptr);
	void setbitrate(int bitrate);
	void reset();
	static std::vector<uint8_t>SEQ_HEADER(const std::vector<uint8_t>&extradate);
	static std::tuple<int, int, int>PROFILE_SAMRATE_CH(const std::vector<uint8_t>& extradate);
	std::vector<uint8_t>getextradata();
	int channels()const;
	int framelen()const;
	int samplerate()const;
protected:
	virtual int init(AVFrame* f)override;
	int sampleRate_;
	//int channels_;
	AVChannelLayout chlay_;
	uint64_t ch_layout_;
	AVSampleFormat fmt_;
	int bitrate_ = 0;
	static const std::vector<int>sample_rate_;
};
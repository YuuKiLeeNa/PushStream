#pragma once
#include"EncodeBase.h"
#ifdef __cplusplus
extern "C"{
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"
#ifdef __cplusplus
}
#endif
#include<vector>
#include<string>

//https://blog.csdn.net/water1209/article/details/128718647
class H264Encoder :public EncodeBase
{
public:
	H264Encoder();
	~H264Encoder();
	int init(int w,int h,AVPixelFormat fmt, const std::string&encoderName = "", AVDictionary* dict = nullptr);
	void setbitrate(int bitrate);
	void setframerate(int framerate);

	int framerate() const;
	int width()const;
	int height()const;

	void setgopsize(int gopsize);
	void reset();
	std::vector<uint8_t> getExtendData()const;
	static std::pair<std::vector<uint8_t>, std::vector<uint8_t>>SPS_PPS(const std::vector<uint8_t>&data);
	std::pair<std::vector<uint8_t>, std::vector<uint8_t>> getSpsPps()const;
	static std::vector<uint8_t> SEQ_header(const std::pair<std::vector<uint8_t>, std::vector<uint8_t>>&data);
	std::vector<uint8_t>seqheader();
protected:
	virtual int init(AVFrame* f)override;
	bool IsKeyFrame(const uint8_t* data, uint32_t size);
	int bitrate_ = 8000000;
	int framerate_;
	int gopsize_ = 25;
	int w_;
	int h_;
	AVPixelFormat pix_;
};


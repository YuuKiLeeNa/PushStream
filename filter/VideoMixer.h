#pragma once
#include "VideoFilter.h"
#include <vector>
#include<atomic>
#include<vector>
//#include"ffMakeErrStr.h"
#include "MixerBase.h"
#ifdef __cplusplus
extern "C" {
#endif
#include"libavutil/rational.h"
#include "libavutil/buffer.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#ifdef __cplusplus
}
#endif

class VideoMixer :public MixerBase<VideoMixer>, public VideoFilter//, public ffMakeErrStr
{
public:
	typedef struct videoSrc
	{
		int w;
		int h;
		AVPixelFormat fmt;
		AVRational framerate;
		AVRational timebase;
		std::string scalew;
		std::string scaleh;
		std::string angle;
		std::string x;
		std::string y;
		int index;
	}videoSrc;

	VideoMixer();
	int addVideoSource(int w, int h, AVPixelFormat fmt, const AVRational &framerate,const AVRational &timebase, const std::string &scalew, const std::string &scaleh, const std::string &angle, const std::string& x, const std::string &y);
	int init();
	int setIndexOfMainOverLay(int i);
	void reset();
	//int addFrame(int indexOfFilter, AVFrame* pushFrame, bool isKeepRef = false);
	//int getFrame(AVFrame* f);
	//int addFrame(int indexOfFilter, AVFrame* pushFrame, bool isKeepRef = false);
	int getFrame(AVFrame* f);
protected:
	bool IsIndexRepeat(int index);
	std::vector<videoSrc>infosets_;
protected:
	int index_ = 0;
	int mainLay_ = 0;
};


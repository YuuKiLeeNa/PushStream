#pragma once
//#ifdef __cplusplus
//extern "C" {
//#endif
//#include "libavutil/frame.h"
//#include""
//#ifdef __cplusplus
//}
//#endif
#include<string>

struct AVFrame;
struct AVFilterContext;
struct AVFilterBufferSink;
struct AVFilterGraph;

class VideoFrameFilter
{
public:
	VideoFrameFilter();
	VideoFrameFilter(const std::string &strDescFilter);
	bool setStrDescFilter(const std::string& strDescFilter);
	virtual ~VideoFrameFilter();
	int operator()(AVFrame* fout, const AVFrame*fin);
	void reset();
protected:
	int init();
protected:
	std::string str_;
	AVFilterContext* in_ = nullptr;
	AVFilterContext* out_ = nullptr;
	AVFilterGraph* graph_ = nullptr;
};


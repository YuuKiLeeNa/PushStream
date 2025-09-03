#include "VideoFrameFilter.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/avfilter.h"
#ifdef __cplusplus
}
#endif

VideoFrameFilter::VideoFrameFilter() :str_("null")
{
}

VideoFrameFilter::VideoFrameFilter(const std::string& strDescFilter): str_(strDescFilter)
{
}

bool VideoFrameFilter::setStrDescFilter(const std::string& strDescFilter)
{
	str_ = strDescFilter;
	return true;
}

VideoFrameFilter::~VideoFrameFilter() 
{
	reset();
}

int VideoFrameFilter::operator()(AVFrame* fout, const AVFrame* fin) 
{
	return 0;
}

void VideoFrameFilter::reset() 
{
	if (graph_)
	{
		avfilter_graph_free(&graph_);
		in_ = nullptr;
		out_ = nullptr;
		str_ = "null";
	}
}

int VideoFrameFilter::init() 
{
	reset();
	graph_ = avfilter_graph_alloc();
	if (!graph_) 
	{
		//makeErrStr("avfilter_graph_alloc return nullptr");
		return -1;
	}
	AVFilterInOut *in = nullptr, *out = nullptr;
	int ret = avfilter_graph_parse2(graph_, str_.c_str(), &in, &out);
	if (ret != 0) 
	{
		//makeErrStr(ret,"avfilter_graph_parse2 error");
		return -2;
	}
	return 0;
}
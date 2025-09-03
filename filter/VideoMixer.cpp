#include "VideoMixer.h"
#include "VideoFilter.h"
#include<sstream>
#include<functional>
#include<algorithm>
#include<numeric>


VideoMixer::VideoMixer()
{
}

int VideoMixer::addVideoSource(int w, int h, AVPixelFormat fmt, const AVRational& framerate, const AVRational& timebase, const std::string& scalew, const std::string& scaleh, const std::string& angle, const std::string& x, const std::string& y)
{
	videoSrc item;
	item.w = w;
	item.h = h;
	item.fmt = fmt;
	item.framerate = framerate;
	item.timebase = timebase;
	item.scalew = scalew;
	item.scaleh = scaleh;
	item.angle = angle;
	item.x = x;
	item.y = y;

	{
		std::lock_guard<std::mutex>lock(data_.mutex_);
		do
		{
			item.index = index_++;
		} while (IsIndexRepeat(item.index));
		infosets_.push_back(std::move(item));
	}
	return item.index;
}

int VideoMixer::init() 
{
	if (!IsIndexRepeat(mainLay_))
		return AVERROR(EFAULT);
	int ret;
	std::lock_guard<std::mutex>m(data_.mutex_);
	data_.graph_ = std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(), [](AVFilterGraph*& f) {avfilter_graph_free(&f); });
	std::vector<std::unique_ptr<AVFilterContext, std::function<void(AVFilterContext*)>>>freeV;
	freeV.reserve(5* infosets_.size());
	std::function<void(AVFilterContext*)>freeCtx = [this](AVFilterContext*ctx)
	{
		avfilter_free(ctx);
	};
	auto initctx = [&freeV,&freeCtx](std::shared_ptr<FilterBase>b, AVFilterContext**f)->int
	{
		if (b->createFilter(f) < 0)
			return AVERROR(EFAULT);
		freeV.emplace_back(std::unique_ptr<std::remove_reference<decltype(freeV)>::type::value_type::element_type, std::remove_reference<decltype(freeV)>::type::value_type::deleter_type>(*f, freeCtx));
		return 0;
	};
	std::vector<std::tuple<int,AVFilterContext*, AVFilterContext*>>listsets;
	listsets.reserve(infosets_.size());
	for (auto& ele : infosets_) 
	{
		AVFilterContext* bufferctx = nullptr;
		std::shared_ptr<bufferFilter>buffer = std::make_shared<bufferFilter>(ele.w, ele.h, AVRational{ 1,1 }, ele.timebase, ele.framerate, ele.fmt, ele.index,data_);
		if ((ret = initctx(buffer, &bufferctx)) < 0)
			return ret;

		AVFilterContext* formatctx = nullptr;
		std::shared_ptr<formatFilter>format = std::make_shared<formatFilter>(AV_PIX_FMT_YUV420P, ele.index, data_);
		if ((ret = initctx(format, &formatctx)) < 0)
			return ret;

		AVFilterContext* scalectx = nullptr;
		std::shared_ptr<scaleFilter>scale = std::make_shared<scaleFilter>(ele.w,ele.h, ele.index, data_);
		if ((ret = initctx(scale, &scalectx)) < 0)
			return ret;

		AVFilterContext* rotatectx = nullptr;
		std::shared_ptr<rotateFilter>rotate = std::make_shared<rotateFilter>(ele.angle,ele.scalew, ele.scaleh, "0x00000000", true, ele.index, data_);
		if ((ret = initctx(rotate, &rotatectx)) < 0)
			return ret;

		AVFilterContext* sinkoroverlayctx = nullptr;
		if (ele.index ==mainLay_)
		{
			std::shared_ptr<buffersinkFilter>sinkoroverlay = std::make_shared<buffersinkFilter>(AV_PIX_FMT_YUV420P, ele.index, data_);
			if ((ret = initctx(sinkoroverlay, &sink_)) < 0)
				return ret;

			if (avfilter_link(bufferctx, 0, scalectx, 0) != 0
				|| avfilter_link(scalectx, 0, formatctx, 0) != 0
				|| avfilter_link(formatctx, 0, rotatectx, 0) != 0)
				return ret;
			listsets.emplace_back(ele.index, bufferctx, rotatectx);
		}
		else 
		{
			std::shared_ptr<overlayFilter>sinkoroverlay = std::make_shared<overlayFilter>(ele.x,ele.y, ele.index,data_);
			if ((ret = initctx(sinkoroverlay, &sinkoroverlayctx)) < 0)
				return ret;

			if (avfilter_link(bufferctx, 0, scalectx, 0) != 0
				|| avfilter_link(scalectx, 0, formatctx, 0) != 0
				|| avfilter_link(formatctx, 0, rotatectx, 0) != 0
				|| avfilter_link(rotatectx, 0, sinkoroverlayctx, 1) != 0)
				return ret;
			listsets.emplace_back(ele.index, bufferctx, sinkoroverlayctx);
		}
	}
	int size = listsets.size();
	auto iter = std::find_if(listsets.begin(), listsets.end(), [this](decltype(listsets)::reference ele) {return mainLay_ == std::get<0>(ele); });
	if (iter == listsets.end())
		return AVERROR(EFAULT);
	AVFilterContext* tmp = std::get<2>(*iter);
	for (auto& ele : listsets)
	{
		if (std::get<0>(ele) == mainLay_)
			continue;
		if (avfilter_link(tmp, 0, std::get<2>(ele), 0) != 0)
			return AVERROR(EFAULT);
		tmp = std::get<2>(ele);
	}
	if (avfilter_link(tmp, 0, sink_, 0) != 0)
		return AVERROR(EFAULT);
	for (auto& ele : listsets)
	{
		auto retv = ctx_.insert({ std::get<0>(ele), std::get<1>(ele) });
		if (!retv.second) 
		{
			ctx_.clear();
			return AVERROR(EFAULT);
		}
	}
	if ((ret = avfilter_graph_config(data_.graph_.get(), nullptr)) < 0) 
	{
		ctx_.clear();
		return ret;
	}
	for (auto& ele : freeV)
		ele.release();
	return 0;
}

int VideoMixer::setIndexOfMainOverLay(int i) 
{
	mainLay_ = i;
	return 0;
}

void VideoMixer::reset() 
{
	infosets_.clear();
	ctx_.clear();
	index_ = 0;
	mainLay_ = 0;
	sink_ = nullptr;
	data_.filters_.clear();
	data_.graph_.reset();
}

//int VideoMixer::addFrame(int indexOfFilter, AVFrame* pushFrame, bool isKeepRef)
//{
//	int ret;
//	int flags = AV_BUFFERSRC_FLAG_PUSH;
//	if (isKeepRef)
//		flags |= AV_BUFFERSRC_FLAG_KEEP_REF;
//	{
//		std::lock_guard<std::mutex>lock(data_.mutex_); 
//			auto iter = ctx_.find(indexOfFilter);
//		if (iter == ctx_.end())
//		{
//			if (flags & AV_BUFFERSRC_FLAG_KEEP_REF)
//				av_frame_unref(pushFrame);
//			return AVERROR(ERANGE);
//		}
//		ret = av_buffersrc_add_frame_flags(iter->second, pushFrame, flags);
//	}
//	if (ret < 0)
//	{
//		makeErrStr(ret, "av_buffersrc_add_frame_flags error:");
//		return ret;
//	}
//	return 0;
//}

int VideoMixer::getFrame(AVFrame* f) 
{
	int ret;
	{
		std::lock_guard<std::mutex>lock(data_.mutex_);
		if (sink_)
			ret = av_buffersink_get_frame_flags(sink_, f, AV_BUFFERSINK_FLAG_NO_REQUEST);
	}
	if (ret < 0)
	{
		if (ret == AVERROR(EAGAIN))
			return ret;
		//makeErrStr(ret, "av_buffersink_get_frame_flags error:");
		return ret;
	}
	return 0;
}


bool VideoMixer::IsIndexRepeat(int index)
{
	auto iter = std::find_if(infosets_.cbegin(), infosets_.cend(), [index](decltype(infosets_)::const_reference v)
		{
			return v.index == index;
		});

	return iter != infosets_.cend();
}
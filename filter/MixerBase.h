#pragma once
//#include "VideoFilter.h"
//#include "ffMakeErrStr.h"
#include<map>
#include<mutex>
#ifdef __cplusplus
extern "C" 
{
#endif
#include <libavfilter/buffersrc.h>
#ifdef __cplusplus
}
#endif

#include "Util/logger.h"
#include "Network/Socket.h"

struct AVFilterContext;
struct AVFrame;

template<typename Derived>
struct MixerBase
{
public:
	virtual ~MixerBase() {}
	int addFrame(int indexOfFilter, AVFrame* pushFrame, bool isKeepRef = false) 
	{
		int ret;
		int flags = AV_BUFFERSRC_FLAG_PUSH;
		Derived*pThis = (Derived*)this;
		if (isKeepRef)
			flags |= AV_BUFFERSRC_FLAG_KEEP_REF;
		{
			std::lock_guard<std::mutex>lock(pThis->data_.mutex_);
			auto iter = ctx_.find(indexOfFilter);
			if (iter == ctx_.end())
			{
				if (flags & AV_BUFFERSRC_FLAG_KEEP_REF)
					av_frame_unref(pushFrame);
				return AVERROR(ERANGE);
			}
			ret = av_buffersrc_add_frame_flags(iter->second, pushFrame, flags);
		}
		if (ret < 0)
		{
			//pThis->makeErrStr(ret, "av_buffersrc_add_frame_flags error:");
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_buffersrc_add_frame_flags error:%s", av_make_error_string(szErr,sizeof(szErr), ret));
			return ret;
		}
		return 0;
	}

protected:
	AVFilterContext* sink_;
	std::map<int, AVFilterContext*>ctx_;
};
#include "some_util_fun.h"
#include "Util/logger.h"
#include "Network/Socket.h"
#include "define_type.h"

int ff_frame_alloc_ref(AVFrame** dst, AVFrame* src)
{
	UPTR_FME fdst(av_frame_alloc());
	if (!fdst)
	{
		PrintE("av_frame_alloc no memory");
		return AVERROR(ENOMEM);
	}
	int ret = ff_frame_ref(fdst.get(), src);
	if(ret == 0)
		*dst = fdst.release();
	return ret;
}

int ff_frame_ref(AVFrame* dst, AVFrame* src)
{
	int ret = av_frame_ref(dst, src);
	if (ret < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_ref error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	//but not width/height or channel layout.XXXX
	//ret = av_frame_copy_props(dst, src);
	////av_channel_layout_copy(&dst->ch_layout, &src->ch_layout);
	////dst->height = src->height;
	////dst->width = src->width;
	//if (ret < 0)
	//{
	//	char szErr[AV_ERROR_MAX_STRING_SIZE];
	//	PrintE("av_frame_copy_props error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	//	return ret;
	//}
	return 0;
}

int ff_frame_get_buffer(AVFrame* dst) 
{
	int ret = av_frame_get_buffer(dst, 0);
	if (ret != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	if ((ret = av_frame_make_writable(dst)) != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_make_writable error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	}
	return ret;
}
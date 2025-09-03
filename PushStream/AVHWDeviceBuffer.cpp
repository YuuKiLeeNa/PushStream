#include "AVHWDeviceBuffer.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/hwcontext.h"

#ifdef __cplusplus
}
#endif

void AVHWDeviceBuffer::reset() 
{
	av_buffer_unref(&hwdevice_);
	av_buffer_unref(&hwframe_);
}

AVHWDeviceBuffer::~AVHWDeviceBuffer() 
{
	reset();
}

int AVHWDeviceBuffer::initCuda(int w,int h)
{
	int ret = av_hwdevice_ctx_create(&hwdevice_, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0);
	if (ret != 0)
		return -1;
	if (!(hwframe_ = av_hwframe_ctx_alloc(hwdevice_)))
		return -1;

	AVHWFramesContext*frameCtx = (AVHWFramesContext*)hwframe_->data;
	frameCtx->width = w;
	frameCtx->height = h;
	frameCtx->initial_pool_size = 20;
	frameCtx->format = AV_PIX_FMT_CUDA;
	frameCtx->sw_format = AV_PIX_FMT_YUV420P;

	if ((ret = av_hwframe_ctx_init(hwframe_)) != 0)
	{
		char sz[256];
		av_make_error_string(sz, sizeof(sz), ret);
		return -1;
	}
	return 0;
}

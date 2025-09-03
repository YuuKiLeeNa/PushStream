#pragma once
#include "CEncoderOpt.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/rational.h"
#include "libavcodec/codec_id.h"
#include "libavcodec/codec.h"
#include "libavdevice/avdevice.h"
#ifdef __cplusplus
}
#endif
#include "Pushstream/define_type.h"
#include<vector>
#include "PushStream/imageConvert.h"


class CVideoEncoder:virtual public CEncoderOpt
{
public:
	int send_Frame(UPTR_FME f, std::vector<UPTR_PKT>& sets);
public:
	~CVideoEncoder();
	void reset();
	bool testCodecOfEncoder(const AVCodec* codec, const AVCodecHWConfig* pConfig);
	std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>testCodecOfEncoder(const std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>& sets);
protected:
	//note reutrn value
	//0:success
	//-1:hwdevice error.may change other codec
	//-2:error
	int initVideoCodecCtx(AVFormatContext* ctx, const AVCodec* codec, const AVCodecHWConfig* pConfig);
	AVCodecContext* getCodecCtx();
	bool init();
	bool initProperty();
	bool initHWFrame(AVCodecContext* codec_ctx);
	bool testCodecOfEncoderReal(const AVCodec* codec, const AVCodecHWConfig* pConfig);
	void freeCodecCtx();
	int convert(UPTR_FME& dst, UPTR_FME src);

protected:
	const AVCodec* m_codecVideo = nullptr;
	AVCodecContext* m_codecCtxVideo = nullptr;
	bool m_bFlushVideo = false;
	int m_iVideoMinBitRate = 0;
	int m_iVideoMaxBitRate = 0;
	int m_iWidth = 0;
	int m_iHeight = 0;
	AVPixelFormat m_reqEncodePixFmt = AV_PIX_FMT_NONE;
	AVPixelFormat m_feedPixFmt = AV_PIX_FMT_NONE;
	AVRational m_ratRatio;
	AVRational m_ratFrameRate;
	AVRational m_ratVideoTimeBase;
	AVHWDeviceType m_EncodeAVHWDeviceType = AV_HWDEVICE_TYPE_NONE;
	AVFrame* m_hw_frame = NULL;
	std::unique_ptr<std::once_flag>m_call_once;
	int64_t m_record_last_video_dts = AV_NOPTS_VALUE;
	AVCodecID m_video_id = AV_CODEC_ID_NONE;
	std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>m_hw_sets;
	imageConvert m_con;
};


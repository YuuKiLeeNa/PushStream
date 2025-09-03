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
#include "CAudioFrameCastTs.h"

class CAudioEncoder :virtual public CEncoderOpt
{
public:
	~CAudioEncoder();
	void reset();
	int send_Frame(UPTR_FME f, std::vector<UPTR_PKT>& sets);
	void setCorrectTs(bool b);
protected:
	//note reutrn value 
	//0:success
	//-1:may change other codec
	//-2:error
	int initAudioCodecCtx(AVFormatContext* ctx = NULL);
	AVCodecContext* getCodecCtx();
	bool init();
	bool initProperty();
	bool initSuitableAudioCodec(AVCodecContext* codec_ctx);
protected:
	const AVCodec* m_codecAudio = nullptr;
	AVCodecContext* m_codecCtxAudio = nullptr;
	bool m_bFlushAudio = false;
	int m_iAudioBitRate = 0;
	int m_iAudioSampleRate = 0;
	int m_iChannels = 0;
	uint64_t m_u64ChannelLayout = 0;
	AVSampleFormat m_sampleFmt = AV_SAMPLE_FMT_NONE;
	AVRational m_audioTimeBase;
	std::unique_ptr<std::once_flag>m_call_once;
	AVCodecID m_audio_id = AV_CODEC_ID_NONE;

	CAudioFrameCastTs m_cast;
};
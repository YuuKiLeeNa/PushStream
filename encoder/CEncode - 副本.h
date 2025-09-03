#pragma once
#include<string>
extern "C" {
#include "libavutil/rational.h"
#include "libavcodec/codec_id.h"
#include "libavcodec/codec.h"
#include "libavdevice/avdevice.h"
}
#include<map>
#include <list>
#include <memory>
#include <utility>
#include <vector>
#include<xcall_once.h>
#include "CMediaOption.h"
#include "define_type.h"

struct AVFormatContext;
struct AVCodec;
struct AVCodecContext;

class CEncode
{
public:
	enum STREAM_TYPE {ENCODE_VIDEO = 1,ENCODE_AUDIO = 1<<1};
	CEncode() { reset(); }
	~CEncode() { reset(); }
	void setFile(const std::string& pathFile, int flag = ENCODE_VIDEO | ENCODE_AUDIO);
	void reset();
	//bool send_Frame(AVFrame* f, CEncode::STREAM_TYPE type);
	bool send_Frame(UPTR_FME f, CEncode::STREAM_TYPE type);
	void setOption(const CMediaOption& opt);
	CMediaOption getOption();

	int getFlag() const { return m_flags; }
protected:
	//note reutrn value
	//0:success
	//-1:hwdevice error.may change other codec
	//-2:error
	int initVideoCodecCtxStream(AVFormatContext*&ctx, const AVCodec*codec, const AVCodecHWConfig*pConfig);

	int initAudioCodecCtxStream(AVFormatContext*&ctx);


	AVCodecContext* getEncCtx(CEncode::STREAM_TYPE type);
	bool init();
	bool initSuitableAudioCodec(AVCodecContext* codec_ctx);

	std::vector<UPTR_PKT>getPacket();
	void setStreamEndFlag(CEncode::STREAM_TYPE type);
	int writeTail();
	int writeTailIfAllStreamEnd();
	bool initHWFrame(AVCodecContext*codec_ctx);
protected:
	std::string m_file;
	AVFormatContext* m_fmtCtx = nullptr;
	const AVCodec* m_codecVideo = nullptr;
	const AVCodec* m_codecAudio = nullptr;
	AVCodecContext* m_codecCtxVideo = nullptr;
	AVCodecContext* m_codecCtxAudio = nullptr;
	//AVCodecContext* m_decodecCtxVideo = nullptr;
	//AVCodecContext* m_decodecCtxAudio = nullptr;
	//std::pair<const AVCodec*, const AVCodecHWConfig*>m_videoCodecConfig;
	AVStream* m_videoStream = nullptr;
	AVStream* m_audioStream = nullptr;
	
	int m_flags;
	//CAudioFrameCast m_CAudioFrameCast;
	//std::list<FRAME>m_listAudioFrame;
	//std::list<FRAME>m_listVideoFrame;

	std::list<UPTR_PKT>m_listAudioPacket;
	std::list<UPTR_PKT>m_listVideoPacket;

	bool m_bFlushAudio= false;
	bool m_bFlushVideo = false;
	//std::unique_ptr<CAudioFrameCast>m_audioCast;
	//bool m_bCheckNeedAudioCast = false;
	///////////////////////////////////////////////////
	int m_iVideoMinBitRate = 0;
	int m_iVideoMaxBitRate = 0;
	int m_iWidth = 0;
	int m_iHeight = 0;
	unsigned m_iNeedWriteTail = 0;//////1:video stream end         2:audio stream end       3:video and audio end
	//AVPixelFormat m_acceptFmt;
	//AVPixelFormat m_fmt;
	//AVPixelFormat m_decodeHwFmt;
	//AVPixelFormat m_decodeFmt; 
	AVPixelFormat m_encodeFmt = AV_PIX_FMT_NONE;

	AVRational m_ratRatio;
	AVRational m_ratFrameRate;
	AVRational m_ratVideoTimeBase;
	//AVBufferRef* m_pAVBufferRef = nullptr; 
	AVHWDeviceType m_EncodeAVHWDeviceType = AV_HWDEVICE_TYPE_NONE;
	//////////////////////////////////////////////////
	int m_iAudioBitRate = 0;
	int m_iAudioSampleRate = 0;
	int m_iChannels = 0;
	uint64_t m_u64ChannelLayout = 0;
	AVSampleFormat m_sampleFmt = AV_SAMPLE_FMT_NONE;
	AVRational m_audioTimeBase;
	AVFrame* m_hw_frame = NULL;
	std::unique_ptr<std::once_flag>m_call_once;
	int64_t m_record_last_video_dts = AV_NOPTS_VALUE;
	//int64_t m_iAudioSamples = 0;
	CMediaOption m_option;
	AVCodecID m_video_id = AV_CODEC_ID_NONE;
	AVCodecID m_audio_id = AV_CODEC_ID_NONE;
};
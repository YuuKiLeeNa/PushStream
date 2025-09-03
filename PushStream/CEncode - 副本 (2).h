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
#include "CAudioFrameCast.h"
#include <utility>
#include <vector>
#include<xcall_once.h>

struct AVFormatContext;
struct AVCodec;
struct AVCodecContext;

class CEncode
{
public:
	typedef struct REL_FRAME { void operator()(AVFrame*& f) { av_frame_unref(f); av_frame_free(&f);  } }REL_FRAME;
	typedef std::unique_ptr<AVFrame, REL_FRAME>FRAME;
	enum STREAM_TYPE {ENCODE_VIDEO = 1,ENCODE_AUDIO = 1<<1};
	CEncode() {}
	~CEncode() { reset(); }
	void setFile(const std::string& pathFile, int flag = ENCODE_VIDEO | ENCODE_AUDIO);
	void reset();
	bool send_Frame(AVFrame* f, CEncode::STREAM_TYPE type);
	bool send_Frame(FRAME f, CEncode::STREAM_TYPE type);
	void setVideoBitRate(int bitRate);
	void setAudioBitRate(int bitRate);
	void setAudioInfo(int sampleRate,int channels, uint64_t channelLayout,AVSampleFormat fmt);
	void setVideoInfo(AVHWDeviceType decodeHwDeviceType,AVBufferRef*pAVBufferRef, int iWidth, int iHeight, AVPixelFormat decodeHwFmt, AVPixelFormat decodeFmt,AVPixelFormat encodeFmt, const AVRational&sar, const AVRational&ratFrameRate, const AVRational&ratTimebase);
	void setHWCodecAndHWConfOfVideo(const std::pair<const AVCodec*, const AVCodecHWConfig*>&pairVideo);
	void setVideoWidthHeight(int width, int height);
	//void setFrameRate(const AVRational& frameRate);
	//void setDecodecCtxVideo(AVCodecContext*videoCtx,AVCodecContext*audioCtx);

protected:
	AVCodecContext* getEncCtx(CEncode::STREAM_TYPE type);
	bool init();
	void initSuitableAudioCodec(AVCodecContext* codec_ctx);
	bool pushFrame(AVFrame* f, CEncode::STREAM_TYPE type);
	std::vector<std::pair<FRAME, AVCodecContext*>>getFrames();
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
	std::pair<const AVCodec*, const AVCodecHWConfig*>m_videoCodecConfig;
	AVStream* m_videoStream = nullptr;
	AVStream* m_audioStream = nullptr;
	
	int m_flags;
	//CAudioFrameCast m_CAudioFrameCast;
	std::list<FRAME>m_listAudioFrame;
	std::list<FRAME>m_listVideoFrame;
	
	bool m_bFlushAudio= true;
	bool m_bFlushVideo = true;
	///////////////////////////////////////////////////
	int m_iVideoBitRate = 0;
	int m_iWidth;
	int m_iHeight;
	unsigned m_iNeedWriteTail = 0;//////1:video stream end         2:audio stream end       3:video and audio end
	//AVPixelFormat m_acceptFmt;
	//AVPixelFormat m_fmt;
	AVPixelFormat m_decodeHwFmt;
	AVPixelFormat m_decodeFmt; 
	AVPixelFormat m_encodeFmt;

	AVRational m_ratRatio;
	AVRational m_ratFrameRate;
	AVRational m_ratTimeBase;
	AVBufferRef* m_pAVBufferRef = nullptr; 
	AVHWDeviceType m_decodeAVHWDeviceType;
	//////////////////////////////////////////////////
	int m_iAudioBitRate = 0;
	int m_iAudioSampleRate;
	int m_iChannels;
	uint64_t m_u64ChannelLayout;
	AVSampleFormat m_sampleFmt;
	AVRational m_audioTimeBase;
	AVFrame* m_hw_frame = NULL;
	std::unique_ptr<std::once_flag>m_call_once;
	int64_t m_record_last_video_dts = AV_NOPTS_VALUE;
	//int64_t m_iAudioSamples = 0;
};
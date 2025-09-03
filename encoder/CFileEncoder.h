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
#include "Pushstream/define_type.h"
#include "CAudioEncoder.h"
#include "CVideoEncoder.h"




struct AVFormatContext;
struct AVCodec;
struct AVCodecContext;

class CFileEncoder:protected CAudioEncoder, protected CVideoEncoder
{
public:
	enum STREAM_TYPE {ENCODE_VIDEO = 1,ENCODE_AUDIO = 1<<1};
	CFileEncoder() { reset(); }
	CFileEncoder(const CFileEncoder&) = delete;
	CFileEncoder& operator=(const CFileEncoder&) = delete;

	~CFileEncoder() { reset(); }
	void setFile(const std::string& pathFile, int flag = ENCODE_VIDEO | ENCODE_AUDIO);
	void reset();
	bool send_Frame(UPTR_FME f, CFileEncoder::STREAM_TYPE type);

	using CVideoEncoder::setOption;
	using CVideoEncoder::getOption;

	int getFlag() const { return m_flags; }
protected:
	//note reutrn value
	//0:success
	//-1:hwdevice error.may change other codec
	//-2:error
	int initVideoCodecCtxStream(AVFormatContext*&ctx, const AVCodec*codec, const AVCodecHWConfig*pConfig);
	int initAudioCodecCtxStream(AVFormatContext*&ctx);

	AVCodecContext* getEncCtx(CFileEncoder::STREAM_TYPE type);
	bool init();


	std::vector<UPTR_PKT>getPacket();
	void setStreamEndFlag(CFileEncoder::STREAM_TYPE type);
	int writeTail();
	int writeTailIfAllStreamEnd();
	
protected:
	std::string m_file;
	AVFormatContext* m_fmtCtx = nullptr;
	AVStream* m_videoStream = nullptr;
	AVStream* m_audioStream = nullptr;
	int m_flags;
	std::list<UPTR_PKT>m_listAudioPacket;
	std::list<UPTR_PKT>m_listVideoPacket;
	unsigned m_iNeedWriteTail = 0;//////1:video stream end         2:audio stream end       3:video and audio end
	std::unique_ptr<std::once_flag>m_fileEncoderInitCallOnce;
};
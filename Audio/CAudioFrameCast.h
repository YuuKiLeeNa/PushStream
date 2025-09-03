#pragma once
#include<map>
#include<vector>
#include<utility>
#include<stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/samplefmt.h"
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif
#include "media-io-defs.h"
#include "PushStream/define_type.h"


//#define RESAMPLER_TIMESTAMP_SMOOTHING_THRESHOLD 70000000ULL
#define AUDIO_MAX_SAMPLERS_COUNT_IN_SECONDS		60


struct AVFrame;
struct AVCodecContext;
struct AVAudioFifo;
struct SwrContext;

class CAudioFrameCast 
{
public:
	CAudioFrameCast();
	~CAudioFrameCast();
	void reset();
	bool translate(const uint8_t* const* data, int iSamples,uint64_t* ts_offset = NULL);
	bool translate(UPTR_FME f,uint64_t* ts_offset = NULL);//, AVCodecContext* decContext,AVCodecContext*encContext);
	UPTR_FME getFrame(int iSamples);
	bool audio_resampler_resample(uint8_t* output[], uint32_t* out_frames, uint64_t* ts_offset, const uint8_t* const input[], uint32_t in_frames);
	uint8_t** getArray(uint8_t*(&p)[MAX_AV_PLANES], int iSamples);
	int getSamplesNum() const;
	bool set(int iInChannels, AVSampleFormat inFormat, int iInSampleRate, int iOutChannels, AVSampleFormat outFormat, int iOutSampleRate);
	bool set(uint64_t InChannelLayout, AVSampleFormat inFormat, int iInSampleRate, uint64_t OutChannelLayout, AVSampleFormat outFormat, int iOutSampleRate);
	bool set(AVChannelLayout*InChannelLayout, AVSampleFormat inFormat, int iInSampleRate, AVChannelLayout*OutChannelLayout, AVSampleFormat outFormat, int iOutSampleRate);
	
	inline AVSampleFormat getInputFormat()const { return m_inFormat; }
	inline AVSampleFormat getOutputFormat() const { return m_outFormat; }
	inline int getInputSampleRate() const {return m_iInSampleRate;}
	inline int getOutputSampleRate() const {return m_iOutSampleRate;}
	inline uint64_t getInputChannelLayout() const { return m_InChannelLayout.u.mask; }
	inline uint64_t getOutputChannelLayout() const { return m_OutChannelLayout.u.mask; }

	operator bool() 
	{
		return m_SwrContext != NULL;
	}

protected:
	//int64_t getExpectIncomingFramePts();
	

protected:
	AVChannelLayout m_InChannelLayout;
	AVSampleFormat m_inFormat;
	int m_iInSampleRate;
	//int m_iInChannels;

	AVChannelLayout m_OutChannelLayout;
	AVSampleFormat m_outFormat;
	int m_iOutSampleRate;
	//int m_iOutChannels;

	SwrContext* m_SwrContext = nullptr;
	AVAudioFifo* m_fifo = nullptr;

	//time stamp in ns
	//int64_t m_pts = 0;
	//int64_t m_pts_diff = 0;
	//int64_t m_accuSamples = 0;

};



#pragma once
#include "rnnoiseDenoise.h"
#include<vector>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/samplefmt.h"
#ifdef __cplusplus
}
#endif
#include "CAudioFrameCast.h"
#include<vector>
#include "PushStream/define_type.h"
#include "media-io-defs.h"
#include "speexWrap/speexDenoise.h"
#include<bitset>
#include<mutex>
#include<variant>
#include<atomic>

class rnnoiseHelp
{
public:
	typedef std::variant<float, int, unsigned>VARIANT;

	enum VOICE_TYPE {RNNOISE_VOICE_TYPE = 0
		, SPEEX_AEC_VOICE_TYPE
		, SPEEX_AES_VOICE_TYPE
		, SPEEX_VAD_VOICE_TYPE
		, SPEEX_AGC_VOICE_TYPE
		, NUM_VOICE_TYPE
	};
	bool setAudioInfo(uint64_t InChannelLayout, AVSampleFormat inFormat, int iInSampleRate, uint64_t OutChannelLayout, AVSampleFormat outFormat, int iOutSampleRate);
	
	UPTR_FME translate(UPTR_FME f, uint64_t* ts_offset = NULL);
	UPTR_FME translate(uint8_t*d[MAX_AV_PLANES], int samples, uint64_t* ts_offset = NULL);
	bool setOption(VOICE_TYPE opt, bool b, VARIANT ant);
	bool getOption(VOICE_TYPE opt, bool& b, VARIANT* ant);
	bool setOnRnnModel();
	bool setOnRnnModel_VAD_AGC();
	bool setOnSpeexModel();
protected:
	std::vector<rnnoiseDenoise>m_vecDenoise;
	std::vector<speexDenoise>m_vecSpeex;

	CAudioFrameCast m_castToInternal;
	CAudioFrameCast m_castOutput;
	const AVSampleFormat m_internalFormat = AV_SAMPLE_FMT_S16P;
	//match rnnoise_get_frame_size size 
	const int m_internalSampleRate = 48000;
	std::bitset<NUM_VOICE_TYPE>m_bitset;
	std::mutex m_mutex;
};



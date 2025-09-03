#include "speexDenoise.h"



speexDenoise::speexDenoise()
{
	//m_st = speex_preprocess_state_init(m_iResampleRate,m_frame_size);
	create();
}

speexDenoise::speexDenoise(int sampleRate, int frame_size):m_iResampleRate(sampleRate), m_frame_size(frame_size)
{
	//m_st = speex_preprocess_state_init(m_iResampleRate, m_frame_size);
	create();
}

speexDenoise::~speexDenoise()
{
	destroy();
}

bool speexDenoise::translate(short* in, bool* bIsSpeechIfOnVad)
{
	//@return Bool value for voice activity (1 for speech, 0 for noise/silence), ONLY if VAD turned on.
	int vad = speex_preprocess_run(m_st, in);
	if (bIsSpeechIfOnVad && m_bIsOnVad)
		*bIsSpeechIfOnVad = vad == 1 ? true : false;
	return true;
}

bool speexDenoise::create()
{
	destroy();
	m_st = speex_preprocess_state_init(m_frame_size,m_iResampleRate);
	setVAD(false);
	setAGCLevel(false,8000);
	setAESLevel(false, 0);
	setNoiseSuppress(false, 0);

	return m_st != 0;
}

bool speexDenoise::create(int sampleRate, int frame_size)
{
	m_iResampleRate = sampleRate;
	m_frame_size = frame_size;
	return create();
}

void speexDenoise::destroy()
{
	if (m_st)
	{
		speex_preprocess_state_destroy(m_st);
		m_st = 0;
	}
}

speexDenoise::speexDenoise(speexDenoise&& obj) noexcept:m_iResampleRate(obj.m_iResampleRate), m_frame_size(obj.m_frame_size), m_st(obj.m_st)
{
	if (this == &obj)
		return;
	obj.m_st = 0;
}

speexDenoise& speexDenoise::operator=(speexDenoise&& obj) noexcept
{
	if (this == &obj)
		return *this;
	m_iResampleRate = obj.m_iResampleRate;
	m_frame_size = obj.m_frame_size;
	m_st = obj.m_st;
	m_st = 0;
	return (*this);
}

speexDenoise::operator bool()
{
	return m_st != 0;
}

int speexDenoise::frameSize()
{
	return m_frame_size;
}

int speexDenoise::sampleRate()
{
	return m_iResampleRate;
}

bool speexDenoise::setNoiseSuppress(bool b, int negative_db)
{
	return setParam<SPEEX_PREPROCESS_GET_DENOISE,SPEEX_PREPROCESS_SET_DENOISE, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, int>(b, negative_db);
	/*if (!m_st || negative_db >= 0)
		return false;
	int i,ret;
	ret = speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_GET_DENOISE, &i);
	if (ret != 0)
		return false;
	if (b) 
	{
		if (i != 1)
		{
			i = 1;
			ret = speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_DENOISE, &i);
			if (ret != 0)
				return false;
		}
		ret = speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &negative_db);
		if (ret != 0)
			return false;
	}
	else
	{
		if (i == 2)
			return true;
		i = 2;
		ret = speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_DENOISE, &i);
		if (ret != 0)
			return false;
	}
	return true;*/
}

bool speexDenoise::setNoiseSuppress(int negative_db)
{

	return setNoiseSuppress(true, negative_db);
}

bool speexDenoise::getNoiseSuppress(bool& b, int* negative_db)
{
	return getParam<SPEEX_PREPROCESS_GET_DENOISE, SPEEX_PREPROCESS_GET_NOISE_SUPPRESS, int>(b, negative_db);
}

bool speexDenoise::setAGCLevel(bool b, float gain_level)
{
	return setParam<SPEEX_PREPROCESS_GET_AGC,SPEEX_PREPROCESS_SET_AGC, SPEEX_PREPROCESS_SET_AGC_LEVEL, float>(b, gain_level);
}

bool speexDenoise::setAGCLevel(float gain_level)
{
	return setAGCLevel(true, gain_level);
}

bool speexDenoise::getAGCLevel(bool& b, float* gain_level)
{
	return getParam<SPEEX_PREPROCESS_GET_AGC, SPEEX_PREPROCESS_GET_AGC_LEVEL, float>(b, gain_level);
}

bool speexDenoise::setAESLevel(bool b, float db)
{
	return setParam<SPEEX_PREPROCESS_GET_DEREVERB,SPEEX_PREPROCESS_SET_DEREVERB, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, float>(b, db);
}

bool speexDenoise::setAESLevel(float db)
{
	return setAESLevel(true, db);
}

bool speexDenoise::getAESLevel(bool& b, float* db)
{
	return getParam<SPEEX_PREPROCESS_GET_DEREVERB, SPEEX_PREPROCESS_GET_DEREVERB_LEVEL, float>(b, db);
}

bool speexDenoise::setVAD(bool b)
{
	bool ret = setOption<SPEEX_PREPROCESS_GET_VAD, SPEEX_PREPROCESS_SET_VAD>(b);
	if (ret)
		m_bIsOnVad = b;
	return ret;
	/*if (!m_st)
		return false;
	int i, ret;
	ret = speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_GET_VAD, &i);
	if (ret != 0)
		return false;

	if (b) 
	{
		if (i != 1) 
		{
			i = 1;
			ret = speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_VAD, &i);
			if (ret != 0)
				return false;
		}
	}
	else 
	{
		if (i != 2) 
		{
			i = 2;
			ret = speex_preprocess_ctl(m_st, SPEEX_PREPROCESS_SET_VAD, &i);
			if (ret != 0)
				return false;
		}
	}
	return true;*/
}

bool speexDenoise::getVAD(bool& b)
{
	return getParam<SPEEX_PREPROCESS_GET_VAD>(b);
}

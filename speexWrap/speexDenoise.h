#pragma once
#include "speex/speex_preprocess.h"


class speexDenoise
{
public:
	speexDenoise();
	speexDenoise(int sampleRate, int frame_size);
	~speexDenoise();
	bool translate(short* in, bool *bIsSpeechIfOnVad = 0);
	bool create();
	bool create(int sampleRate, int frame_size);
	void destroy();
	speexDenoise(const speexDenoise&) = delete;
	speexDenoise(speexDenoise&&obj)noexcept;
	speexDenoise& operator=(const speexDenoise&) = delete;
	speexDenoise& operator=(speexDenoise&& obj)noexcept;
	operator bool();
	int frameSize();
	int sampleRate();

	bool setNoiseSuppress(bool b, int negative_db);
	bool setNoiseSuppress(int negative_db);
	bool getNoiseSuppress(bool &b, int*negative_db);

	/// 8000-80000
	bool setAGCLevel(bool b, float gain_level);
	bool setAGCLevel(float gain_level);
	bool getAGCLevel(bool &b, float*gain_level);


	bool setAESLevel(bool b, float db);
	bool setAESLevel(float db);
	bool getAESLevel(bool& b, float* db);


	bool setVAD(bool b);
	bool getVAD(bool&b);

protected:
	template<int GET_OPTION,int OPTION, int CHILD_OPTION, typename PARAM_TYPE>
	bool setParam(bool b, typename PARAM_TYPE value) 
	{
		if (!m_st)
			return false;
		int i, ret;
		ret = speex_preprocess_ctl(m_st, GET_OPTION, &i);
		if (ret != 0)
			return false;
		if (b)
		{
			if (i != 1)
			{
				i = 1;
				ret = speex_preprocess_ctl(m_st, OPTION, &i);
				if (ret != 0)
					return false;
			}
			ret = speex_preprocess_ctl(m_st, CHILD_OPTION, &value);
			if (ret != 0)
				return false;
		}
		else
		{
			if (i == 2)
				return true;
			i = 2;
			ret = speex_preprocess_ctl(m_st, OPTION, &i);
			if (ret != 0)
				return false;
		}
		return true;
	}

	template<int GET_OPTION, int OPTION>
	bool setOption(bool b)
	{
		if (!m_st)
			return false;
		int i, ret;
		ret = speex_preprocess_ctl(m_st, GET_OPTION, &i);
		if (ret != 0)
			return false;

		if (b)
		{
			if (i != 1)
			{
				i = 1;
				ret = speex_preprocess_ctl(m_st, OPTION, &i);
				if (ret != 0)
					return false;
			}
		}
		else
		{
			if (i != 0)
			{
				i = 0;
				ret = speex_preprocess_ctl(m_st, OPTION, &i);
				if (ret != 0)
					return false;
			}
		}
		return true;
	}

	template<int GET_OPTION, int GET_CHILD_OPTION, typename PARAM_TYPE>
	bool getParam(bool& b, typename PARAM_TYPE* value)
	{
		if (!m_st)
			return false;
		int i, ret;
		ret = speex_preprocess_ctl(m_st, GET_OPTION, &i);
		if (ret != 0)
			return false;
		b = i == 1 ? true : false;
		if (b && value)
		{
			ret = speex_preprocess_ctl(m_st, GET_CHILD_OPTION, value);
			if (ret != 0)
				return false;
		}
		return true;
	}

	template<int GET_OPTION>
	bool getParam(bool& b)
	{
		if (!m_st)
			return false;
		int i, ret;
		ret = speex_preprocess_ctl(m_st, GET_OPTION, &i);
		if (ret != 0)
			return false;
		b = i == 1 ? true : false;
		return true;
	}


protected:
	SpeexPreprocessState* m_st = 0;
	int m_iResampleRate = 48000;
	int m_frame_size = 480;

	bool m_bIsOnVad;
};



#include "rnnoiseHelp.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif

#include "Util/logger.h"
#include "Network/Socket.h"


bool rnnoiseHelp::setAudioInfo(uint64_t InChannelLayout, AVSampleFormat inFormat, int iInSampleRate, uint64_t OutChannelLayout, AVSampleFormat outFormat, int iOutSampleRate)
{
	bool b = m_castToInternal.set(InChannelLayout, inFormat, iInSampleRate, InChannelLayout, m_internalFormat, m_internalSampleRate);
	if (!b)
		return false;

	AVChannelLayout inchlay;
	int err = av_channel_layout_from_mask(&inchlay, InChannelLayout);
	if (err != 0)
		return false;
	{
		std::lock_guard<std::mutex>m(m_mutex);
		m_vecDenoise.clear();
		m_vecDenoise.resize(inchlay.nb_channels);
		m_vecSpeex.clear();
		m_vecSpeex.reserve(inchlay.nb_channels);
		for (int i = 0; i < inchlay.nb_channels; ++i)
			m_vecSpeex.emplace_back(m_internalSampleRate, rnnoiseDenoise::s_frame_size);
		m_bitset.reset();
	}

	b = m_castOutput.set(InChannelLayout, m_internalFormat, m_internalSampleRate, OutChannelLayout, outFormat, iOutSampleRate);
	if (!b)
		return false;

	return true;
}

UPTR_FME rnnoiseHelp::translate(UPTR_FME f, uint64_t* ts_offset)
{
	return translate(f->data, f->nb_samples, ts_offset);
}

UPTR_FME rnnoiseHelp::translate(uint8_t* d[MAX_AV_PLANES], int samples, uint64_t* ts_offset)
{
	uint64_t offset_internal, offset_output;
	if (!m_castToInternal.translate(d, samples, ts_offset ? &offset_internal:NULL))
	{
		PrintE("m_castToInternal.translate error");
		return NULL;
	}

	if (ts_offset)
	{
		if (!m_castOutput.translate(NULL, 0, &offset_output))
		{
			PrintE("get audio noise output offset failed");
			return NULL;
		}
		*ts_offset = offset_internal + offset_output;
	}

	while (m_castToInternal.getSamplesNum() >= rnnoiseDenoise::s_frame_size) 
	{
		auto frame = m_castToInternal.getFrame(rnnoiseDenoise::s_frame_size);

		{
			std::lock_guard<std::mutex>m(m_mutex);
			for (int ch = 0; ch < frame->ch_layout.nb_channels; ++ch)
			{
				if (m_bitset.test(RNNOISE_VOICE_TYPE))
					if (!m_vecDenoise[ch].translate((const short*)frame->data[ch], (short*)frame->data[ch]))
					{
						PrintE("m_vecDenoise[ch].translate error");
						//return NULL;
					}
				if (m_bitset.test(SPEEX_AEC_VOICE_TYPE)
					|| m_bitset.test(SPEEX_AES_VOICE_TYPE)
					|| m_bitset.test(SPEEX_VAD_VOICE_TYPE)
					|| m_bitset.test(SPEEX_AGC_VOICE_TYPE))
				{
					bool bIsSpeech = false;
					if (!m_vecSpeex[ch].translate((short*)frame->data[ch], &bIsSpeech))
					{
						PrintE("m_vecSpeex[ch].translate error");
						//return NULL;
					}
					/*if (m_bitset.test(SPEEX_VAD_VOICE_TYPE)) 
					{
						PrintD("%s", bIsSpeech ? "speech":"noise");
					}*/
				}
			}
		}

		if (!m_castOutput.translate(frame->data, frame->nb_samples))
		{
			PrintE("m_castToInternal.translate error");
			return NULL;
		}
	}
	
	int out_samples = m_castOutput.getSamplesNum();
	if (out_samples <= 0)
		return NULL;
	return m_castOutput.getFrame(out_samples);
}

bool rnnoiseHelp::setOption(VOICE_TYPE opt, bool b, VARIANT ant)
{
	bool ret = true;
	std::lock_guard<std::mutex>lock(m_mutex);
	switch (opt)
	{
	case rnnoiseHelp::RNNOISE_VOICE_TYPE:
		m_bitset.set(opt, b);
		break;
	case rnnoiseHelp::SPEEX_AEC_VOICE_TYPE:
	{
		for (auto& ele : m_vecSpeex)
		{
			ret = ele.setNoiseSuppress(b, std::get<1>(ant));
			if (!ret)
			{
				PrintE("speex setNoiseSuppress error");
				break;
			}
		}
		if (ret)
			m_bitset.set(opt, b);
	}
		break;
	case rnnoiseHelp::SPEEX_AES_VOICE_TYPE:
	{
		for (auto& ele : m_vecSpeex)
		{
			ret = ele.setAESLevel(b, std::get<0>(ant));
			if (!ret)
			{
				PrintE("speex setAESLevel error");
				break;
			}
		}
		if (ret)
			m_bitset.set(opt, b);
	}
		break;
	case rnnoiseHelp::SPEEX_VAD_VOICE_TYPE:
	{
		for (auto& ele : m_vecSpeex)
		{
			ret = ele.setVAD(b);
			if (!ret)
			{
				PrintE("speex setVAD error");
				break;
			}
		}
		if (ret)
			m_bitset.set(opt, b);
	}
		break;
	case rnnoiseHelp::SPEEX_AGC_VOICE_TYPE:
	{
		for (auto& ele : m_vecSpeex)
		{
			ret = ele.setAGCLevel(b, std::get<0>(ant));
			if (!ret)
			{
				PrintE("speex setAGCLevel error");
				break;
			}
		}
		if (ret)
			m_bitset.set(opt, b);
	}
		break;
	default:
		break;
	}
	return ret;
}

bool rnnoiseHelp::getOption(VOICE_TYPE opt, bool& b, VARIANT* ant)
{
	std::lock_guard<std::mutex>lock(m_mutex);
	switch (opt)
	{
	case rnnoiseHelp::RNNOISE_VOICE_TYPE:
		b = m_bitset.test(opt);
		break;
	case rnnoiseHelp::SPEEX_AEC_VOICE_TYPE:
	{
		if (m_vecSpeex.empty())
			return false;
		int v;
		if (!m_vecSpeex[0].getNoiseSuppress(b, &v)) 
		{
			return false;
		}
		if (ant)
			*ant = v;
		return true;
	}
	break;
	case rnnoiseHelp::SPEEX_AES_VOICE_TYPE:
	{
		if (m_vecSpeex.empty())
			return false;
		float v;
		if (!m_vecSpeex[0].getAESLevel(b, &v))
		{
			return false;
		}
		if (ant)
			*ant = v;
		return true;
	}
	break;
	case rnnoiseHelp::SPEEX_VAD_VOICE_TYPE:
	{
		if (m_vecSpeex.empty())
			return false;
		if (!m_vecSpeex[0].getVAD(b))
			return false;
		return true;
	}
	break;
	case rnnoiseHelp::SPEEX_AGC_VOICE_TYPE:
	{
		if (m_vecSpeex.empty())
			return false;
		float v;
		if (!m_vecSpeex[0].getAGCLevel(b, &v))
		{
			return false;
		}
		if (ant)
			*ant = v;
		return true;
	}
	break;
	default:
		break;
	}
	return false;
}

bool rnnoiseHelp::setOnRnnModel() 
{
	bool ret = true;
	std::lock_guard<std::mutex>lock(m_mutex);
	m_bitset.set(RNNOISE_VOICE_TYPE, true);

	for (auto& ele : m_vecSpeex)
	{
		ret = ele.setNoiseSuppress(false, 0);
		//ret = ele.setNoiseSuppress(true, 0);
		if (!ret)
		{
			PrintE("speex setNoiseSuppress error");
			break;
		}
		ret = ele.setAESLevel(false, 0.0f);
		//ret = ele.setAESLevel(true, 0.0f);
		if (!ret)
		{
			PrintE("speex setAESLevel error");
			break;
		}
		ret = ele.setVAD(false);
		if (!ret)
		{
			PrintE("speex setVAD error");
			break;
		}
		ret = ele.setAGCLevel(false, 80000.0);
		if (!ret)
		{
			PrintE("speex setAGCLevel error");
			break;
		}
	}
	if (ret)
	{
		m_bitset.set(SPEEX_AEC_VOICE_TYPE, false);
		m_bitset.set(SPEEX_AES_VOICE_TYPE, false);
		m_bitset.set(SPEEX_VAD_VOICE_TYPE, false);
		m_bitset.set(SPEEX_AGC_VOICE_TYPE, false);
	}
	return ret;
}

bool rnnoiseHelp::setOnRnnModel_VAD_AGC()
{
	bool ret = true;
	std::lock_guard<std::mutex>lock(m_mutex);
	m_bitset.set(RNNOISE_VOICE_TYPE, true);

	for (auto& ele : m_vecSpeex)
	{
		ret = ele.setNoiseSuppress(false, 0);
		//ret = ele.setNoiseSuppress(true, 0);
		if (!ret)
		{
			PrintE("speex setNoiseSuppress error");
			break;
		}
		ret = ele.setAESLevel(false, 0.0f);
		//ret = ele.setAESLevel(true, 0.0f);
		if (!ret)
		{
			PrintE("speex setAESLevel error");
			break;
		}
		ret = ele.setVAD(true);
		if (!ret)
		{
			PrintE("speex setVAD error");
			break;
		}
		ret = ele.setAGCLevel(true, 80000.0);
		if (!ret)
		{
			PrintE("speex setAGCLevel error");
			break;
		}
	}
	if (ret) 
	{
		m_bitset.set(SPEEX_AEC_VOICE_TYPE, false);
		m_bitset.set(SPEEX_AES_VOICE_TYPE, false);
		m_bitset.set(SPEEX_VAD_VOICE_TYPE, true);
		m_bitset.set(SPEEX_AGC_VOICE_TYPE, true);
	}
	return ret;
}

bool rnnoiseHelp::setOnSpeexModel() 
{
	bool ret = true;
	std::lock_guard<std::mutex>lock(m_mutex);
	m_bitset.set(RNNOISE_VOICE_TYPE, false);

	for (auto& ele : m_vecSpeex)
	{
		ret = ele.setNoiseSuppress(true, -25);
		if (!ret)
		{
			PrintE("speex setNoiseSuppress error");
			break;
		}
		ret = ele.setAESLevel(true, 25.0f);
		if (!ret)
		{
			PrintE("speex setAESLevel error");
			break;
		}
		ret = ele.setVAD(true);
		if (!ret)
		{
			PrintE("speex setVAD error");
			break;
		}
		ret = ele.setAGCLevel(true, 8000);
		if (!ret)
		{
			PrintE("speex setAGCLevel error");
			break;
		}
	}
	if (ret)
	{
		m_bitset.set(SPEEX_AEC_VOICE_TYPE, true);
		m_bitset.set(SPEEX_AES_VOICE_TYPE, true);
		m_bitset.set(SPEEX_VAD_VOICE_TYPE, true);
		m_bitset.set(SPEEX_AGC_VOICE_TYPE, true);
	}
	return ret;
}

#include "CAudioFrameCast.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/audio_fifo.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include <assert.h>
#include "libavutil/error.h"
#ifdef __cplusplus
}
#endif
#include<memory>
#include<functional>
#include "Util/logger.h"
#include "Network/Socket.h"

#include "util_uint64.h"



CAudioFrameCast::CAudioFrameCast()
{
	reset();
}

CAudioFrameCast::~CAudioFrameCast() 
{
	reset();
}

void CAudioFrameCast::reset()
{
	if (m_SwrContext)
	{
		swr_free(&m_SwrContext);
		m_SwrContext = nullptr;
	}
	if (m_fifo)
	{
		av_audio_fifo_free(m_fifo);
		m_fifo = nullptr;
	}

	av_channel_layout_uninit(&m_InChannelLayout);
	m_inFormat = AV_SAMPLE_FMT_NONE;
	m_iInSampleRate = 0;

	av_channel_layout_uninit(&m_OutChannelLayout);
	m_outFormat = AV_SAMPLE_FMT_NONE;
	m_iOutSampleRate = 0;

}

bool CAudioFrameCast::translate(const uint8_t* const* data, int iSamples,uint64_t* ts_offset)
{
	if (!m_fifo)
		return false;
	if (ts_offset)
	{
		uint64_t offset_swrontext = (uint64_t)swr_get_delay(m_SwrContext, 1000000000);
		int iFifoSamples = av_audio_fifo_size(m_fifo);
		uint64_t offset_fifo = util_mul_div64(iFifoSamples, 1000000000, m_iOutSampleRate);
		*ts_offset = offset_swrontext + offset_fifo;
	}
	if (!data || iSamples == 0)
		return true;

	int64_t nb_samples = av_rescale_q_rnd(iSamples + swr_get_delay(m_SwrContext, m_iInSampleRate), { 1, m_iInSampleRate }, { 1, m_iOutSampleRate }, AVRounding::AV_ROUND_UP);
	//uint8_t** pBuff = (uint8_t**)calloc(m_InChannelLayout.nb_channels, nb_samples);

	int iSpaceSamples = av_audio_fifo_space(m_fifo);
	if (iSpaceSamples < nb_samples)
	{
		PrintE("CAudioFrameCast no space for samplers(%d):%d", iSpaceSamples, nb_samples);
		return false;
	}

	//uint8_t** pptr = (uint8_t**)calloc(m_InChannelLayout.nb_channels*sizeof(uint8_t*), 1);
	uint8_t* pptr[MAX_AV_PLANES];

	if (pptr == nullptr)
		return false;
	int ret = av_samples_alloc(pptr, nullptr, m_OutChannelLayout.nb_channels, nb_samples, m_outFormat, 16);
	assert(ret == av_samples_get_buffer_size(nullptr, m_OutChannelLayout.nb_channels, nb_samples, m_outFormat, 16));

	if (ret <= 0)
	{
		free(pptr);
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_samples_get_buffer_size error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	std::unique_ptr<uint8_t*, std::function<void(uint8_t**)>>freepBuf(pptr, [](uint8_t** p)
		{
			av_freep(&p[0]);
			//free(p);
		});

	int numsSamples = 0;
	if ((numsSamples = swr_convert(m_SwrContext, pptr, nb_samples, (const uint8_t**)data, iSamples)) < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("swr_convert error:%s", av_make_error_string(szErr, sizeof(szErr), numsSamples));
		return false;
	}
	//assert(numsSamples == nb_samples);

	if ((ret = av_audio_fifo_write(m_fifo, (void**)pptr, numsSamples)) < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_audio_fifo_write error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}

	/*if ((ret = av_audio_fifo_realloc(m_fifo, numsSamples + av_audio_fifo_size(m_fifo))) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_audio_fifo_realloc error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}*/

	//int64_t exptectTimePtsInNS = getExpectIncomingFramePts();
	//int64_t timePtsInNS = util_mul_div64(pts, 1000000000, m_iInSampleRate);
	//m_pts_diff += timePtsInNS - exptectTimePtsInNS;


	/*if (RESAMPLER_TIMESTAMP_SMOOTHING_THRESHOLD <= std::abs(m_pts_diff))
	{
		PrintE("CAudioFrameCast incoming pts exceeds RESAMPLER_TIMESTAMP_SMOOTHING_THRESHOLD %lld", m_pts_diff);
		int64_t offset = util_mul_div64(m_pts_diff, m_iOutSampleRate, 1000000000);
		if (offset + numsSamples > iSpaceSamples)
		{
			PrintE("CAudioFrameCast(%d) no space for writing(%lld)", iSpaceSamples, offset + numsSamples);
			return false;
		}
		if (offset > 0) 
		{
			uint8_t* p[AV_NUM_DATA_POINTERS];
			ret = av_samples_alloc(p, NULL, m_OutChannelLayout.nb_channels, offset, m_outFormat, 0);
			if (ret < 0) 
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_samples_alloc error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
				return false;
			}
			ret = av_samples_set_silence(p, 0, offset, m_OutChannelLayout.nb_channels, m_outFormat);
			if (ret < 0)
			{
				av_freep(&p[0]);
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_samples_set_silence error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
				return false;
			}
			if ((ret = av_audio_fifo_write(m_fifo, (void**)p, offset)) < 0)
			{
				av_freep(&p[0]);
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_audio_fifo_write error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return false;
			}
			av_freep(&p[0]);
			if ((ret = av_audio_fifo_write(m_fifo, (void**)pptr, numsSamples)) < 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_audio_fifo_write error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return false;
			}
		}
		else 
		{
			int iReadableSize = av_audio_fifo_size(m_fifo);
			if (-offset < iReadableSize) 
			{
				if ((ret = av_audio_fifo_drain(m_fifo, -offset)) < 0) 
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_audio_fifo_drain error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return false;
				}
			}
			else 
			{
				if ((ret = av_audio_fifo_drain(m_fifo, iReadableSize)) < 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_audio_fifo_drain error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return false;
				}


				
			}

		}
	}*/

	/*if (iSpaceSamples >= numsSamples)
	{
		if ((ret = av_audio_fifo_write(m_fifo, (void**)pptr, numsSamples)) < 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_audio_fifo_write error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return false;
		}
	}
	else 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("CAudioFrameCast no space for samplers(%d):%d", iSpaceSamples, numsSamples);
		return false;
	}*/
	return true;
}

bool CAudioFrameCast::translate(UPTR_FME f, uint64_t* ts_offset)//, AVCodecContext* decContext, AVCodecContext* encContext)
{
	/*if (!m_fifo)
		return false;
	if (!f)
		return true;*/
	//UPTR_FME free_frame(f);
	return translate(f->data, f->nb_samples, ts_offset/*, pts*/);
}

UPTR_FME CAudioFrameCast::getFrame(int iSamples)
{
	int fifoSize = av_audio_fifo_size(m_fifo);
	if(fifoSize > 0 && fifoSize >= iSamples)
	{
		AVFrame* tmp = av_frame_alloc();
		UPTR_FME free_frame_tmp(tmp);
		/*std::unique_ptr<AVFrame, std::function<void(AVFrame*&)>>free_frame_tmp(tmp, [](AVFrame*& f)
			{
				av_frame_unref(f);
				av_frame_free(&f);
			});*/
		//Tmp->channels = m_iOutChannels;
		//tmp->channel_layout = m_OutChannelLayout;
		//tmp->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
		//tmp->ch_layout.nb_channels = m_iOutChannels;
		tmp->ch_layout = m_OutChannelLayout;
		tmp->sample_rate = m_iOutSampleRate;
		tmp->nb_samples = (iSamples <= 0 ? fifoSize: iSamples);
		tmp->format = m_outFormat;
		
		int ret = av_frame_get_buffer(tmp, 0);
		if (ret != 0) 
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_frame_get_buffer error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
			return nullptr;
		}
		if ((ret = av_frame_make_writable(tmp)) != 0) 
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_frame_make_writable:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
			return nullptr;
		}
		ret = av_audio_fifo_read(m_fifo, (void**)tmp->extended_data, tmp->nb_samples);
		if (ret < 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_audio_fifo_read error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
			return nullptr;
		}
		//tmp->pts = m_accuSamples;
		//m_accuSamples += tmp->nb_samples;
		return free_frame_tmp;
	}
	return nullptr;
}

bool CAudioFrameCast::audio_resampler_resample(uint8_t* output[], uint32_t* out_frames, uint64_t* ts_offset, const uint8_t* const input[], uint32_t in_frames)
{
	if (!m_SwrContext)
		return false;
	int ret;

	int64_t delay = swr_get_delay(m_SwrContext, m_iInSampleRate);
	int estimated = (int)av_rescale_rnd(delay + (int64_t)in_frames,
		(int64_t)m_iOutSampleRate,
		(int64_t)m_iInSampleRate,
		AV_ROUND_UP);

	*ts_offset = (uint64_t)swr_get_delay(m_SwrContext, 1000000000);

	/* resize the buffer if bigger */


	if (estimated > *out_frames) {
		if (output[0])
			av_freep(&output[0]);

		int nums = av_samples_alloc(output, NULL, m_OutChannelLayout.nb_channels,
			estimated, m_outFormat, 16);//需要调用16字节对齐函数
		if (nums <= 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_samples_alloc error:%s", av_make_error_string(szErr,sizeof(szErr), nums));
			return false;
		}
		else
			*out_frames = estimated;
		//rs->output_size = estimated;
	}

	ret = swr_convert(m_SwrContext, output, *out_frames,
		(const uint8_t**)input, in_frames);

	if (ret < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("swr_convert error:%s", av_make_error_string(szErr, AV_ERROR_MAX_STRING_SIZE, ret));
		//blog(LOG_ERROR, "swr_convert failed: %d", ret);
		return false;
	}

	//for (uint32_t i = 0; i < rs->output_planes; i++)
	//	output[i] = rs->output_buffer[i];
	*out_frames = (uint32_t)ret;

	if (m_fifo) 
	{
		av_audio_fifo_free(m_fifo);
		m_fifo = NULL;
	}

	return true;
}

uint8_t** CAudioFrameCast::getArray(uint8_t* (&p)[MAX_AV_PLANES], int iSamples)
{
	int size = av_audio_fifo_size(m_fifo);
	if (iSamples > size)
		return NULL;

	uint8_t* data = NULL;
	int linesize[MAX_AV_PLANES];
	memset(linesize, 0, sizeof(linesize));
	if (av_sample_fmt_is_planar(m_outFormat))
		for (int i = 0; i < m_OutChannelLayout.nb_channels; ++i)
			linesize[i] = iSamples;
	else
		linesize[0] = iSamples;

	int err = av_samples_alloc(p, linesize, m_OutChannelLayout.nb_channels, iSamples, m_outFormat, 16);
	if (err < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_samples_alloc error:%s\n", av_make_error_string(szErr, sizeof(szErr), err));
		return nullptr;
	}

	err = av_audio_fifo_read(m_fifo, (void* const*)p, iSamples);
	if (err < 0)
	{
		av_freep(&p[0]);
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_audio_fifo_read error:%s\n", av_make_error_string(szErr, sizeof(szErr), err));
		return nullptr;
	}
	return p;
}

int CAudioFrameCast::getSamplesNum() const
{
	if (m_fifo)
		return av_audio_fifo_size(m_fifo);
	return 0;
}

bool CAudioFrameCast::set(int iInChannels, AVSampleFormat inFormat, int iInSampleRate, int iOutChannels, AVSampleFormat outFormat, int iOutSampleRate)
{
	av_channel_layout_default(&m_InChannelLayout, iInChannels);//av_get_default_channel_layout(iInChannels);//iInChannelLayout;
	av_channel_layout_default(&m_OutChannelLayout,iOutChannels);//iOutChannelLayout;
	return set(&m_InChannelLayout, inFormat, iInSampleRate, &m_OutChannelLayout, outFormat, iOutSampleRate);
}

bool CAudioFrameCast::set(uint64_t InChannelLayout, AVSampleFormat inFormat, int iInSampleRate, uint64_t OutChannelLayout, AVSampleFormat outFormat, int iOutSampleRate)
{
	bool b = true;
	AVChannelLayout inchlay, outchlay;
	int err = av_channel_layout_from_mask(&inchlay, InChannelLayout);
	if (err != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
		b = false;
	}
	err = av_channel_layout_from_mask(&outchlay, OutChannelLayout);
	if (err != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
		b = false;
	}
	if (!b)
		return b;
	return set(&inchlay, inFormat, iInSampleRate, &outchlay, outFormat, iOutSampleRate);
}

bool CAudioFrameCast::set(AVChannelLayout*InChannelLayout, AVSampleFormat inFormat, int iInSampleRate, AVChannelLayout*OutChannelLayout, AVSampleFormat outFormat, int iOutSampleRate)
{
	reset();
	//m_InChannelLayout = InChannelLayout;
	int ret;
	if ((ret = av_channel_layout_copy(&m_InChannelLayout, InChannelLayout)) != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_copy error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	m_inFormat = inFormat;
	m_iInSampleRate = iInSampleRate;

	//m_OutChannelLayout = OutChannelLayout;
	if ((ret = av_channel_layout_copy(&m_OutChannelLayout, OutChannelLayout)) != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_copy error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	m_outFormat = outFormat;
	m_iOutSampleRate = iOutSampleRate;

	int iFormatBytes = av_get_bytes_per_sample(m_outFormat);
	if (iFormatBytes == 0) 
	{
		PrintE("av_get_bytes_per_sample error:unexpected return 0");
		return false;
	}

	int icalc_samplers = AUDIO_MAX_SAMPLERS_COUNT_IN_SECONDS * m_iOutSampleRate * m_OutChannelLayout.nb_channels * iFormatBytes;

	m_fifo = av_audio_fifo_alloc(m_outFormat, m_OutChannelLayout.nb_channels/*m_iOutChannels*/, icalc_samplers);
	if (!m_fifo)
	{
		PrintE("av_audio_fifo_alloc error:no memory");
		return false;
	}
	swr_alloc_set_opts2(&m_SwrContext, &m_OutChannelLayout, m_outFormat, m_iOutSampleRate, &m_InChannelLayout, m_inFormat, m_iInSampleRate, 0, NULL);
	if (!m_SwrContext)
	{
		av_audio_fifo_free(m_fifo);
		m_fifo = nullptr;
		return false;
	}
	if (swr_init(m_SwrContext) != 0)
	{
		swr_free(&m_SwrContext);
		av_audio_fifo_free(m_fifo);
		m_fifo = nullptr;
		m_SwrContext = nullptr;
		return false;
	}
	return true;
}

//int64_t CAudioFrameCast::getExpectIncomingFramePts()
//{
//	int64_t samples = av_audio_fifo_size(m_fifo);
//	assert(samples >= 0);
//	//int64_t iExpectPtsInOutSamplerate = m_pts + samples;
//	int64_t pts = util_mul_div64(samples, 1000000000, m_iOutSampleRate) + swr_get_delay(m_SwrContext, 1000000000) + m_pts;
//	return pts;
//}
//

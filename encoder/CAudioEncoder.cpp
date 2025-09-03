#include "CAudioEncoder.h"
#include "Util/logger.h"
#include "Network/Socket.h"

CAudioEncoder::~CAudioEncoder()
{
	reset();
}

void CAudioEncoder::reset()
{
	if (m_codecCtxAudio)
		avcodec_free_context(&m_codecCtxAudio);
	m_codecAudio = nullptr;
	m_bFlushAudio = false;
	m_iAudioBitRate = 0;
	m_iAudioSampleRate = 0;
	m_iChannels = 0;
	m_u64ChannelLayout = 0;
	m_sampleFmt = AVSampleFormat::AV_SAMPLE_FMT_NONE;
	memset(&m_audioTimeBase, 0, sizeof(m_audioTimeBase));
	m_call_once.reset(new std::once_flag);
	m_opt.clear();
	m_audio_id = AV_CODEC_ID_NONE;
	m_cast.reset();
}

int CAudioEncoder::send_Frame(UPTR_FME f, std::vector<UPTR_PKT>& sets)
{
	std::vector<UPTR_FME>sendf_sets;
	if (m_bFlushAudio)
	{
		PrintW("audio already flushed");
		return 0;
	}
	int ret = 0;
	if (!m_codecCtxAudio)
	{
		std::call_once(*m_call_once.get(), [&ret, this]()
			{
				ret = this->init();
			});
		if (!ret)
			return ret;
	}
	if(!f)
		m_bFlushAudio = true;

	if (f && !m_cast && (f->sample_rate != m_codecCtxAudio->sample_rate
		|| (AVSampleFormat)f->format != m_codecCtxAudio->sample_fmt
		|| av_channel_layout_compare(&f->ch_layout, &m_codecCtxAudio->ch_layout) != 0
		|| (f->nb_samples != m_codecCtxAudio->frame_size && (m_codecAudio->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) == 0)
		)) 
	{
		if (!m_cast.set(&f->ch_layout, (AVSampleFormat)f->format, f->sample_rate, &m_codecCtxAudio->ch_layout, m_codecCtxAudio->sample_fmt, m_codecCtxAudio->sample_rate))
			return -1;
	}
	if (m_cast && f) 
	{
		if (!m_cast.translate(std::move(f)))
			return -1;
		sendf_sets = m_cast.getFrame(m_codecCtxAudio->frame_size);
	}
	else
	{
		sendf_sets.emplace_back(std::move(f));
	}


	for (auto& ele : sendf_sets)
	{
		ret = avcodec_send_frame(m_codecCtxAudio, ele.get());
		if (ret != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("avcodec_send_frame error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return ret;
		}
		do
		{
			UPTR_PKT rel_pkt(av_packet_alloc());
			if (!rel_pkt)
			{
				PrintE("av_packet_alloc error:no memory");
				return AVERROR(ENOMEM);
			}
			ret = avcodec_receive_packet(m_codecCtxAudio, rel_pkt.get());
			if (ret == 0)
			{
				rel_pkt->time_base = m_codecCtxAudio->time_base;
				sets.emplace_back(std::move(rel_pkt));
			}
			else if (ret == AVERROR(EAGAIN))
				break;
			else if (ret == AVERROR_EOF)
			{
				printf("avcodec_receive_packet goto end stream audio:\n");
				//setStreamEndFlag(pEncodeCtx == m_codecCtxVideo ? STREAM_TYPE::ENCODE_VIDEO : ENCODE_AUDIO);
				break;
			}
		} while (true);
	}
	return ret;
}

void CAudioEncoder::setCorrectTs(bool b)
{
	m_cast.enableTsCorrect(b);
}

int CAudioEncoder::initAudioCodecCtx(AVFormatContext* ctx)
{
	int ret;
	if (ctx && ctx->oformat->audio_codec == AV_CODEC_ID_NONE)
	{
		PrintE("file name %s can't deduce audio id");
		return -2;
	}
	if (!m_codecAudio)
	{
		AVCodecID id = m_audio_id == AV_CODEC_ID_NONE ? (ctx ? ctx->oformat->audio_codec: AV_CODEC_ID_NONE) : m_audio_id;
		m_codecAudio = avcodec_find_encoder(id);
		if (!m_codecAudio)
		{
			PrintE("can't find audio encoder id:%d", id);
			return -2;
		}
	}
	AVCodecContext* codecCtxAudio = avcodec_alloc_context3(m_codecAudio);
	if (codecCtxAudio)
	{
		std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*&)>>free_codecAudioCtx(codecCtxAudio, [](AVCodecContext*& p) {avcodec_free_context(&p); });
		if (!initSuitableAudioCodec(codecCtxAudio))
			return -2;
		codecCtxAudio->bit_rate = m_iAudioBitRate;
		if (ctx && ctx->oformat->flags & AVFMT_GLOBALHEADER)
			codecCtxAudio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		if ((ret = avcodec_open2(codecCtxAudio, m_codecAudio, nullptr)) == 0)
		{
			if (codecCtxAudio->frame_size == 0 && m_codecAudio->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
				codecCtxAudio->frame_size = 10000;
			m_codecCtxAudio = free_codecAudioCtx.release();
		}
		else
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("audio avcodec_open2 error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return -2;
		}
	}
	else
	{
		PrintE("failed,avcodec_alloc_context3 return NULL");
		return -1;
	}
	return 0;
}

AVCodecContext* CAudioEncoder::getCodecCtx()
{
	return m_codecCtxAudio;
}

bool CAudioEncoder::init()
{
	if (!initProperty())
		return false;
	int ret;
	if ((ret = initAudioCodecCtx()) != 0)
		return false;

	return true;

}

bool CAudioEncoder::initProperty()
{
	m_iAudioBitRate = m_opt.av_opt_get_int("b:a", 128000);
	m_iAudioSampleRate = m_opt.av_opt_get_int("ar", 48000);
	m_u64ChannelLayout = m_opt.av_opt_get_int("channel_layout", AV_CH_LAYOUT_STEREO);
	m_sampleFmt = m_opt.av_opt_get_sample_fmt("sample_fmt", AV_SAMPLE_FMT_FLTP);
	m_audio_id = m_opt.av_opt_get_codec_id("codecid:a", AV_CODEC_ID_AAC);

	int ret;
	AVChannelLayout chlayout;
	if ((ret = av_channel_layout_from_mask(&chlayout, m_u64ChannelLayout)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	m_iChannels = chlayout.nb_channels;
	return true;
}

bool CAudioEncoder::initSuitableAudioCodec(AVCodecContext* codec_ctx)
{
	if (!m_codecAudio)
		return false;
	//const uint64_t*pLayouts = m_codecAudio->channel_layouts;
	const AVChannelLayout* pLayouts = m_codecAudio->ch_layouts;
	//m_codecCtxAudio->channel_layout = av_get_default_channel_layout(m_iChannels);
	//m_codecCtxAudio->channels = m_iChannels;
	bool bFind = false;
	const AVChannelLayout* pSelChannelLayout = NULL, * lowest = NULL;
	if (pLayouts)
	{
		//uint64_t saveLayoutLowest = 0, saveLayoutLow = 0;
		for (; pLayouts != 0; ++pLayouts)
		{
			if (pLayouts->order == AV_CHANNEL_ORDER_NATIVE)
			{
				if (pLayouts->nb_channels == m_iChannels && pLayouts->u.mask == m_u64ChannelLayout)
				{
					bFind = true;
					pSelChannelLayout = pLayouts;
				}
				if (!lowest)
					lowest = pLayouts;
			}
		}
	}
	if (!bFind)
		pSelChannelLayout = lowest;

	if (pSelChannelLayout)
	{
		codec_ctx->ch_layout = *pSelChannelLayout;
		if (!bFind)
		{
			m_iChannels = codec_ctx->ch_layout.nb_channels;
			m_u64ChannelLayout = codec_ctx->ch_layout.u.mask;
		}
	}
	else
	{
		codec_ctx->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
		codec_ctx->ch_layout.u.mask = AV_CH_LAYOUT_STEREO;
		codec_ctx->ch_layout.nb_channels = m_iChannels;
	}

	//if (pLayouts)
	//{
	//	uint64_t saveLayoutLowest = *pLayouts, saveLayoutLow = *pLayouts;// = m_u64ChannelLayout;
	//	for (; *pLayouts != 0; ++pLayouts)
	//	{
	//		if (m_codecCtxAudio->channel_layout == *pLayouts)
	//			break;
	//		else
	//		{
	//			if (*pLayouts == AV_CH_LAYOUT_STEREO)
	//				saveLayoutLowest = AV_CH_LAYOUT_STEREO;
	//			if (*pLayouts == m_u64ChannelLayout)
	//				saveLayoutLow = m_u64ChannelLayout;
	//		}
	//	}
	//	if (*pLayouts == 0)
	//	{
	//		if (saveLayoutLow == m_u64ChannelLayout)
	//			m_codecCtxAudio->channel_layout = m_u64ChannelLayout;
	//		else
	//			m_codecCtxAudio->channel_layout = saveLayoutLowest;
	//		m_codecCtxAudio->channels = av_get_channel_layout_nb_channels(m_codecCtxAudio->channel_layout);
	//	}
	//}

	codec_ctx->sample_fmt = m_sampleFmt;

	const AVSampleFormat* pAVSampleFormat = m_codecAudio->sample_fmts;
	if (codec_ctx->sample_fmt == AVSampleFormat::AV_SAMPLE_FMT_NONE)
		codec_ctx->sample_fmt = *pAVSampleFormat;

	for (; *pAVSampleFormat != AVSampleFormat::AV_SAMPLE_FMT_NONE; ++pAVSampleFormat)
	{
		if (codec_ctx->sample_fmt == *pAVSampleFormat)
			break;
	}
	if (*pAVSampleFormat == AVSampleFormat::AV_SAMPLE_FMT_NONE)
	{
		//codec_ctx->sample_fmt = *m_codecAudio->sample_fmts;
		PrintE("audio encoder doesn't support audio format %d", codec_ctx->sample_fmt);
		return false;
	}

	codec_ctx->sample_rate = m_iAudioSampleRate;
	int chooseSuitSamples = *m_codecAudio->supported_samplerates;
	const int* tmpSampleRate = m_codecAudio->supported_samplerates;
	for (; *tmpSampleRate != 0; ++tmpSampleRate)
		if (*tmpSampleRate == codec_ctx->sample_rate)
			break;
		else if (std::abs(*tmpSampleRate - codec_ctx->sample_rate) < std::abs(chooseSuitSamples - codec_ctx->sample_rate))
			chooseSuitSamples = *tmpSampleRate;

	if (*tmpSampleRate == 0)
		codec_ctx->sample_rate = chooseSuitSamples;
	codec_ctx->time_base = { 1,codec_ctx->sample_rate };
	return true;
}

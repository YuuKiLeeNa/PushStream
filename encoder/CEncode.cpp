#include "CEncode.h"
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavdevice/avdevice.h"
#include "libavutil/time.h"
#include "libavdevice/avdevice.h"
#include "libavutil/channel_layout.h"
}
#include<memory>
#include<functional>
#include "CSelCodec.h"
#include<assert.h>
#include<functional>
#include<algorithm>
#include<numeric>
#include<iterator>
#include "Util/logger.h"
#include "Network/Socket.h"
#include"Pushstream/some_util_fun.h"




static char szErr[128];
#define PRINT_MSG(msg, x) printf(msg,av_make_error_string(szErr, sizeof(szErr), x))

//template <class _Container>
//class MY_ITER :public  std::iterator<std::random_access_iterator_tag, typename _Container::value_type> { // wrap pushes to back of container as output iterator
//public:
//	using container_type = _Container;
//
//	_CONSTEXPR20 explicit MY_ITER(_Container& _Cont, typename _Container::value_type::second_type&& v) noexcept /* strengthened */
//		: container(_STD addressof(_Cont)), m_member(v) {}
//
//	_CONSTEXPR20 MY_ITER& operator=(const typename _Container::value_type& _Val) {
//		container->push_back(_Val);
//		return *this;
//	}
//
//	_CONSTEXPR20 MY_ITER& operator=(typename _Container::value_type&& _Val) {
//		container->push_back(_STD move(_Val));
//		return *this;
//	}
//
//	_CONSTEXPR20 MY_ITER& operator=(typename _Container::value_type::first_type&& _Val) {
//		container->push_back({ _STD move(_Val) ,m_member });
//		return *this;
//	}
//
//	_NODISCARD _CONSTEXPR20 MY_ITER& operator*() noexcept /* strengthened */ {
//		return *this;
//	}
//
//	_CONSTEXPR20 MY_ITER& operator++() noexcept /* strengthened */ {
//		return *this;
//	}
//
//	_CONSTEXPR20 MY_ITER operator++(int) noexcept /* strengthened */ {
//		return *this;
//	}
//
//protected:
//	_Container* container = nullptr;
//	typename _Container::value_type::second_type m_member;
//};

void CEncode::setFile(const std::string& pathFile, int flag)
{
	//reset();
	m_file = pathFile;
	m_flags = flag;
}

void CEncode::reset()
{
	m_file.clear();
	if (m_fmtCtx)
	{
		avformat_close_input(&m_fmtCtx);
		if (m_fmtCtx && !(m_fmtCtx->flags & AVFMT_NOFILE))
			avio_closep(&m_fmtCtx->pb);
		avformat_free_context(m_fmtCtx);
		m_fmtCtx = nullptr;
	}
	if (m_codecVideo)
	{
		if (m_codecCtxVideo)
		{
			if (m_codecCtxVideo->hw_frames_ctx)
				av_buffer_unref(&m_codecCtxVideo->hw_frames_ctx);
			avcodec_free_context(&m_codecCtxVideo);
		}
	}
	if (m_codecCtxAudio)
		avcodec_free_context(&m_codecCtxAudio);
	m_codecVideo = nullptr;
	m_codecAudio = nullptr;
	//m_CAudioFrameCast.reset();
	m_listAudioPacket.clear();
	m_listVideoPacket.clear();

	//m_iAudioSamples = 0;
	m_bFlushAudio = false;
	m_bFlushVideo = false;
	//m_bCheckNeedAudioCast = false;

	m_iVideoMaxBitRate = 0;
	m_iVideoMinBitRate = 0;
	m_iWidth = 0;
	m_iHeight = 0;
	m_iNeedWriteTail = 0;
	//m_decodeHwFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	//m_decodeFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	m_encodeFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	m_EncodeAVHWDeviceType = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;

	memset(&m_ratRatio, 0, sizeof(m_ratRatio));
	memset(&m_ratFrameRate, 0, sizeof(m_ratFrameRate));
	memset(&m_ratVideoTimeBase, 0, sizeof(m_ratVideoTimeBase));
	//if (m_pAVBufferRef)
	//{
	//	m_pAVBufferRef = nullptr;
	//	//av_buffer_unref(&m_pAVBufferRef);
	//}
	m_iAudioBitRate = 0;
	m_iAudioSampleRate = 0;
	m_iChannels = 0;
	m_u64ChannelLayout = 0;
	m_sampleFmt = AVSampleFormat::AV_SAMPLE_FMT_NONE;
	memset(&m_audioTimeBase, 0, sizeof(m_audioTimeBase));

	if (m_hw_frame)
	{
		//av_frame_unref(m_hw_frame);
		av_frame_free(&m_hw_frame);
		m_hw_frame = NULL;
	}
	//m_iAudioSamples = 0;
	m_call_once.reset(new std::once_flag);
	m_record_last_video_dts = AV_NOPTS_VALUE;
	m_option.clear();
	m_video_id = AV_CODEC_ID_NONE;
	m_audio_id = AV_CODEC_ID_NONE;
	//m_audioCast.reset();
}

//bool CEncode::send_Frame(AVFrame* f, CEncode::STREAM_TYPE type)
//{
//	return send_Frame(UPTR_FME(f), type);
//}

bool CEncode::send_Frame(UPTR_FME f, CEncode::STREAM_TYPE type)
{
	if ((m_bFlushAudio && type == CEncode::STREAM_TYPE::ENCODE_AUDIO)
		|| (m_bFlushVideo && type == CEncode::STREAM_TYPE::ENCODE_VIDEO))
	{
		PrintE("%s already flushed", type == STREAM_TYPE::ENCODE_AUDIO ? "AUDIO" : "VIDEO");
		return false;
	}
	if (!m_codecCtxAudio && !m_codecCtxVideo)
	{
		bool b = false;
		std::call_once(*m_call_once.get(), [&b, this]()
			{
				b = this->init();
			});
		if (!b)
			return false;
	}

	auto pEncodeCtx = getEncCtx(type);
	if (!pEncodeCtx)
		return true;
	if (f)
	{
		if (pEncodeCtx/* == m_codecCtxVideo*/)
			f->pts = av_rescale_q_rnd(f->pts, f->time_base, pEncodeCtx->time_base, AVRounding::AV_ROUND_NEAR_INF);
	}
	else 
	{
		if (type == CEncode::STREAM_TYPE::ENCODE_VIDEO)
			m_bFlushVideo = true;
		else if (type == CEncode::STREAM_TYPE::ENCODE_AUDIO)
			m_bFlushAudio = true;
	}
	int ret;
	bool bNeedWriteTail = false;

	AVFrame* tmpf = NULL;
	if (pEncodeCtx == m_codecCtxVideo && m_codecCtxVideo->hw_frames_ctx && m_hw_frame && f )
	{
		av_frame_unref(m_hw_frame);
		if ((ret = av_hwframe_get_buffer(m_codecCtxVideo->hw_frames_ctx, m_hw_frame, 0)) != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_hwframe_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			av_frame_free(&m_hw_frame);
			return false;
		}

		ret = av_hwframe_transfer_data(m_hw_frame, f.get(), 0);
		if (ret != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_hwframe_transfer_data error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return false;
		}
		//m_hw_frame->pts = f->pts;
		//m_hw_frame->time_base = f->time_base;
		ret = av_frame_copy_props(m_hw_frame, f.get());
		if (ret != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_frame_copy_props error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return false;
		}
		tmpf = m_hw_frame;
	}
	else
	{
		tmpf = f.get();
		/*if (!m_bCheckNeedAudioCast && tmpf)
		{
			m_audioCast.reset();
			if (m_codecCtxAudio->frame_size != tmpf->nb_samples
				|| m_codecCtxAudio->sample_rate != tmpf->sample_rate
				|| m_codecCtxAudio->sample_fmt != (AVSampleFormat)tmpf->format
				|| av_channel_layout_compare(&m_codecCtxAudio->ch_layout, &tmpf->ch_layout) != 0)
			{
				m_audioCast.reset(new decltype(m_audioCast)::element_type);
				if (!m_audioCast) 
				{
					PrintE("no memory for 'new CAudioFrameCast'");
					return false;
				}
				if (!m_audioCast->set(&tmpf->ch_layout
					, (AVSampleFormat)tmpf->format
					, tmpf->sample_rate
					, &m_codecCtxAudio->ch_layout
					, m_codecCtxAudio->sample_fmt
					, m_codecCtxAudio->sample_rate)) 
				{
					m_audioCast.reset();
					return false;
				}
			}
			m_bCheckNeedAudioCast = true;
		}
		if (m_audioCast) 
		{
		}*/
	}
	//return true;

	ret = avcodec_send_frame(pEncodeCtx, tmpf);
	if (ret != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avcodec_send_frame error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	//av_init_packet(rel_pkt.get());
	do
	{
		UPTR_PKT rel_pkt(av_packet_alloc());
		if (!rel_pkt) 
		{
			PrintE("av_packet_alloc error:no memory");
			return false;
		}
		ret = avcodec_receive_packet(pEncodeCtx, rel_pkt.get());
		if (ret == 0)
		{
			if (pEncodeCtx == m_codecCtxVideo)
			{
				rel_pkt->stream_index = m_videoStream->index;
				//printf("before pts=%lld dts=%lld", rel_pkt->pts, rel_pkt->dts);
				/*if (rel_pkt->pts != AV_NOPTS_VALUE)
					rel_pkt->pts = av_rescale_q_rnd(rel_pkt->pts, FC.second->time_base, m_videoStream->time_base, AVRounding::AV_ROUND_NEAR_INF);
				else
					printf("warning:rel_pkt->pts == AV_NOPTS_VALUE");
				if(rel_pkt->dts != AV_NOPTS_VALUE)
					rel_pkt->dts = av_rescale_q_rnd(rel_pkt->dts, FC.second->time_base, m_videoStream->time_base, AVRounding::AV_ROUND_NEAR_INF);
				else
					printf("warning:rel_pkt->dts == AV_NOPTS_VALUE");*/
				/*if (!rel_pkt->data)
				{
					printf("xxx\n");
				}*/

				//printf("video stream dts=%lld  pts=%lld\n", rel_pkt->dts, rel_pkt->pts);

				if (rel_pkt->dts > rel_pkt->pts || rel_pkt->dts == AV_NOPTS_VALUE)
					rel_pkt->dts = rel_pkt->pts;

				av_packet_rescale_ts(rel_pkt.get(), pEncodeCtx->time_base, m_videoStream->time_base);
				if (m_record_last_video_dts == AV_NOPTS_VALUE)
				{
					m_record_last_video_dts = rel_pkt->dts;
				}
				else
				{
					if (m_record_last_video_dts >= rel_pkt->dts)
					{
						PrintW("input packet dts doesn't deformation increment");
						rel_pkt->dts = ++m_record_last_video_dts;
					}
					else
						m_record_last_video_dts = rel_pkt->dts;
				}
				//printf("\tafter pts=%lld dts=%lld\n", rel_pkt->pts, rel_pkt->dts);
				//if(rel_pkt)
				m_listVideoPacket.push_back(std::move(rel_pkt));
				//else 
				//	printf("error:avcodec_receive_packet return 0,but packet=NULL\n");
			}
			else
			{
				//printf("before pts=%lld dts=%lld", rel_pkt->pts, rel_pkt->dts);
				rel_pkt->stream_index = m_audioStream->index;
				//rel_pkt->pts = av_rescale_q_rnd(rel_pkt->pts, FC.second->time_base, m_audioStream->time_base, AVRounding::AV_ROUND_NEAR_INF);
				//rel_pkt->dts = rel_pkt->pts;
				av_packet_rescale_ts(rel_pkt.get(), pEncodeCtx->time_base, m_audioStream->time_base);
				//if (rel_pkt)
				//m_listAudioPacket.push_back(std::move(rel_pkt));
				//else
				//	printf("error:avcodec_receive_packet return 0,but packet=NULL\n");
				//printf("\tafter pts=%lld dts=%lld\n", rel_pkt->pts, rel_pkt->dts);


				//static std::list<int64_t>audio_list;
				//static int num = 0;
				//if (++num > 500)
				//	audio_list.pop_front();
				//if (!audio_list.empty())
				//{
				//	if (rel_pkt->dts <= audio_list.back())
				//	{
				//		printf("xxxxxxxxxxxxxxxxx\n");
				//	}
				//}
				//audio_list.push_back(rel_pkt->dts);
				

				m_listAudioPacket.push_back(std::move(rel_pkt));
			}
		}
		else if (ret == AVERROR(EAGAIN))
			break;
		else if (ret == AVERROR_EOF)
		{
			printf("avcodec_receive_packet goto end stream id:%s\n", pEncodeCtx == m_codecCtxVideo ? "VIDEO" : "AUDIO");
			setStreamEndFlag(pEncodeCtx == m_codecCtxVideo ? STREAM_TYPE::ENCODE_VIDEO : ENCODE_AUDIO);
			break;
		}
	} while (true);

	std::vector<UPTR_PKT>sets = getPacket();
	//if (!sets.empty())
	//	sets.emplace_back(UPTR_PKT());
	//return true;

	for (auto& ele : sets)
	{
		//int ret = av_interleaved_write_frame(m_fmtCtx, ele.get());
		int ret = av_write_frame(m_fmtCtx, ele.get());
		if (ret < 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_write_frame error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return false;
		}
	}

	writeTailIfAllStreamEnd();
	return true;
}

void CEncode::setOption(const CMediaOption& opt)
{
	m_option = opt;
}

CMediaOption CEncode::getOption()
{
	return m_option;
}

int CEncode::initVideoCodecCtxStream(AVFormatContext* &ctx, const AVCodec* codec, const AVCodecHWConfig* pConfig)
{
	if (!(m_flags & ENCODE_VIDEO))
		return 0;

	if (!codec)
		return -2;
	m_codecVideo = codec;
	
	AVCodecContext* codecCtxVideo = avcodec_alloc_context3(m_codecVideo);
	if (!codecCtxVideo)
		return -2;
	int ret;
	if (codecCtxVideo)
	{
		std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*&)>>freem_codecVideoCtx(codecCtxVideo, [](AVCodecContext*& p)
			{
				avcodec_free_context(&p);
			});
		//if (m_iVideoMinBitRate != 0)
		//	codecCtxVideo->bit_rate = m_iVideoBitRate;
		codecCtxVideo->framerate = m_ratFrameRate;
		//if (m_iVideoMaxBitRate != 0)
		codecCtxVideo->rc_max_rate = m_iVideoMaxBitRate;
		//if (m_iVideoMinBitRate != 0)
		codecCtxVideo->rc_min_rate = m_iVideoMinBitRate;
		//if (m_iVideoMaxBitRate)
		codecCtxVideo->rc_buffer_size = 2 * m_iVideoMaxBitRate;
		codecCtxVideo->time_base = m_ratVideoTimeBase;
		codecCtxVideo->codec_type = AVMEDIA_TYPE_VIDEO;
		codecCtxVideo->width = m_iWidth;
		codecCtxVideo->height = m_iHeight;
		codecCtxVideo->sample_aspect_ratio = m_ratRatio;
		codecCtxVideo->gop_size = (int)m_ratFrameRate.num / m_ratFrameRate.den;
		if (pConfig)
		{
			codecCtxVideo->pix_fmt = pConfig->pix_fmt;
			AVBufferRef* buf = nullptr;
			if ((ret = av_hwdevice_ctx_create(&buf, pConfig->device_type, "auto", nullptr, 0)) != 0)
			{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_hwdevice_ctx_create error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return -1;
			}
			std::unique_ptr<AVBufferRef, std::function<void(AVBufferRef*&)>>rel_free(buf, [](AVBufferRef*& f) {av_buffer_unref(&f); });
			codecCtxVideo->hw_frames_ctx = av_hwframe_ctx_alloc(buf);
			AVHWFramesContext* pFrameContext = (AVHWFramesContext*)codecCtxVideo->hw_frames_ctx->data;
			pFrameContext->width = m_iWidth;
			pFrameContext->height = m_iHeight;
			pFrameContext->initial_pool_size = 20;
			pFrameContext->format = pConfig->pix_fmt;
			const AVPixelFormat* ptmp = codec->pix_fmts;
			if (*ptmp == AV_PIX_FMT_NONE) 
				return -1;
			while (ptmp)
			{
				AVPixelFormat fmt = *ptmp;
				if (fmt == AV_PIX_FMT_NONE || fmt == m_encodeFmt)
					break;
				++ptmp;
			}
			if (*ptmp == AV_PIX_FMT_NONE)
			{
				return -1;
				//pFrameContext->sw_format = *codec->pix_fmts;
			}
			else
				pFrameContext->sw_format = m_encodeFmt;
			if ((ret = av_hwframe_ctx_init(codecCtxVideo->hw_frames_ctx)) != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_hwframe_ctx_init error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				av_buffer_unref(&codecCtxVideo->hw_frames_ctx);
				return -1;
			}
			if (!initHWFrame(codecCtxVideo))
				return -1;
		}
		else
		{
			const enum AVPixelFormat* pFormat = m_codecVideo->pix_fmts;
			for (; *pFormat != AV_PIX_FMT_NONE;)
			{
				if (*pFormat == m_encodeFmt)
					break;
			}
			if (*pFormat != AV_PIX_FMT_NONE)
				codecCtxVideo->pix_fmt = *pFormat;
			else
				return -2;
				//codecCtxVideo->pix_fmt = *m_codecVideo->pix_fmts;
		}
		if (codecCtxVideo->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
				/* just for testing, we also add B-frames */
			codecCtxVideo->max_b_frames = 2;
		}
		if (codecCtxVideo->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
			/* Needed to avoid using macroblocks in which some coeffs overflow.
			 * This does not happen with normal video, it just happens here as
			 * the motion of the chroma plane does not match the luma plane. */
			codecCtxVideo->mb_decision = 2;
		}
		if (ctx->oformat->flags & AVFMT_GLOBALHEADER)
			codecCtxVideo->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		if (codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_HEVC)
		{
			std::string strPreset = m_option.av_opt_get("preset", "medium");
			ret = av_opt_set(codecCtxVideo->priv_data, "preset", strPreset.c_str(), 0);
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_opt_set error for preset=%s:%s", strPreset.c_str(), av_make_error_string(szErr, sizeof(szErr), ret));
			}
			std::string strProfile = m_option.av_opt_get("profile", "main");
			ret = av_opt_set(codecCtxVideo->priv_data, "profile", strProfile.c_str(), 0);
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_opt_set error for profile=%s:%s", strProfile.c_str(), av_make_error_string(szErr, sizeof(szErr), ret));
			}
			if (strncmp(m_codecVideo->name,"hevc_nvenc", sizeof("hevc_nvenc")) == 0
				|| strncmp(m_codecVideo->name, "h264_nvenc", sizeof("h264_nvenc")) == 0)
			{
				//std::string strTune = m_option.av_opt_get("tune", "hq");
				ret = av_opt_set(codecCtxVideo->priv_data, "tune", "hq", 0);
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_opt_set error for tune=hp:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				}
				std::string strRC = m_option.av_opt_get("rc", "vbr_hq");
				ret = av_opt_set(codecCtxVideo->priv_data, "rc", strRC.c_str(), 0);
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_opt_set error for rc=vbr_hq:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				}
			}
			if (strncmp(m_codecVideo->name, "libx265", sizeof("libx265")) == 0)
			{
				ret = av_opt_set(codecCtxVideo->priv_data, "tune", "film", 0);
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_opt_set error for tune=film:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				}
				int bitrate = (int)(m_iVideoMaxBitRate / 1024);
				std::stringstream ss;
				ss << "vbv-maxrate=" << bitrate << ":vbv-bufsize=" << bitrate * 2;
				ret = av_opt_set(codecCtxVideo->priv_data, "x265-params", ss.str().c_str(), 0);
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_opt_set error for x265-params=%s:%s", ss.str().c_str(),av_make_error_string(szErr, sizeof(szErr), ret));
				}
			}
			if (strncmp(m_codecVideo->name, "libx264", sizeof("libx264")) == 0) 
			{
				ret = av_opt_set(codecCtxVideo->priv_data, "tune", "film", 0);
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_opt_set error for tune=film:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				}
				ret = av_opt_set(codecCtxVideo->priv_data, "x264-params", "nal-hrd=vbr", 0);
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_opt_set error for x264-params=nal-hrd=vbr:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				}
			}
		}
		if ((ret = avcodec_open2(codecCtxVideo, m_codecVideo, nullptr)) != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("avcodec_open2 error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			if (codecCtxVideo->hw_frames_ctx)
				av_buffer_unref(&codecCtxVideo->hw_frames_ctx);
			codecCtxVideo = NULL;
			return -1;
		}
		else
		{
			m_videoStream = avformat_new_stream(ctx, m_codecVideo);
			if (!m_videoStream)
				return -2;
			if ((ret=avcodec_parameters_from_context(m_videoStream->codecpar, codecCtxVideo)) >= 0)
			{
				m_videoStream->time_base = { 1, AV_TIME_BASE };
				m_videoStream->id = ctx->nb_streams - 1;
				m_codecCtxVideo = freem_codecVideoCtx.release();
				m_bFlushVideo = false;
			}
			else 
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("avcodec_parameters_from_context error:%s", av_make_error_string(szErr,sizeof(szErr), ret));
				avformat_free_context(ctx);
				ctx = NULL;
				return -2;
			}
		}
	}
	return 0;
}

int CEncode::initAudioCodecCtxStream(AVFormatContext*& ctx)
{
	if (!(m_flags & ENCODE_AUDIO))
		return 0;

	int ret;
	if (ctx->oformat->audio_codec == AV_CODEC_ID_NONE)
	{
		PrintE("file name %s can't deduce audio id", m_file.c_str());
		return -2;
	}
	if (!m_codecAudio)
	{
		AVCodecID id = m_audio_id == AV_CODEC_ID_NONE ? ctx->oformat->audio_codec : m_audio_id;
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
		if (ctx->oformat->flags & AVFMT_GLOBALHEADER)
				codecCtxAudio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		if ((ret = avcodec_open2(codecCtxAudio, m_codecAudio, nullptr)) == 0)
		{
			if (m_codecAudio->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
				codecCtxAudio->frame_size = 10000;

			m_audioStream = avformat_new_stream(ctx, m_codecAudio);
			if (!m_audioStream)
				return -2;
			
			if ((ret=avcodec_parameters_from_context(m_audioStream->codecpar, codecCtxAudio)) >= 0)
			{
				m_audioStream->time_base = { 1, codecCtxAudio->sample_rate };
				m_audioStream->id = ctx->nb_streams - 1;
				m_codecCtxAudio = free_codecAudioCtx.release();
				m_bFlushAudio = false;
			}
			else 
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("avcodec_parameters_from_context error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return -2;
			}
		}
		else
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("audio avcodec_open2 error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return -2;
		}
	}
	return 0;
}

AVCodecContext* CEncode::getEncCtx(CEncode::STREAM_TYPE type)
{
	switch (type)
	{
	case STREAM_TYPE::ENCODE_VIDEO:
		return m_codecCtxVideo;
	case STREAM_TYPE::ENCODE_AUDIO:
		return m_codecCtxAudio;
	default:
		break;
	}
	return nullptr;
}

bool CEncode::init()
{
	if ((m_flags & (ENCODE_VIDEO | ENCODE_AUDIO)) == 0)
	{
		PrintE("file doesn't set ENCODE_VIDEO or ENCODE_AUDIO flag");
		return false;
	}

	m_iAudioBitRate = m_option.av_opt_get_int("b:a", 128000);
	m_iVideoMinBitRate = m_option.av_opt_get_int("b:vMin", 2560 * 1024);
	m_iVideoMaxBitRate = m_option.av_opt_get_int("b:vMax", 5120 * 1024);
	m_iAudioSampleRate = m_option.av_opt_get_int("ar", 48000);
	m_u64ChannelLayout = m_option.av_opt_get_int("channel_layout", AV_CH_LAYOUT_STEREO);
	m_sampleFmt = m_option.av_opt_get_sample_fmt("sample_fmt", AV_SAMPLE_FMT_FLTP);
	AVRational videosize = m_option.av_opt_get_q("s", { 1920 ,1080 });
	m_ratRatio = m_option.av_opt_get_q("aspect", { 1,1 });

	m_video_id = m_option.av_opt_get_codec_id("codecid:v", AV_CODEC_ID_H265);
	m_audio_id = m_option.av_opt_get_codec_id("codecid:a", AV_CODEC_ID_AAC);

	m_ratVideoTimeBase = m_option.av_opt_get_q("v:time_base", { 1,30 });
	m_ratFrameRate = m_option.av_opt_get_q("r", { 30,1 });

	m_encodeFmt = m_option.av_opt_get_pixel_fmt("pix_fmt", AV_PIX_FMT_YUV420P);
	if (m_encodeFmt == AV_PIX_FMT_NONE)
		return false;


	int ret;
	AVChannelLayout chlayout;
	if ((ret=av_channel_layout_from_mask(&chlayout, m_u64ChannelLayout)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s",av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	m_iChannels = chlayout.nb_channels;
	m_iWidth = videosize.num;
	m_iHeight = videosize.den;

	//std::vector<AVPixelFormat>formatsets = CSelCodec::getHWPixelFormat();
	AVFormatContext* fmtCtx = NULL;
	//if (m_flags & ENCODE_VIDEO)
	//{
	const struct AVOutputFormat* oformat = av_guess_format(/*"mp4"*/nullptr, m_file.c_str(), nullptr);
	
	if ((ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, m_file.c_str())) < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avformat_alloc_output_context2 error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	if (!fmtCtx->oformat)
		fmtCtx->oformat = oformat;//av_guess_format("mp4", nullptr, nullptr);

	if (!fmtCtx->url && !(fmtCtx->url = av_strdup(m_file.c_str())))
	{
		PrintE("av_strdup error\n");
		avformat_free_context(fmtCtx);
		return false;
	}
	//fmtCtx->flags |= AVFMT_FLAG_FLUSH_PACKETS;
	//fmtCtx->flush_packets = 1;

	if (m_flags & ENCODE_VIDEO)
	{

		if (oformat->video_codec == AV_CODEC_ID_NONE)
		{
			PrintE("file name %s error:cant't deduced video format", m_file.c_str());
			avformat_free_context(fmtCtx);
			return false;
		}

		AVCodecID id = m_video_id != AV_CODEC_ID_NONE ? m_video_id : oformat->video_codec;
		std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>hw_sets = CSelCodec::getHWEncodeCodecByStreamIdAndAcceptPixFmt(id, true,AV_PIX_FMT_NONE, AV_HWDEVICE_TYPE_NONE);
		if (hw_sets.empty())
		{
				PrintE("cant't find video encoder");
				return false;
		}
	
		for (auto& ele : hw_sets)
		{
			if (!fmtCtx)
			{
				if ((ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, m_file.c_str())) < 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("avformat_alloc_output_context2 error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return false;
				}
				if (!fmtCtx->oformat)
					fmtCtx->oformat = oformat;//av_guess_format("mp4", nullptr, nullptr);

				if (!fmtCtx->url && !(fmtCtx->url = av_strdup(m_file.c_str())))
				{
					PrintE("av_strdup error\n");
					avformat_free_context(fmtCtx);
					return false;
				}
				//fmtCtx->flags |= AVFMT_FLAG_FLUSH_PACKETS;
				//fmtCtx->flush_packets = 1;
			}
			ret = initVideoCodecCtxStream(fmtCtx, ele.first, ele.second);
			if (ret == -2)
			{
				avformat_free_context(fmtCtx);
				return false;
			}
			if (ret == 0)
				break;
		}
	}
	
	
	std::unique_ptr<AVFormatContext, std::function<void(AVFormatContext*&)>>freectxformat(fmtCtx, [](AVFormatContext*& f)
			{
					avformat_free_context(f);
			});


	if (initAudioCodecCtxStream(fmtCtx) != 0)
		return false;

	m_fmtCtx = freectxformat.release();
	if (m_codecCtxAudio || m_codecCtxVideo)
	{
		if (!(fmtCtx->flags & AVFMT_NOFILE))
		{
			if ((ret = avio_open(&fmtCtx->pb, m_file.c_str(), AVIO_FLAG_WRITE)) < 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("avio_open error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				reset();
				return false;
			}
		}
		if ((ret = avformat_write_header(fmtCtx, nullptr)) < 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("avformat_write_header error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			reset();
			return false;
		}
		return true;
	}
	else
		return false;
}

bool CEncode::initSuitableAudioCodec(AVCodecContext*codec_ctx)
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

std::vector<UPTR_PKT> CEncode::getPacket()
{
	std::vector<UPTR_PKT>sets;
	//m_flags;
	auto iterVideo = m_listVideoPacket.begin();
	auto iterAudio = m_listAudioPacket.begin();

	auto MoveAudioToList = [&]()
		{
			assert(iterVideo == m_listVideoPacket.end());
			std::back_insert_iterator<std::remove_reference<decltype(sets)>::type>iter(sets);
			std::for_each(std::make_move_iterator(iterAudio), std::make_move_iterator(m_listAudioPacket.end()), [&iter, this](decltype(m_listAudioPacket)::value_type&& ele)
				{
					*iter++ = std::forward<decltype(m_listAudioPacket)::value_type>(ele);//std::make_pair<std::remove_reference<decltype(sets)>::type::value_type::first_type, std::remove_reference<decltype(sets)>::type::value_type::second_type>(std::move(ele), std::move(m_codecCtxAudio));
				});
			iterAudio = m_listAudioPacket.end();
		};

	auto MoveVideoToList = [&]()
		{
			assert(iterAudio == m_listAudioPacket.end());
			std::back_insert_iterator<std::remove_reference<decltype(sets)>::type>iter(sets);
			std::for_each(std::make_move_iterator(iterVideo), std::make_move_iterator(m_listVideoPacket.end()), [&iter, this](decltype(m_listVideoPacket)::value_type&& ele)
				{
					*iter++ = std::forward<decltype(m_listVideoPacket)::value_type>(ele);
				});
			iterVideo = m_listVideoPacket.end();
		};

	if ((m_flags & (ENCODE_VIDEO | ENCODE_AUDIO)) == (ENCODE_VIDEO | ENCODE_AUDIO))
	{
		while (iterVideo != m_listVideoPacket.end() && iterAudio != m_listAudioPacket.end())
		{
			if (av_rescale_q_rnd((*iterVideo)->dts, m_videoStream->time_base, { 1, AV_TIME_BASE }, AVRounding::AV_ROUND_NEAR_INF)
				<= av_rescale_q_rnd((*iterAudio)->dts, m_audioStream->time_base, { 1, AV_TIME_BASE }, AVRounding::AV_ROUND_NEAR_INF))
				sets.push_back(std::move(*iterVideo++));
			else
				sets.push_back(std::move(*iterAudio++));

		}
	}

	if ((m_bFlushAudio && m_bFlushVideo) || (m_flags & (ENCODE_VIDEO | ENCODE_AUDIO)) != (ENCODE_VIDEO | ENCODE_AUDIO))
	{
		if (iterVideo != m_listVideoPacket.end())
			MoveVideoToList();
		else if (iterAudio != m_listAudioPacket.end())
			MoveAudioToList();
	}

	m_listVideoPacket.erase(m_listVideoPacket.begin(), iterVideo);
	m_listAudioPacket.erase(m_listAudioPacket.begin(), iterAudio);

	/*
	static int times = 0;
	++times;
	if (times % 30 == 0)
	{
		int byte_size_audio_list = 0;
		int iNum_audio = 0;
		int byte_size_video_list = 0;
		int iNum_video = 0;
		for (auto& ele : m_listAudioPacket)
		{
			byte_size_audio_list += ele->size;
			++iNum_audio;
		}
		for (auto& ele : m_listVideoPacket)
		{
			byte_size_video_list += ele->size;
			++iNum_video;
		}

		printf("audio size(%d) MB=%f \t video size(%d) MB=%f\n", iNum_audio, (float)byte_size_audio_list/(1024*1024), iNum_video,(float)byte_size_video_list / (1024 * 1024));
	}
	*/

	return sets;
}

void CEncode::setStreamEndFlag(CEncode::STREAM_TYPE type)
{
	m_iNeedWriteTail |= type;
}

int CEncode::writeTail()
{
	int err = AVERROR_INVALIDDATA;
	if (m_fmtCtx)
	{
		err = av_write_trailer(m_fmtCtx);
		if (err != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_write_trailer error:%s", av_make_error_string(szErr, sizeof(szErr), err));
		}
	}
	return err;
}

int CEncode::writeTailIfAllStreamEnd()
{
	if (m_flags == m_iNeedWriteTail)
		return writeTail();
	return 0;
}

bool CEncode::initHWFrame(AVCodecContext*codec_ctx)
{
	if (m_hw_frame)
	{
		av_frame_free(&m_hw_frame);
	}
	m_hw_frame = av_frame_alloc();
	
	int ret = av_hwframe_get_buffer(codec_ctx->hw_frames_ctx, m_hw_frame, 0);
	if (ret != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_hwframe_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}

	AVPixelFormat* fmt = NULL;
	ret = av_hwframe_transfer_get_formats(codec_ctx->hw_frames_ctx, AVHWFrameTransferDirection::AV_HWFRAME_TRANSFER_DIRECTION_TO, &fmt, 0);
	if (ret != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_hwframe_transfer_get_formats error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}

	for (; *fmt != AV_PIX_FMT_NONE; ++fmt)
	{
		AVPixelFormat tmp = *fmt;
	}
	return true;
}

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



using REL_FRAME = CEncode::REL_FRAME;
using FRAME = CEncode::FRAME;

static char szErr[128];
#define PRINT_MSG(msg, x) printf(msg,av_make_error_string(szErr, sizeof(szErr), x))

template <class _Container>
class MY_ITER:public  std::iterator<std::random_access_iterator_tag, typename _Container::value_type> { // wrap pushes to back of container as output iterator
public:
	using container_type = _Container;

	_CONSTEXPR20 explicit MY_ITER(_Container& _Cont, typename _Container::value_type::second_type&& v) noexcept /* strengthened */
		: container(_STD addressof(_Cont)), m_member(v){}

	_CONSTEXPR20 MY_ITER& operator=(const typename _Container::value_type& _Val) {
		container->push_back(_Val);
		return *this;
	}

	_CONSTEXPR20 MY_ITER& operator=(typename _Container::value_type&& _Val) {
		container->push_back(_STD move(_Val));
		return *this;
	}

	_CONSTEXPR20 MY_ITER& operator=(typename _Container::value_type::first_type&& _Val) {
		container->push_back({ _STD move(_Val) ,m_member });
		return *this;
	}

	_NODISCARD _CONSTEXPR20 MY_ITER& operator*() noexcept /* strengthened */ {
		return *this;
	}

	_CONSTEXPR20 MY_ITER& operator++() noexcept /* strengthened */ {
		return *this;
	}

	_CONSTEXPR20 MY_ITER operator++(int) noexcept /* strengthened */ {
		return *this;
	}

protected:
	_Container* container = nullptr;
	typename _Container::value_type::second_type m_member;
};

void CEncode::setFile(const std::string& pathFile, int flag)
{
	reset();
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
		avcodec_free_context(&m_codecCtxVideo);
	if (m_codecCtxAudio)
		avcodec_free_context(&m_codecCtxAudio);
	m_codecVideo = nullptr;
	m_codecAudio = nullptr;
	//m_CAudioFrameCast.reset();
	m_listAudioFrame.clear();
	m_listVideoFrame.clear();
	//m_iAudioSamples = 0;
	m_bFlushAudio = false;
	m_bFlushVideo = false;

	m_iVideoBitRate = 0;
	m_iWidth = 0;
	m_iHeight = 0;
	m_iNeedWriteTail = 0;
	m_decodeHwFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	m_decodeFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	m_encodeFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	m_decodeAVHWDeviceType = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;

	memset(&m_ratRatio, 0, sizeof(m_ratRatio));
	memset(&m_ratFrameRate,0,sizeof(m_ratFrameRate));
	memset(&m_ratTimeBase,0,sizeof(m_ratTimeBase));
	if (m_pAVBufferRef)
	{
		m_pAVBufferRef = nullptr;
		//av_buffer_unref(&m_pAVBufferRef);
	}
	m_iAudioBitRate = 0;
	m_iAudioSampleRate = 0;
	m_iChannels = 0;
	m_u64ChannelLayout = 0;
	m_sampleFmt = AVSampleFormat::AV_SAMPLE_FMT_NONE;
	memset(&m_audioTimeBase,0,sizeof(m_audioTimeBase));

	if (m_hw_frame) 
	{
		av_frame_free(&m_hw_frame);
		m_hw_frame = NULL;
	}
	//m_iAudioSamples = 0;
	m_call_once.reset(new std::once_flag);
	m_record_last_video_dts = AV_NOPTS_VALUE;
}

bool CEncode::send_Frame(AVFrame* f, CEncode::STREAM_TYPE type)
{
	return send_Frame(FRAME(f), type);
}

bool CEncode::send_Frame(FRAME f, CEncode::STREAM_TYPE type)
{

	if ((m_bFlushAudio && type == CEncode::STREAM_TYPE::ENCODE_AUDIO)
		|| (m_bFlushVideo && type == CEncode::STREAM_TYPE::ENCODE_VIDEO))
	{
		printf("%s already flushed\n", type == STREAM_TYPE::ENCODE_AUDIO ? "AUDIO" : "VIDEO");
		return false;
	}
	if (!m_codecCtxAudio && !m_codecCtxVideo) 
	{
		bool b = false;
		std::call_once(*m_call_once.get(), [&b,this]() 
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
		if (pEncodeCtx == m_codecCtxVideo)
			f->pts = av_rescale_q_rnd(f->pts, f->time_base, pEncodeCtx->time_base, AVRounding::AV_ROUND_NEAR_INF);
	}
	if (!pushFrame(f.release(), type))
		return false;
	int ret;
	auto frameSets = getFrames();
	bool bNeedWriteTail = false;

	for (auto& FC : frameSets)
	{
		if (FC.second == m_codecCtxVideo  && m_hw_frame && FC.first)
		{
			av_frame_unref(m_hw_frame);

			if ((ret=av_hwframe_get_buffer(m_codecCtxVideo->hw_frames_ctx, m_hw_frame, 0)) != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_hwframe_get_buffer error:%s", av_make_error_string(szErr,sizeof(szErr), ret));
				av_frame_free(&m_hw_frame);
				return false;
			}

			ret = av_hwframe_transfer_data(m_hw_frame, FC.first.get(), 0);
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_hwframe_transfer_data error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return false;
			}
			ret = av_frame_copy_props(m_hw_frame, FC.first.get());
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_frame_copy_props error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return false;
			}
			ret = avcodec_send_frame(FC.second, m_hw_frame);
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("avcodec_send_frame error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return false;
			}
		}
		else
			ret = avcodec_send_frame(FC.second, FC.first.get());
		if (ret != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("avcodec_send_frame error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return false;
		}
		std::unique_ptr<AVPacket, std::function<void(AVPacket*&)>>rel_pkt(av_packet_alloc(), [](AVPacket*& p)
			{
				av_packet_unref(p);
				av_packet_free(&p);
			});
		av_init_packet(rel_pkt.get());
		do
		{
			ret = avcodec_receive_packet(FC.second, rel_pkt.get());
			if (ret == 0)
			{
				if (FC.second == m_codecCtxVideo)
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
					if (!rel_pkt->data) 
					{
						printf("xxx\n");
					}

					//printf("video stream dts=%lld  pts=%lld\n", rel_pkt->dts, rel_pkt->pts);
					
					if (rel_pkt->dts > rel_pkt->pts || rel_pkt->dts == AV_NOPTS_VALUE)
						rel_pkt->dts = rel_pkt->pts;

					av_packet_rescale_ts(rel_pkt.get(), FC.second->time_base, m_videoStream->time_base);
					if (m_record_last_video_dts == AV_NOPTS_VALUE)
					{
						m_record_last_video_dts = rel_pkt->dts;
					}
					else
					{
						if (m_record_last_video_dts >= rel_pkt->dts)
							rel_pkt->dts = ++m_record_last_video_dts;
						else
							m_record_last_video_dts = rel_pkt->dts;
					}
					//printf("\tafter pts=%lld dts=%lld\n", rel_pkt->pts, rel_pkt->dts);
				}
				else
				{
					//printf("before pts=%lld dts=%lld", rel_pkt->pts, rel_pkt->dts);
					rel_pkt->stream_index = m_audioStream->index;
					//rel_pkt->pts = av_rescale_q_rnd(rel_pkt->pts, FC.second->time_base, m_audioStream->time_base, AVRounding::AV_ROUND_NEAR_INF);
					//rel_pkt->dts = rel_pkt->pts;
					av_packet_rescale_ts(rel_pkt.get(), FC.second->time_base, m_audioStream->time_base);
					//printf("\tafter pts=%lld dts=%lld\n", rel_pkt->pts, rel_pkt->dts);
				}
				if(rel_pkt->data)
					int ret = av_interleaved_write_frame(m_fmtCtx, rel_pkt.get());
				else 
				{
					printf("unknown error\n");
				}
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_interleaved_write_frame error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return false;
				}
			}
			else if (ret == AVERROR(EAGAIN))
				break;
			else if (ret == AVERROR_EOF)
			{
				printf("avcodec_receive_packet goto end stream id:%s\n", FC.second == m_codecCtxVideo ? "VIDEO" : "AUDIO");
				setStreamEndFlag(FC.second == m_codecCtxVideo ? STREAM_TYPE::ENCODE_VIDEO:ENCODE_AUDIO);
				writeTailIfAllStreamEnd();
				break;
			}
		} while (true);
	}
	return true;
}

void CEncode::setVideoBitRate(int bitRate)
{
	m_iVideoBitRate = bitRate;
}

void CEncode::setAudioBitRate(int bitRate) 
{
	m_iAudioBitRate = bitRate;
}

void CEncode::setAudioInfo(int sampleRate, int channels, uint64_t channelLayout, AVSampleFormat fmt)
{
	m_iAudioSampleRate = sampleRate;
	m_iChannels = channels;
	m_u64ChannelLayout = channelLayout;
	m_sampleFmt = fmt;
}

void CEncode::setVideoInfo(AVHWDeviceType decodeHwDeviceType, AVBufferRef* pAVBufferRef, int iWidth, int iHeight, AVPixelFormat decodeHwFmt, AVPixelFormat decodeFmt, AVPixelFormat encodeFmt, const AVRational& sar, const AVRational& ratFrameRate, const AVRational& ratTimebase)
{
	if(pAVBufferRef)
		m_pAVBufferRef = av_buffer_ref(pAVBufferRef);
	m_iWidth = iWidth;
	m_iHeight = iHeight;
	m_decodeHwFmt = decodeHwFmt;
	m_decodeFmt = decodeFmt;
	m_encodeFmt = encodeFmt;
	m_ratRatio = sar;
	m_ratFrameRate = ratFrameRate;
	m_ratTimeBase = ratTimebase;
	m_decodeAVHWDeviceType = decodeHwDeviceType;
}

void CEncode::setHWCodecAndHWConfOfVideo(const std::pair<const AVCodec*, const AVCodecHWConfig*>& pairVideo) 
{
	m_videoCodecConfig = pairVideo;
}

void CEncode::setVideoWidthHeight(int width, int height)
{
	m_iWidth = width;
	m_iHeight = height;
}

//void CEncode::setFrameRate(const AVRational& frameRate)
//{
//	m_frameRate = frameRate;
//}

//void CEncode::setDecodecCtxVideo(AVCodecContext* videoCtx, AVCodecContext* audioCtx)
//{
//	m_decodecCtxVideo = videoCtx;
//	m_decodecCtxAudio = audioCtx;
//}

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
	int ret;
	if (avformat_alloc_output_context2(&m_fmtCtx, nullptr, nullptr, m_file.c_str()) < 0)
		return false;
	if (!m_fmtCtx->oformat)
		m_fmtCtx->oformat = av_guess_format("mp4", nullptr, nullptr);

	if (!m_fmtCtx->url && !(m_fmtCtx->url = av_strdup(m_file.c_str())))
	{
		printf("av_strdup error\n");
		return false;
	}
	auto fun = [this](std::pair<const AVCodec*, const AVCodecHWConfig*> CEncode::* pCodecConf, AVCodecID idDefault, AVPixelFormat accptPix, AVHWDeviceType type)
	{
		if (!(this->*pCodecConf).first)
		{
			AVCodecID id;
			if ((!m_fmtCtx->oformat) || m_fmtCtx->oformat->video_codec == AV_CODEC_ID_NONE)
				id = idDefault;
			else
			{
				if (&(this->*pCodecConf) == &m_videoCodecConfig)
					id = m_fmtCtx->oformat->video_codec;
				else
					assert(0);
			}
			this->*pCodecConf = CSelCodec::getHWEncodeCodecByStreamIdAndAcceptPixFmt(id, accptPix, type);
		}
	};
	if ((m_flags & ENCODE_VIDEO) && m_fmtCtx->oformat->video_codec != AV_CODEC_ID_NONE)
	{

		if (m_decodeAVHWDeviceType != AV_HWDEVICE_TYPE_NONE
			&& m_pAVBufferRef != nullptr
			&& m_decodeHwFmt != AV_PIX_FMT_NONE)
		{
			fun(&CEncode::m_videoCodecConfig, m_fmtCtx->oformat->video_codec, m_decodeHwFmt, m_decodeAVHWDeviceType);
			assert(m_videoCodecConfig.first != nullptr && m_videoCodecConfig.second != nullptr);
		}	
		else
		{
			m_pAVBufferRef = nullptr;
			fun(&CEncode::m_videoCodecConfig, m_fmtCtx->oformat->video_codec, m_decodeHwFmt, m_decodeAVHWDeviceType);
		}
		m_codecVideo = m_videoCodecConfig.first;
		if (!m_codecVideo)
		{
			m_codecVideo = avcodec_find_encoder(m_fmtCtx->oformat->video_codec);
			if (!m_codecVideo)
				return false;
		}
		m_codecCtxVideo = avcodec_alloc_context3(m_codecVideo);
		if (m_codecCtxVideo)
		{
			std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*&)>>freem_codecVideoCtx(m_codecCtxVideo, [](AVCodecContext*& p)
				{
					avcodec_free_context(&p);
				});
			if(m_iVideoBitRate != 0)
				m_codecCtxVideo->bit_rate = m_iVideoBitRate;
			m_codecCtxVideo->framerate = m_ratFrameRate;
			m_codecCtxVideo->time_base = m_ratTimeBase;
			m_codecCtxVideo->codec_type = AVMEDIA_TYPE_VIDEO;
			m_codecCtxVideo->width = m_iWidth;
			m_codecCtxVideo->height = m_iHeight;
			m_codecCtxVideo->sample_aspect_ratio = m_ratRatio;
			m_codecCtxVideo->gop_size = (int)m_ratFrameRate.num/ m_ratFrameRate.den;
			if (m_videoCodecConfig.second)
			{
				if (m_pAVBufferRef != nullptr
					&& m_decodeAVHWDeviceType != AV_HWDEVICE_TYPE_NONE
					&& m_decodeHwFmt != AV_PIX_FMT_NONE)
				{
					m_codecCtxVideo->hw_frames_ctx = av_buffer_ref(m_pAVBufferRef);
					m_codecCtxVideo->pix_fmt = m_decodeHwFmt;//((AVHWFramesContext*)m_decodecCtxVideo->hw_frames_ctx->data)->format;
				}
				else 
				{
					m_codecCtxVideo->pix_fmt = m_videoCodecConfig.second->pix_fmt;
					AVBufferRef* buf = nullptr;
					if ((ret = av_hwdevice_ctx_create(&buf, m_videoCodecConfig.second->device_type, nullptr, nullptr, 0)) != 0) 
					{
						char szErr[AV_ERROR_MAX_STRING_SIZE];
						PrintE("av_hwdevice_ctx_create error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
						//PRINT_MSG("av_hwdevice_ctx_create error:%s\n",ret);

						return false;
					}
					std::unique_ptr<AVBufferRef, std::function<void(AVBufferRef*&)>>rel_free(buf, [](AVBufferRef*& f) {av_buffer_unref(&f); });
					m_codecCtxVideo->hw_frames_ctx = av_hwframe_ctx_alloc(buf);
					AVHWFramesContext*pFrameContext = (AVHWFramesContext*)m_codecCtxVideo->hw_frames_ctx->data;
					pFrameContext->width = m_iWidth;
					pFrameContext->height = m_iHeight;
					pFrameContext->initial_pool_size = 20;
					pFrameContext->format = m_videoCodecConfig.second->pix_fmt;
					const AVPixelFormat* ptmp = m_videoCodecConfig.first->pix_fmts;
					while (ptmp) 
					{
						AVPixelFormat fmt = *ptmp;
						if (fmt == AV_PIX_FMT_NONE)
							break;
						++ptmp;
					}
					pFrameContext->sw_format = AV_PIX_FMT_YUV420P;//m_videoCodecConfig.second->pix_fmt;// *m_videoCodecConfig.first->pix_fmts;//AV_PIX_FMT_YUV420P;// m_decodeFmt;//m_decodecCtxVideo->pix_fmt;
					if ((ret = av_hwframe_ctx_init(m_codecCtxVideo->hw_frames_ctx)) != 0)
					{
						//PRINT_MSG("av_hwframe_ctx_init error:%s\n", ret);
						char szErr[AV_ERROR_MAX_STRING_SIZE];
						PrintE("av_hwframe_ctx_init error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
						//av_buffer_unref(&buf);
						av_buffer_unref(&m_codecCtxVideo->hw_frames_ctx);
						return false;
					}
					//av_buffer_unref(&buf);
				}
				if (!initHWFrame())
					return false;
			}
			else
			{
				const enum AVPixelFormat* pFormat = m_codecVideo->pix_fmts;
				for (; *pFormat != AV_PIX_FMT_NONE;) 
				{
					if (*pFormat == m_decodeFmt)
						break;
				}
				if(*pFormat != AV_PIX_FMT_NONE)
					m_codecCtxVideo->pix_fmt = *pFormat;
				else 
					m_codecCtxVideo->pix_fmt = *m_codecVideo->pix_fmts;
			}
			if (m_codecCtxVideo->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
				/* just for testing, we also add B-frames */
				m_codecCtxVideo->max_b_frames = 2;
			}
			if (m_codecCtxVideo->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
				/* Needed to avoid using macroblocks in which some coeffs overflow.
				 * This does not happen with normal video, it just happens here as
				 * the motion of the chroma plane does not match the luma plane. */
				m_codecCtxVideo->mb_decision = 2;
			}

			if (m_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
				m_codecCtxVideo->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if ((ret = avcodec_open2(m_codecCtxVideo, m_codecVideo, nullptr)) != 0)
			{
				//PRINT_MSG("avcodec_open2 error %s\n", ret);
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("avcodec_open2 error:%s", av_make_error_string(szErr, sizeof(szErr), ret));

			}
			else
			{
				if (m_videoStream = avformat_new_stream(m_fmtCtx, m_codecVideo))
					if (avcodec_parameters_from_context(m_videoStream->codecpar, m_codecCtxVideo) >= 0)
					{
						m_videoStream->time_base = { 1, AV_TIME_BASE };
						m_videoStream->id = m_fmtCtx->nb_streams - 1;
						freem_codecVideoCtx.release();
						m_bFlushVideo = false;
					}
			}
		}
	}
	if ((m_flags & ENCODE_AUDIO) && m_fmtCtx->oformat->audio_codec)
	{
		if (!m_codecAudio)
		{
			m_codecAudio = avcodec_find_encoder(m_fmtCtx->oformat->audio_codec);
			if (!m_codecAudio)
				m_codecAudio = avcodec_find_encoder(AV_CODEC_ID_AAC);
		}

		m_codecCtxAudio = avcodec_alloc_context3(m_codecAudio);
		if (m_codecCtxAudio)
		{
			std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext*&)>>free_codecAudioCtx(m_codecCtxAudio, [](AVCodecContext*& p) {avcodec_free_context(&p); });
			initSuitableAudioCodec();
			if (m_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
				m_codecCtxAudio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			if (avcodec_open2(m_codecCtxAudio, m_codecAudio, nullptr) == 0)
			{
				if (m_codecAudio->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
					m_codecCtxAudio->frame_size = 10000;

				if (m_audioStream = avformat_new_stream(m_fmtCtx, m_codecAudio))
				{
					if (avcodec_parameters_from_context(m_audioStream->codecpar, m_codecCtxAudio) >= 0)
					{
						m_audioStream->time_base = { 1, m_codecCtxAudio->sample_rate };
						m_audioStream->id = m_fmtCtx->nb_streams - 1;
						free_codecAudioCtx.release();
						m_bFlushAudio = false;
					}
				}
			}
		}
	}
	if (m_codecCtxAudio || m_codecCtxVideo) 
	{
		if (!(m_fmtCtx->flags & AVFMT_NOFILE))
		{
			if ((ret = avio_open(&m_fmtCtx->pb, m_file.c_str(), AVIO_FLAG_WRITE)) < 0)
			{
				//PRINT_MSG("avio_open error:%s\n", ret);
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("avio_open error:%s", av_make_error_string(szErr, sizeof(szErr), ret));

				reset();
				return false;
			}
		}
		if ((ret = avformat_write_header(m_fmtCtx, nullptr)) < 0)
		{
			//PRINT_MSG("avformat_write_header error:%s\n", ret);
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

void CEncode::initSuitableAudioCodec()
{
	if (!m_codecAudio || !m_codecCtxAudio)
		return;


	//const uint64_t*pLayouts = m_codecAudio->channel_layouts;
	const AVChannelLayout* pLayouts = m_codecAudio->ch_layouts;
	//m_codecCtxAudio->channel_layout = av_get_default_channel_layout(m_iChannels);
	//m_codecCtxAudio->channels = m_iChannels;
	bool bFind = false;
	const AVChannelLayout* pSelChannelLayout = NULL, *lowest = NULL;
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
				if(!lowest)
					lowest = pLayouts;
			}
		}
	}
	if (!bFind) 
	{
		pSelChannelLayout = lowest;
	}
	
	if (pSelChannelLayout)
	{
		m_codecCtxAudio->ch_layout = *pSelChannelLayout;
		if (!bFind)
		{
			m_iChannels = m_codecCtxAudio->ch_layout.nb_channels;
			m_u64ChannelLayout = m_codecCtxAudio->ch_layout.u.mask;
		}
	}
	else 
	{
		m_codecCtxAudio->ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
		m_codecCtxAudio->ch_layout.u.mask = AV_CH_LAYOUT_STEREO;
		m_codecCtxAudio->ch_layout.nb_channels = m_iChannels;
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
	

	m_codecCtxAudio->sample_fmt = m_sampleFmt;

	const AVSampleFormat*pAVSampleFormat = m_codecAudio->sample_fmts;
	for (; *pAVSampleFormat != AVSampleFormat::AV_SAMPLE_FMT_NONE; ++pAVSampleFormat)
	{
		if (m_codecCtxAudio->sample_fmt == *pAVSampleFormat)
			break;
	}
	if (*pAVSampleFormat == AVSampleFormat::AV_SAMPLE_FMT_NONE)
		m_codecCtxAudio->sample_fmt = *m_codecAudio->sample_fmts;

	m_codecCtxAudio->sample_rate = m_iAudioSampleRate;
	int chooseSuitSamples = *m_codecAudio->supported_samplerates;
	const int* tmpSampleRate = m_codecAudio->supported_samplerates;
	for (; *tmpSampleRate != 0; ++tmpSampleRate)
		if (*tmpSampleRate == m_codecCtxAudio->sample_rate)
			break;
		else if(std::abs(*tmpSampleRate - m_codecCtxAudio->sample_rate) < std::abs(chooseSuitSamples - m_codecCtxAudio->sample_rate))
			chooseSuitSamples = *tmpSampleRate;

	if (*tmpSampleRate == 0)
		m_codecCtxAudio->sample_rate = chooseSuitSamples;
	m_codecCtxAudio->time_base = {1,m_codecCtxAudio->sample_rate };
}

bool CEncode::pushFrame(AVFrame* f, CEncode::STREAM_TYPE type)
{
	switch (type) 
	{
		case ENCODE_VIDEO:
			m_listVideoFrame.push_back(FRAME(f));
			if (!f)
				m_bFlushVideo = true;
			break;
		case ENCODE_AUDIO:
		{
			if (f)
				m_listAudioFrame.push_back(FRAME(f));
			else
			{
				m_listAudioFrame.push_back(FRAME(nullptr));
				m_bFlushAudio = true;
			}
		}
		break;
		default:
			av_frame_unref(f);
			av_frame_free(&f);
			break;
	}
	return true;
}

std::vector<std::pair<FRAME, AVCodecContext*>>CEncode::getFrames() 
{
	std::vector<std::pair<FRAME, AVCodecContext*>>sets;
	auto iterVideo = m_listVideoFrame.begin();
	auto iterAudio = m_listAudioFrame.begin();

	auto MoveAudioToList = [&]()
	{
		sets.push_back({ std::move(*iterVideo++), m_codecCtxVideo });
		assert(iterVideo == m_listVideoFrame.end());
		std::back_insert_iterator<std::remove_reference<decltype(sets)>::type>iter(sets);
		std::for_each(std::make_move_iterator(iterAudio), std::make_move_iterator(m_listAudioFrame.end()), [&iter, this](decltype(m_listAudioFrame)::value_type&& ele)
			{
				*iter++ = std::make_pair<std::remove_reference<decltype(sets)>::type::value_type::first_type, std::remove_reference<decltype(sets)>::type::value_type::second_type>(std::move(ele),std::move(m_codecCtxAudio));
			});
		iterAudio = m_listAudioFrame.end();
	};
	auto MoveVideoToList = [&]() 
	{
		sets.push_back({ std::move(*iterAudio++), m_codecCtxAudio });
		assert(iterAudio == m_listAudioFrame.end());
		MY_ITER<std::remove_reference<decltype(sets)>::type>iter(sets, std::move(m_codecCtxVideo));
		std::for_each(std::make_move_iterator(iterVideo), std::make_move_iterator(m_listVideoFrame.end()), [&iter, this](decltype(m_listVideoFrame)::value_type&& ele)
			{
				*iter++ = std::move(ele);
			});
		iterVideo = m_listVideoFrame.end();
	};
	while (iterVideo != m_listVideoFrame.end() && iterAudio != m_listAudioFrame.end()) 
	{
		if (*iterVideo && *iterAudio) 
		{
			if (av_rescale_q_rnd((*iterVideo)->pts, m_codecCtxVideo->time_base, { 1,1 }, AVRounding::AV_ROUND_NEAR_INF)
				<= av_rescale_q_rnd((*iterAudio)->pts, m_codecCtxAudio->time_base, { 1,1 }, AVRounding::AV_ROUND_NEAR_INF))
				sets.push_back({ std::move(*iterVideo++), m_codecCtxVideo });
			else
				sets.push_back({ std::move(*iterAudio++), m_codecCtxAudio });
		}
		else
		{
			if (!*iterVideo)
				MoveAudioToList();
			else if (!*iterAudio) 
				MoveVideoToList();
		}
	}
	if (m_bFlushAudio && m_bFlushVideo) 
	{
		if (iterVideo != m_listVideoFrame.end())
			MoveAudioToList();
		else if (iterAudio != m_listAudioFrame.end())
			MoveVideoToList();
	}

	m_listVideoFrame.erase(m_listVideoFrame.begin(), iterVideo);
	m_listAudioFrame.erase(m_listAudioFrame.begin(), iterAudio);
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
			//PRINT_MSG("av_write_trailer error:%s\n", err);
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

bool CEncode::initHWFrame()
{
	if (m_hw_frame) 
	{
		av_frame_free(&m_hw_frame);
	}
	m_hw_frame = av_frame_alloc();
	int ret = av_hwframe_get_buffer(m_codecCtxVideo->hw_frames_ctx, m_hw_frame, 0);
	if (ret != 0) 
	{
		//PRINT_MSG("av_hwframe_get_buffer error:%s\n", ret);
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_hwframe_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));

		return false;
	}

	AVPixelFormat* fmt = NULL;
	ret = av_hwframe_transfer_get_formats(m_codecCtxVideo->hw_frames_ctx, AVHWFrameTransferDirection::AV_HWFRAME_TRANSFER_DIRECTION_TO, &fmt, 0);
	if (ret != 0)
	{
		//PRINT_MSG("av_hwframe_transfer_get_formats error:%s\n", ret);
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_hwframe_transfer_get_formats error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}

	for (; *fmt != AV_PIX_FMT_NONE; ++fmt) 
	{
		AVPixelFormat tmp = *fmt;
	}
	/*if (ret != 0)
		return false;*/
	return true;
}

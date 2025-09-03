#include "CVideoEncoder.h"
#include "Util/logger.h"
#include "Network/Socket.h"
#include "CSelCodec.h"
#include "Pushstream/some_util_fun.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/pixdesc.h"
#ifdef __cplusplus
}
#endif
int CVideoEncoder::send_Frame(UPTR_FME f, std::vector<UPTR_PKT>& sets)
{
	if (m_bFlushVideo)
	{
		PrintW("video already flushed");
		return 0;
	}
	int ret;
	if (!m_codecCtxVideo)
	{
		std::call_once(*m_call_once, [&ret, this]()
			{
				ret = this->init();
			});
		if (!ret)
			return AVERROR(EINVAL);
	}
	if (f)
	{
		f->pts = av_rescale_q_rnd(f->pts, f->time_base, m_codecCtxVideo->time_base, AVRounding::AV_ROUND_NEAR_INF);
		f->time_base = m_codecCtxVideo->time_base;
	}
	else
		m_bFlushVideo = true;


	AVFrame* tmpf = NULL;
	if (m_codecCtxVideo->hw_frames_ctx && m_hw_frame && f)
	{
		av_frame_unref(m_hw_frame);
		if ((ret = av_hwframe_get_buffer(m_codecCtxVideo->hw_frames_ctx, m_hw_frame, 0)) != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_hwframe_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			av_frame_free(&m_hw_frame);
			return ret;
		}

		if (m_feedPixFmt == AV_PIX_FMT_NONE)
		{
			AVPixelFormat* tmpPixFormat = NULL, *copyPixFormat = NULL;
			if ((ret = av_hwframe_transfer_get_formats(m_codecCtxVideo->hw_frames_ctx, AVHWFrameTransferDirection::AV_HWFRAME_TRANSFER_DIRECTION_TO, &tmpPixFormat, 0)) != 0) 
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_hwframe_transfer_get_formats error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				av_frame_free(&m_hw_frame);
				return ret;
			}
			copyPixFormat = tmpPixFormat;
			for (m_feedPixFmt = *tmpPixFormat; *copyPixFormat != AV_PIX_FMT_NONE; ++copyPixFormat)
			{
				if ((AVPixelFormat)f->format == *copyPixFormat) 
				{
					m_feedPixFmt = *copyPixFormat;
					PrintD("AVPixelFormat matchs av_hwframe_transfer_data");
					break;
				}
			}
			av_freep(&tmpPixFormat);
		}

		if ((AVPixelFormat)f->format != m_feedPixFmt
			|| f->width != m_codecCtxVideo->width
			|| f->height != m_codecCtxVideo->height) 
		{
			UPTR_FME tmpf;
			tmpf.swap(f);
			if ((ret = convert(f, std::move(tmpf))) != 0)
				return ret;
		}

		ret = av_hwframe_transfer_data(m_hw_frame, f.get(), 0);
		if (ret != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_hwframe_transfer_data error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return ret;
		}
		//m_hw_frame->pts = f->pts;
		//m_hw_frame->time_base = f->time_base;
		ret = av_frame_copy_props(m_hw_frame, f.get());
		if (ret != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_frame_copy_props error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return ret;
		}
		tmpf = m_hw_frame;
	}
	else
	{
		if (m_feedPixFmt == AV_PIX_FMT_NONE)
			m_feedPixFmt = m_codecCtxVideo->pix_fmt;

		if (f && ((AVPixelFormat)f->format != m_feedPixFmt
			|| f->width != m_codecCtxVideo->width
			|| f->height != m_codecCtxVideo->height))
		{
			UPTR_FME tmpf;
			tmpf.swap(f);
			if ((ret = convert(f, std::move(tmpf))) != 0)
				return ret;
		}
		tmpf = f.get();
	}

	ret = avcodec_send_frame(m_codecCtxVideo, tmpf);
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
		ret = avcodec_receive_packet(m_codecCtxVideo, rel_pkt.get());
		if (ret == 0)
		{
			rel_pkt->time_base = m_codecCtxVideo->time_base;

			if (rel_pkt->dts == AV_NOPTS_VALUE)
			{
				rel_pkt->dts = rel_pkt->pts;
				PrintW("video frame dts==AV_NOPTS_VALUE", rel_pkt->dts, rel_pkt->pts);
			}
			else if (rel_pkt->dts > rel_pkt->pts)
			{
				rel_pkt->dts = rel_pkt->pts;
				PrintW("video frame dts(%lld) >pts(%lld)", rel_pkt->dts, rel_pkt->pts);
			}
			//av_packet_rescale_ts(rel_pkt.get(), m_codecCtxVideo->time_base, m_videoStream->time_base);
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
			sets.emplace_back(std::move(rel_pkt));
		}
		else if (ret == AVERROR(EAGAIN))
			break;
		else if (ret == AVERROR_EOF)
		{
			printf("avcodec_receive_packet goto end stream id:video");
			break;
		}
	} while (true);
	return ret;
}

CVideoEncoder::~CVideoEncoder()
{
	reset();
}

void CVideoEncoder::reset()
{
	freeCodecCtx();
	m_codecVideo = nullptr;
	m_bFlushVideo = false;

	m_iVideoMaxBitRate = 0;
	m_iVideoMinBitRate = 0;
	m_iWidth = 0;
	m_iHeight = 0;

	m_reqEncodePixFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	m_EncodeAVHWDeviceType = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;
	m_feedPixFmt = AV_PIX_FMT_NONE;

	memset(&m_ratRatio, 0, sizeof(m_ratRatio));
	memset(&m_ratFrameRate, 0, sizeof(m_ratFrameRate));
	memset(&m_ratVideoTimeBase, 0, sizeof(m_ratVideoTimeBase));
	
	m_call_once.reset(new std::once_flag);
	m_record_last_video_dts = AV_NOPTS_VALUE;
	m_video_id = AV_CODEC_ID_NONE;
	m_hw_sets.clear();
	m_con.reset();
}

bool CVideoEncoder::testCodecOfEncoder(const AVCodec* codec, const AVCodecHWConfig* pConfig)
{
	freeCodecCtx();

	if (!initProperty())
		return false;

	return testCodecOfEncoderReal(codec, pConfig);
}

std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>> CVideoEncoder::testCodecOfEncoder(const std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>>& sets)
{
	freeCodecCtx();
	std::vector<std::pair<const AVCodec*, const AVCodecHWConfig*>> res_sets;
	if (!initProperty())
		return res_sets;
	for (auto& ele : sets)
		if (testCodecOfEncoderReal(ele.first, ele.second))
			res_sets.push_back(ele);
	return res_sets;
}

int CVideoEncoder::initVideoCodecCtx(AVFormatContext* ctx, const AVCodec* codec, const AVCodecHWConfig* pConfig)
{
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
				if (fmt == AV_PIX_FMT_NONE || fmt == m_reqEncodePixFmt || m_reqEncodePixFmt == AV_PIX_FMT_NONE)
					break;
				++ptmp;
			}
			if (*ptmp == AV_PIX_FMT_NONE)
			{
				return -1;
			}
			else
			{
				pFrameContext->sw_format = *ptmp;
			}
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
				if (*pFormat == m_reqEncodePixFmt || m_reqEncodePixFmt == AV_PIX_FMT_NONE)
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
		if (ctx && ctx->oformat->flags & AVFMT_GLOBALHEADER)
			codecCtxVideo->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		if (codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_HEVC)
		{
			std::string strPreset = m_opt.av_opt_get("preset", "medium");
			ret = av_opt_set(codecCtxVideo->priv_data, "preset", strPreset.c_str(), 0);
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_opt_set error for preset=%s:%s", strPreset.c_str(), av_make_error_string(szErr, sizeof(szErr), ret));
			}
			std::string strProfile = m_opt.av_opt_get("profile", "main");
			ret = av_opt_set(codecCtxVideo->priv_data, "profile", strProfile.c_str(), 0);
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_opt_set error for profile=%s:%s", strProfile.c_str(), av_make_error_string(szErr, sizeof(szErr), ret));
			}
			if (strncmp(m_codecVideo->name, "hevc_nvenc", sizeof("hevc_nvenc")) == 0
				|| strncmp(m_codecVideo->name, "h264_nvenc", sizeof("h264_nvenc")) == 0)
			{
				//std::string strTune = m_option.av_opt_get("tune", "hq");
				ret = av_opt_set(codecCtxVideo->priv_data, "tune", "hq", 0);
				if (ret != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_opt_set error for tune=hp:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				}
				std::string strRC = m_opt.av_opt_get("rc", "vbr_hq");
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
					PrintE("av_opt_set error for x265-params=%s:%s", ss.str().c_str(), av_make_error_string(szErr, sizeof(szErr), ret));
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
		m_codecCtxVideo = freem_codecVideoCtx.release();
	}
	return 0;
}

AVCodecContext* CVideoEncoder::getCodecCtx()
{
	return m_codecCtxVideo;
}

bool CVideoEncoder::init()
{
	if (!initProperty())
		return false;

	int ret;
	for (auto& ele : m_hw_sets)
	{
		ret = initVideoCodecCtx(NULL, ele.first, ele.second);
		if (ret == -2)
			return false;
		if (ret == 0)
			break;
	}
	return true;
}

bool CVideoEncoder::initProperty()
{
	m_iVideoMinBitRate = m_opt.av_opt_get_int("b:vMin", 2560 * 1024);
	m_iVideoMaxBitRate = m_opt.av_opt_get_int("b:vMax", 5120 * 1024);
	AVRational videosize = m_opt.av_opt_get_q("s", { 1920 ,1080 });
	m_ratRatio = m_opt.av_opt_get_q("aspect", { 1,1 });

	m_video_id = m_opt.av_opt_get_codec_id("codecid:v", AV_CODEC_ID_H265);

	/*if (m_video_id == AV_CODEC_ID_NONE)
	{
		PrintE("init failed.CVideoEncoder needs to set codecid:x.");
		return false;
	}*/

	m_ratVideoTimeBase = m_opt.av_opt_get_q("v:time_base", { 1,30 });
	m_ratFrameRate = m_opt.av_opt_get_q("r", { 30,1 });

	//m_reqEncodePixFmt = m_opt.av_opt_get_pixel_fmt("pix_fmt", AV_PIX_FMT_NONE);
	m_reqEncodePixFmt = m_opt.av_opt_get_pixel_fmt("pix_fmt", AV_PIX_FMT_YUV420P);
	//if (m_encodeFmt == AV_PIX_FMT_NONE)
	//	return false;

	int ret;
	m_iWidth = videosize.num;
	m_iHeight = videosize.den;
	if (m_video_id != AV_CODEC_ID_NONE)
	{
		m_hw_sets = CSelCodec::getHWEncodeCodecByStreamIdAndAcceptPixFmt(m_video_id, true, AV_PIX_FMT_NONE, AV_HWDEVICE_TYPE_NONE);
		if (m_hw_sets.empty())
		{
			PrintE("cant't find video encoder");
			return false;
		}
	}
	return true;
}

bool CVideoEncoder::initHWFrame(AVCodecContext* codec_ctx)
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
	
	AVPixelFormat* copyFmt = fmt;
	for (; *copyFmt != AV_PIX_FMT_NONE; ++copyFmt)
	{
		AVPixelFormat tmp = *copyFmt;
	}
	av_freep(&fmt);
	return true;
}

bool CVideoEncoder::testCodecOfEncoderReal(const AVCodec* codec, const AVCodecHWConfig* pConfig)
{
	int ret = initVideoCodecCtx(NULL, codec, pConfig);
	if (ret < 0)
		return false;
	freeCodecCtx();
	return true;
}

void CVideoEncoder::freeCodecCtx()
{
	if (m_codecCtxVideo)
	{
		if (m_codecCtxVideo->hw_frames_ctx)
			av_buffer_unref(&m_codecCtxVideo->hw_frames_ctx);
		avcodec_free_context(&m_codecCtxVideo);
	}
	if (m_hw_frame)
		av_frame_free(&m_hw_frame);
}

int CVideoEncoder::convert(UPTR_FME &dst, UPTR_FME src)
{
	if (!m_con.init((AVPixelFormat)src->format, src->width, src->height, (AVPixelFormat)m_feedPixFmt, m_codecCtxVideo->width, m_codecCtxVideo->height))
		return AVERROR(EINVAL);
	UPTR_FME f_convert(av_frame_alloc());
	if (!f_convert)
	{
		PrintE("failed av_frame_alloc return NULL");
		return AVERROR(ENOMEM);
	}
	int ret;
	if ((ret = av_frame_copy_props(f_convert.get(), src.get())) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintW("av_frame_copy_props error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	}
	f_convert->width = m_codecCtxVideo->width;
	f_convert->height = m_codecCtxVideo->height;
	f_convert->format = m_feedPixFmt;
	if ((ret = ff_frame_get_buffer(f_convert.get())) != 0)
		return ret;
	if (!m_con.operator()(src->data, src->linesize, f_convert->data, f_convert->linesize))
	{
		PrintE("convert frame error:src format:%s,src width:%d, src height:%d,dst format:%s,dst width:%d,dst height:%d",
			av_get_pix_fmt_name((AVPixelFormat)src->format), src->width, src->height,
			av_get_pix_fmt_name((AVPixelFormat)f_convert->format), f_convert->width, f_convert->height);
		return AVERROR(EINVAL);;
	}
	dst.swap(f_convert);
	return 0;
}

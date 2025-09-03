#include "CFileEncoder.h"
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

void CFileEncoder::setFile(const std::string& pathFile, int flag)
{
	//reset();
	m_file = pathFile;
	m_flags = flag;
}

void CFileEncoder::reset()
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
	CAudioEncoder::reset();
	CVideoEncoder::reset();
	m_listAudioPacket.clear();
	m_listVideoPacket.clear();
	
	m_videoStream = NULL;
	m_audioStream = NULL;
	m_iNeedWriteTail = 0;
	m_fileEncoderInitCallOnce.reset(new std::once_flag);

}

bool CFileEncoder::send_Frame(UPTR_FME f, CFileEncoder::STREAM_TYPE type)
{
	if ((m_bFlushAudio && type == CFileEncoder::STREAM_TYPE::ENCODE_AUDIO)
		|| (m_bFlushVideo && type == CFileEncoder::STREAM_TYPE::ENCODE_VIDEO))
	{
		PrintE("%s already flushed", type == STREAM_TYPE::ENCODE_AUDIO ? "AUDIO" : "VIDEO");
		return false;
	}
	if (!m_codecCtxAudio && !m_codecCtxVideo)
	{
		bool b = false;
		std::call_once(*m_fileEncoderInitCallOnce.get(), [&b, this]()
			{
				b = this->init();
			});
		if (!b)
			return false;
	}

	int ret;
	std::vector<UPTR_PKT>sets;
	if (type == CFileEncoder::STREAM_TYPE::ENCODE_VIDEO)
	{
		ret = CVideoEncoder::send_Frame(std::move(f), sets);
		if (ret == AVERROR_EOF) 
			setStreamEndFlag(CFileEncoder::STREAM_TYPE::ENCODE_VIDEO);
		std::transform(std::make_move_iterator(sets.begin()), std::make_move_iterator(sets.end()), std::back_insert_iterator<decltype(m_listVideoPacket)>(m_listVideoPacket), [this](decltype(m_listVideoPacket)::value_type&& ele)
			{
				av_packet_rescale_ts(ele.get(), ele->time_base, m_videoStream->time_base);
				ele->time_base = m_videoStream->time_base;
				ele->stream_index = m_videoStream->id;
				//printf("video time = %f\n",1.0* ele->pts* ele->time_base.num/ ele->time_base.den);
				return std::move(ele);
			});
	}
	else if (type == CFileEncoder::STREAM_TYPE::ENCODE_AUDIO) 
	{
		ret = CAudioEncoder::send_Frame(std::move(f), sets);
		if (ret == AVERROR_EOF)
			setStreamEndFlag(CFileEncoder::STREAM_TYPE::ENCODE_AUDIO);
		std::transform(std::make_move_iterator(sets.begin()), std::make_move_iterator(sets.end()), std::back_insert_iterator<decltype(m_listAudioPacket)>(m_listAudioPacket), [this](decltype(m_listAudioPacket)::value_type&& ele)
			{
				av_packet_rescale_ts(ele.get(), ele->time_base, m_audioStream->time_base);
				ele->time_base = m_audioStream->time_base;
				ele->stream_index = m_audioStream->id;
				//printf("audio time = %f\n", 1.0 * ele->pts * ele->time_base.num / ele->time_base.den);
				return std::move(ele);
			});

	}

	sets = getPacket();

	for (auto& ele : sets)
	{
		//int ret = av_interleaved_write_frame(m_fmtCtx, ele.get());
		int ret = av_write_frame(m_fmtCtx, ele.get());

		//double ts = (1.0)*ele->dts * ele->time_base.num / ele->time_base.den;
		//printf("%s send dts = %f\n", ele->stream_index == 0 ?"video":"audio", ts);
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


int CFileEncoder::initVideoCodecCtxStream(AVFormatContext*& ctx, const AVCodec* codec, const AVCodecHWConfig* pConfig)
{
	if (!(m_flags & ENCODE_VIDEO))
		return 0;
	int ret;
	if ((ret = CVideoEncoder::initVideoCodecCtx(ctx, codec, pConfig)) == 0)
	{
		m_videoStream = avformat_new_stream(ctx, m_codecVideo);
		if (!m_videoStream)
		{
			avcodec_free_context(&m_codecCtxVideo);
			m_codecCtxVideo = NULL;
			return -2;
		}
		if ((ret = avcodec_parameters_from_context(m_videoStream->codecpar, m_codecCtxVideo)) >= 0)
		{
			m_videoStream->time_base = { 1, AV_TIME_BASE };
			m_videoStream->id = ctx->nb_streams - 1;
			m_videoStream->codecpar->codec_tag;
			m_bFlushVideo = false;
			ret = 0;
		}
		else
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("avcodec_parameters_from_context error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			avcodec_free_context(&m_codecCtxVideo);
			m_codecCtxVideo = NULL;
		}
	}
	return ret;
}

int CFileEncoder::initAudioCodecCtxStream(AVFormatContext*&ctx)
{
	if (!(m_flags & ENCODE_AUDIO))
		return 0;

	int ret;
	
	if ((ret = initAudioCodecCtx(ctx)) != 0)
		return ret;

	m_audioStream = avformat_new_stream(ctx, m_codecAudio);
	if (!m_audioStream)
	{
		avcodec_free_context(&m_codecCtxAudio);
		m_codecCtxAudio = NULL;
		return -2;
	}
	if ((ret=avcodec_parameters_from_context(m_audioStream->codecpar, m_codecCtxAudio)) >= 0)
	{
		m_audioStream->time_base = { 1, m_codecCtxAudio->sample_rate };
		m_audioStream->id = ctx->nb_streams - 1;
		m_audioStream->codecpar->codec_tag;
		m_bFlushAudio = false;
	}
	else 
	{
		avcodec_free_context(&m_codecCtxAudio);
		m_codecCtxAudio = NULL;
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avcodec_parameters_from_context error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return -2;
	}
	return 0;
}

AVCodecContext* CFileEncoder::getEncCtx(CFileEncoder::STREAM_TYPE type)
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

bool CFileEncoder::init()
{
	if ((m_flags & (ENCODE_VIDEO | ENCODE_AUDIO)) == 0)
	{
		PrintE("file doesn't set ENCODE_VIDEO or ENCODE_AUDIO flag");
		return false;
	}

	if ((m_flags & ENCODE_AUDIO) && !CAudioEncoder::initProperty())
		return false;
	if ((m_flags & ENCODE_VIDEO) && !CVideoEncoder::initProperty())
		return false;
	
	int ret;
	AVFormatContext* fmtCtx = NULL;
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
		if (m_video_id == AV_CODEC_ID_NONE) 
		{
			m_video_id = oformat->video_codec;
			m_hw_sets = CSelCodec::getHWEncodeCodecByStreamIdAndAcceptPixFmt(m_video_id, true, AV_PIX_FMT_NONE, AV_HWDEVICE_TYPE_NONE);
			if (m_hw_sets.empty())
			{
				PrintE("cant't find video encoder");
				return false;
			}
		}

		for (auto& ele : m_hw_sets)
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
				continue;
			}
			if (ret == 0)
				break;
		}
		if (fmtCtx == NULL || m_codecCtxVideo == NULL)
		{
			if(fmtCtx)
				avformat_free_context(fmtCtx);
			PrintE("init video failed");
			return false;
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

std::vector<UPTR_PKT> CFileEncoder::getPacket()
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
					*iter++ = std::move(ele);
				});
			iterAudio = m_listAudioPacket.end();
		};

	auto MoveVideoToList = [&]()
		{
			assert(iterAudio == m_listAudioPacket.end());
			std::back_insert_iterator<std::remove_reference<decltype(sets)>::type>iter(sets);
			std::for_each(std::make_move_iterator(iterVideo), std::make_move_iterator(m_listVideoPacket.end()), [&iter, this](decltype(m_listVideoPacket)::value_type&& ele)
				{
					*iter++ = std::move(ele);
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

	return sets;
}

void CFileEncoder::setStreamEndFlag(CFileEncoder::STREAM_TYPE type)
{
	m_iNeedWriteTail |= type;
}

int CFileEncoder::writeTail()
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

int CFileEncoder::writeTailIfAllStreamEnd()
{
	if (m_flags == m_iNeedWriteTail)
		return writeTail();
	return 0;
}


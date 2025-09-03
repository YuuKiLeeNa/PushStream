#include "CEncodeHelp.h"
#include "Util/logger.h"
#include "Network/Socket.h"
#include<string.h>
#include"utilfun.h"
#include<sstream>
#include<iomanip>

#include "Pushstream/some_util_fun.h"

//
//std::string GetExePath()
//{
//	wchar_t szFilePath[MAX_PATH + 1] = { 0 };
//	std::string strPath2 = "";
//	GetModuleFileNameW(NULL, szFilePath, MAX_PATH);
//	auto iter = std::find(std::rbegin(szFilePath), std::rend(szFilePath), L'\\');
//	int len = lstrlenW(szFilePath);
//	if (iter != std::rend(szFilePath))
//	{
//		len = iter.base() - std::begin(szFilePath);
//	}
//	std::wstring wpath(szFilePath, len);
//	std::string strPath = wchar_t2uft8(wpath.c_str());// .substr(0, 24);
//	return strPath;
//}

std::string GetTimeStr() 
{
	time_t now = time(0);
	tm* local_time = localtime(&now);

	int year = 1900 + local_time->tm_year;
	int month = 1 + local_time->tm_mon;
	int day = local_time->tm_mday;
	int hour = local_time->tm_hour;
	int min = local_time->tm_min;
	int sec = local_time->tm_sec;
	
	char szstr[32];
	if (snprintf(szstr, sizeof(szstr), "%04d_%02d_%02d_%02d_%02d_%02d", year, month, day, hour, min, sec) < 0)
	{
		std::stringstream ss;
		ss << std::setw(4) << year << std::setw(1) << "_";
		ss << std::setw(2) << std::setfill('0') << month << std::setw(1) << "_";
		ss << std::setw(2) << std::setfill('0') << day << std::setw(1) << "_";
		ss << std::setw(2) << std::setfill('0') << hour << std::setw(1) << "_";
		ss << std::setw(2) << std::setfill('0') << min << std::setw(1) << "_";
		ss << std::setw(2) << std::setfill('0') << sec;
		return ss.str();
	}
	return std::string(szstr);
}


std::string GetTimeNamePathFileName() 
{
	std::string file = GetExePath() + GetTimeStr()+".mp4";
	return file;
}

CEncodeHelp::CEncodeHelp() {}

CEncodeHelp::~CEncodeHelp()
{
	{
		std::lock_guard<std::mutex>lock(m_mutex);
		m_bStop = true;
	}
	if (m_th.joinable())
	{
		m_th.join();
	}
	m_encode.reset();
}

void CEncodeHelp::setEnableAdjustTime(bool b)
{
	std::lock_guard<std::mutex>lock(m_mutex);
	m_bEnableAdjustTime = b;
	m_pts_adjust = AV_NOPTS_VALUE;
}

void CEncodeHelp::startRecord(const std::string& pathFileName, int flag)
{
	EndRecord();
	if (!m_tmp)
		m_tmp.reset(av_frame_alloc());
	m_pathFileName = pathFileName;
	if (!m_pathFileName.empty())
		m_encode.setFile(m_pathFileName, flag);
	else
		m_encode.setFile(GetTimeNamePathFileName(), flag);

	m_flag = m_encode.getFlag();
	m_video_timebase = m_option.av_opt_get_q("v:time_base", { 1,30 });
	m_video_framerate = m_option.av_opt_get_q("r", { 30,1 });

	//AVHWDeviceType hwdevice_type = (AVHWDeviceType)m_option.av_opt_get_int("hwaccel", AV_HWDEVICE_TYPE_CUDA);
	//m_encode.setAudioBitRate(audioBitrate);
	//m_encode.setAudioInfo(48000, 2, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP);
	//m_encode.setVideoBitRate(2048 * 1024);
	//m_encode.setVideoInfo(AV_HWDEVICE_TYPE_CUDA, NULL, 1920, 1080, AV_PIX_FMT_NONE, AV_PIX_FMT_NONE, AV_PIX_FMT_NONE, { 1,1 }, m_video_framerate, m_video_timebase);
	
	m_bStop = false;
	setEnableAdjustTime(true);
	/*m_bEnableAdjustTime = true;
	m_pts_adjust = AV_NOPTS_VALUE;*/
	m_th = std::thread(&CEncodeHelp::thread_fun, this);
}

void CEncodeHelp::push(AVFrame* f, ENCODER_TYPE::STREAM_TYPE type)
{
	AVFrame* p = NULL;
	if (ff_frame_alloc_ref(&p, f) < 0)
		return;
	UPTR_FME fdst(p);
	push(std::move(fdst), type);
}

void CEncodeHelp::push(UPTR_FME f, ENCODER_TYPE::STREAM_TYPE type)
{
	std::unique_lock<std::mutex>lock(m_mutex);
	if (m_bStop)
		return;

	if (!(m_flag & type))
		return;

	if (m_bEnableAdjustTime)
	{
		if (m_pts_adjust == AV_NOPTS_VALUE 
			&& type != ENCODER_TYPE::STREAM_TYPE::ENCODE_VIDEO
			&& m_flag & ENCODER_TYPE::STREAM_TYPE::ENCODE_VIDEO)
			return;

		else if (m_flag & ENCODER_TYPE::STREAM_TYPE::ENCODE_VIDEO && m_pts_adjust == AV_NOPTS_VALUE && type == CEncode::STREAM_TYPE::ENCODE_VIDEO)
		{
			if (f->pts != AV_NOPTS_VALUE)
			{
				m_pts_adjust = av_rescale_q(f->pts, f->time_base, AV_TIME_BASE_Q);
				f->pts = 0;
				f->time_base = m_video_timebase;
				av_frame_unref(m_tmp.get());
				if (ff_frame_ref(m_tmp.get(), f.get()) < 0)
					return;
			}
			else
				return;
		}
		else if (type == ENCODER_TYPE::STREAM_TYPE::ENCODE_VIDEO)
		{
			f->pts = av_rescale_q_rnd(f->pts, f->time_base, m_video_timebase, AVRounding::AV_ROUND_INF);
			f->time_base = m_video_timebase;
			int64_t t = av_rescale_q_rnd(m_pts_adjust, AV_TIME_BASE_Q, f->time_base, AVRounding::AV_ROUND_INF);
			f->pts -= t;
			if (f->pts < 0)
			{
				return;
			}
			int64_t iExpectPts = av_rescale_q_rnd(m_video_framerate.den * AV_TIME_BASE / m_video_framerate.num, AV_TIME_BASE_Q, f->time_base, AVRounding::AV_ROUND_INF);
			int64_t pts = m_tmp->pts + iExpectPts;
			if (f->pts < m_tmp->pts)
			{
				//PrintW("input video pts=%lld < previous frame pts=%lld, frame dropped", fdst->pts, m_tmp->pts);
				f->pts = m_tmp->pts;
				m_tmp = std::move(f);
				return;
			}
			else if (f->pts < pts + iExpectPts)
			{
				//PrintI("OK input pts=%lld ,except pts=%d", fdst->pts, pts);
				f->pts = pts;
				av_frame_unref(m_tmp.get());
				if (ff_frame_ref(m_tmp.get(), f.get()) < 0)
					return;
			}
			else
			{
				PrintI("input video frame pts too large,it inserts some frame");
				m_tmp->pts += iExpectPts;
				for (; f->pts - m_tmp->pts >= 2 * iExpectPts; m_tmp->pts += iExpectPts)
				{
					AVFrame* fptr;
					if (ff_frame_alloc_ref(&fptr, m_tmp.get()) < 0)
						continue;
					m_list.emplace_back(UPTR_FME(fptr), type);
				}
				f->pts = m_tmp->pts;
				av_frame_unref(m_tmp.get());

				if (ff_frame_ref(m_tmp.get(), f.get()) < 0)
					return;
			}
			/*if (m_tmp->pts == AV_NOPTS_VALUE)
			{
				av_frame_unref(m_tmp.get());
				av_frame_ref(m_tmp.get(), fdst);
				av_frame_copy_props(m_tmp.get(), fdst);
			}*/
			//printf("stream id=%d  pts=%lld\n", (int)type, fdst->pts);
		}
		else if(type == CEncode::STREAM_TYPE::ENCODE_AUDIO)
		{
			if (!(m_flag & CEncode::STREAM_TYPE::ENCODE_VIDEO) && m_pts_adjust == AV_NOPTS_VALUE) 
				m_pts_adjust = av_rescale_q(f->pts, f->time_base, AV_TIME_BASE_Q);

			int64_t t = av_rescale_q_rnd(m_pts_adjust, AV_TIME_BASE_Q, f->time_base, AVRounding::AV_ROUND_INF);
			f->pts -= t;
			if (f->pts < 0)
				return;

			if(m_audio_previous_frame_pts != AV_NOPTS_VALUE)
			{
				if (f->pts != m_audio_previous_frame_pts + f->nb_samples)
				{
					int64_t diff = m_audio_previous_frame_pts - f->pts + f->nb_samples;
					int64_t diff_adjust = av_rescale_q_rnd(diff, f->time_base, AV_TIME_BASE_Q, AVRounding::AV_ROUND_INF);
					PrintI("m_pts_adjust(%lld) adjust -%lld\n", m_pts_adjust, diff_adjust);
					m_pts_adjust -= diff_adjust;
					f->pts = m_audio_previous_frame_pts + f->nb_samples;
				}
			}
			m_audio_previous_frame_pts = f->pts;
		}
	}
	m_list.emplace_back(std::move(f), type);
	m_con.notify_one();
}

void CEncodeHelp::EndRecord()
{
	m_bStop = true;
	m_bEnableAdjustTime = false;
	m_pts_adjust = AV_NOPTS_VALUE;
	m_audio_previous_frame_pts = AV_NOPTS_VALUE;
	if (m_th.joinable())
	{
		m_con.notify_one();
		m_th.join();
		std::unique_lock<std::mutex>lock(m_mutex);
		for (auto& ele : m_list)
			m_encode.send_Frame(std::move(ele.first), ele.second);
		m_encode.send_Frame(NULL, decltype(m_encode)::STREAM_TYPE::ENCODE_VIDEO);
		m_encode.send_Frame(NULL, decltype(m_encode)::STREAM_TYPE::ENCODE_AUDIO);
		m_encode.reset();
	}
}

void CEncodeHelp::setOption(const CMediaOption& opt)
{
	m_option = opt;
	m_encode.setOption(opt);
}

CMediaOption CEncodeHelp::getOption()
{
	return m_option;
}

void CEncodeHelp::thread_fun()
{
	std::unique_lock<std::mutex>lock(m_mutex);
	while (!m_bStop)
	{
		m_con.wait(lock, [this]()
			{
				return (m_bEnableAdjustTime && m_pts_adjust != AV_NOPTS_VALUE && !m_list.empty())
				|| (!m_bEnableAdjustTime && !m_list.empty())
			|| m_bStop;
			});

		for (auto& ele : m_list)
		{
			m_encode.send_Frame(std::move(ele.first), ele.second);
		}
		m_list.clear();
	}
}

void CEncodeHelp::send_frame()
{
	for (auto& ele : m_list)
		if (ele.first)
			m_encode.send_Frame(std::move(ele.first), ele.second);
}

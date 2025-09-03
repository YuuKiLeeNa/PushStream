#pragma once
#include<thread>
#include<condition_variable>
#include<mutex>
#include<atomic>
#include"CEncode.h"
#include<list>
#include"CMediaOption.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include<libavutil/frame.h>
#ifdef __cplusplus
}
#endif
#include "Pushstream/define_type.h"
#include "CFileEncoder.h"

typedef CFileEncoder ENCODER_TYPE;


class CEncodeHelp
{
public:
	CEncodeHelp();
	~CEncodeHelp();
	void setEnableAdjustTime(bool b = true);
	//void setSaveMediaFilePath(const std::string& pathFileName) { m_pathFileName = pathFileName; }
	void startRecord(const std::string& pathFileName = "", int flag = 3);
	void push(AVFrame* f, ENCODER_TYPE::STREAM_TYPE type);
	void push(UPTR_FME f, ENCODER_TYPE::STREAM_TYPE type);
	void EndRecord();
	void setOption(const CMediaOption &opt);
	CMediaOption getOption();
protected:
	void thread_fun();
	void send_frame();
protected:
	CMediaOption m_option;
	//CEncode m_encode;
	ENCODER_TYPE  m_encode;
	std::list<std::pair<UPTR_FME, ENCODER_TYPE::STREAM_TYPE>>m_list;
	std::condition_variable m_con;
	std::mutex m_mutex;
	bool m_bStop = true;
	std::thread m_th;
	bool m_bEnableAdjustTime = false;
	int64_t m_pts_adjust = AV_NOPTS_VALUE;
	AVRational m_video_timebase;
	AVRational m_video_framerate;
	UPTR_FME m_tmp;
	std:: string m_pathFileName;
	int m_flag;
	int64_t m_audio_previous_frame_pts = AV_NOPTS_VALUE;
};



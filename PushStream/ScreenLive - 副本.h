#pragma once
#include "net/EventLoop.h"
#include<memory>
#include<vector>
#include<mutex>
#include "filter/AudioMixer.h"
#include "filter/VideoMixer.h"
//#include "AudioCapture/AudioCapture.h"
#include "VideoCapture/VideoCapture.h"
#include "VideoCapture/DXGIScreenCapture.h"
#include<atomic>
#include"AACEncoder.h"
#include "H264Encoder.h"
#ifdef __cplusplus
extern "C" {
#endif
#include"libavutil/channel_layout.h"
#include "libavutil/pixfmt.h"
#ifdef __cplusplus
}
#endif
#include<atomic>
#include<thread>
#include "xop/RtmpPublisher.h"
#include<string>
#include<condition_variable>
#include<xcall_once.h>
#include "AVPublishTime.h"
#include "CEncodeHelp.h"
#include"CSource.h"


class AudioCapture;
class VideoCapture;
class Camera;
class RenderCameraWidget;

template<typename T>struct freeType
{
	void operator()(T* p) { p->Destroy(); delete p; }
};

template<typename T>using UPTR = std::unique_ptr<T, freeType<T>>;



class ScreenLive
{
public:
	ScreenLive();
	~ScreenLive();
	void addRemCamera(RenderCameraWidget*camera, bool bisadd = true);
	void setDesktopWidgetWidthHeight(int w,int h);
	
	std::pair<int,int>scale_accord(int childW,int childH, int parentFrameW, int parentFrameH, int parentW,int parentH);
	void setUrl(const std::string& url) { url_ = url; }
	void resetRecord();
	void resetpushStream();

	void startRecord();
	void startPushStream();
	void stopRecord();
	void stopPushStream();

protected:
	int start();
	void stop();
	void audio_thread();
	void video_thread();

	int initVideoFilter();
	int initAudioFilter();
	void setMediaInfo();

protected:
	int initPushStreamProtocol();
	int initRecord();
	void initPushStreamProtocol_init_once();
	void initRecord_init_once();
protected:
	std::unique_ptr<std::once_flag>m_once_flat_stream_protocol;
	std::unique_ptr<std::once_flag>m_once_flat_record;
	std::mutex m_mutexStreamProtocol;
	std::mutex m_mutexRecord;
	std::atomic_bool m_pushStreamState = false;
	std::atomic_bool m_recordState = false;
	std::atomic_bool m_pushStreamStateIsInit = false;
	std::atomic_bool m_recordStateIsInit = false;
protected:
	std::unique_ptr<xop::EventLoop>event_;

	std::vector<RenderCameraWidget*>vcamera_;

	bool isstart_ = false;

	AudioMixer amixer_;
	VideoMixer vmixer_;
	std::shared_ptr<VideoCapture>ScreenCapture_;

	std::vector<std::pair<PSSource, HANDLE>>AudioDevice_;


	std::map<int, std::function<bool(AVFrame*)>>getvframe_;
	//std::map<int, std::pair<std::function<int(std::shared_ptr<AudioCapture>,AVFrame*,int)>, std::shared_ptr<AudioCapture>>>getaframe_;
	std::mutex mutexVideoSrc_;
	std::mutex mutexAudioSrc_;

	H264Encoder h264_;
	int framerate_ = 30;
	AVPixelFormat vfmt_ = AV_PIX_FMT_YUV420P;

	AACEncoder aac_;
	int samplerate_ = 48000;
	uint64_t ch_layout_ = AV_CH_LAYOUT_STEREO;
	AVSampleFormat afmt_ = AV_SAMPLE_FMT_FLTP;
	int desktop_w_;
	int desktop_h_;
	std::atomic_bool bstop_ = true;
	std::shared_ptr<std::thread> videothread_;
	std::shared_ptr<std::thread> audiothread_;
	std::shared_ptr<xop::RtmpPublisher>rtmp_;
	std::string url_ = "rtmp://127.0.0.1/live/123";
	//////////////////////////////////////////////
	//////////////////////////////////////////////
	std::condition_variable cond_w_audio_video_sync;
	//std::condition_variable cond_w_video_;
	std::mutex mutex_audio_video_sync_;
	//std::mutex mutex_video_;
	std::unique_ptr<std::once_flag> m_once_flag_send_mediainfo;
	unsigned long long pts_video = 0;
	unsigned long long pts_audio = 0;
	//std::shared_ptr<AVPublishTime>m_publishTime;
	CEncodeHelp m_encodeHelp;

};


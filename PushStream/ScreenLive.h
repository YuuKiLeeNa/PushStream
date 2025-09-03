#pragma once
#include<memory>
#include<vector>
#include<mutex>
#include "filter/AudioMixer.h"
#include "filter/VideoMixer.h"
#include "VideoCapture/VideoCapture.h"
#include "VideoCapture/DXGIScreenCapture.h"
#include<atomic>
#include "AACEncoderPushStream.h"
#include "H264EncoderPushStream.h"
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
#include "xop/RtspPusher.h"
#include<string>
#include<condition_variable>
#include<xcall_once.h>
#include "CEncodeHelp.h"
#include"CSource.h"

#include "VideoCapture/camera.hpp"
#include "xop/rtmp.h"
#include<chrono>
#include<functional>
#include<algorithm>
#include<numeric>
#include "CAudioFrameCast.h"
#include<assert.h>
#include<sstream>
#include"define_type.h"
#include "VideoCapture/CaptureEvents.h"


template<typename T>struct freeType
{
	void operator()(T* p) { p->Destroy(); delete p; }
};

template<typename T>using UPTR = std::unique_ptr<T, freeType<T>>;

template<typename UI>
class ScreenLive:CaptureEvents
{
public:
	using CHILD_TYPE = UI;
	ScreenLive();
	~ScreenLive();
	//void addRemCamera(RenderCameraWidget*camera, bool bisadd);
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
	//void video_thread();
	void video_thread_openGL();

	int initVideoFilter();
	int initAudioFilter(const std::vector<PSSource>&sets);
	void setMediaInfo();

protected:
	int initPushStreamProtocol();
	int initRecord();
	void initPushStreamProtocol_init_once();
	void initRecord_init_once();
protected:
	//CaptureEvents
	void Restart(CaptureEvents::CaptureEventsType type, int index) override;
	void Close(CaptureEvents::CaptureEventsType type, int index) override;
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
	//std::unique_ptr<xop::EventLoop>event_;

	//std::vector<RenderCameraWidget*>vcamera_;

	bool isstart_ = false;

	AudioMixer amixer_;
	VideoMixer vmixer_;
	std::shared_ptr<VideoCapture>ScreenCapture_;

	//std::vector<HANDLE>>AudioDevice_;

	std::map<int, std::function<bool(AVFrame*)>>getvframe_;
	//std::map<int, std::pair<std::function<int(std::shared_ptr<AudioCapture>,AVFrame*,int)>, std::shared_ptr<AudioCapture>>>getaframe_;
	std::mutex mutexVideoSrc_;
	std::mutex mutexAudioSrc_;

	H264EncoderPushStream h264_;
	int framerate_ = 25;
	AVPixelFormat vfmt_ = AV_PIX_FMT_YUV420P;

	AACEncoderPushStream aac_;
	int samplerate_ = 48000;
	uint64_t ch_layout_ = AV_CH_LAYOUT_STEREO;
	AVSampleFormat afmt_ = AV_SAMPLE_FMT_FLTP;
	int desktop_w_;
	int desktop_h_;
	std::atomic_bool bstop_ = true;
	std::shared_ptr<std::thread> videothread_;
	std::shared_ptr<std::thread> audiothread_;

	std::shared_ptr<xop::RtmpPublisher>rtmp_;
	std::shared_ptr<xop::RtspPusher>rtsp_;

	//std::string url_ = "rtmp://127.0.0.1/live/123";
	std::string url_ = "rtsp://127.0.0.1/live/123";
	//////////////////////////////////////////////
	//////////////////////////////////////////////
	//std::condition_variable cond_w_audio_video_sync;
	//std::condition_variable cond_w_video_;
	std::mutex mutex_audio_video_sync_;
	//std::mutex mutex_video_;
	std::unique_ptr<std::once_flag> m_once_flag_send_mediainfo;
	unsigned long long pts_video = 0;
	unsigned long long pts_audio = 0;
	//std::shared_ptr<AVPublishTime>m_publishTime;
	CEncodeHelp m_encodeHelp;
	int64_t m_audio_previous_frame_pts = AV_NOPTS_VALUE;
	uint64_t m_init_time = os_gettime_ns();;
};


template<typename UI>
ScreenLive<UI>::ScreenLive() :ScreenCapture_(new DXGIScreenCapture(this), freeType<DXGIScreenCapture>())
{
	
	//AudioMic_->Init(WASAPICapture::CAPTURE_TYPE::MICROPHONE);
	//AudioDesk_->Init(WASAPICapture::CAPTURE_TYPE::DESKTOP);
	/*m_ScreenCapture->setCaptureFrameCallBack([this](std::shared_ptr<uint8_t>& bgra_image, const uint32_t& width, const uint32_t& height)
		{
			emit signalsFrameUpdate(bgra_image, width, height);
		});*/
	ScreenCapture_->Init();
	ScreenCapture_->CaptureStarted();

}

template<typename UI>
ScreenLive<UI>::~ScreenLive()
{
	stop();
}

//template<typename UI>
//void ScreenLive<UI>::addRemCamera(RenderCameraWidget* camera, bool bisadd)
//{
//	auto iter = std::find(vcamera_.begin(), vcamera_.end(), camera);
//	if (iter != vcamera_.end())
//	{
//		if (!bisadd)
//			vcamera_.erase(iter);
//	}
//	else if (bisadd)
//		vcamera_.push_back(camera);
//}

template<typename UI>
void ScreenLive<UI>::setDesktopWidgetWidthHeight(int w, int h)
{
	desktop_w_ = w;
	desktop_h_ = h;
}

template<typename UI>
std::pair<int, int>ScreenLive<UI>::scale_accord(int childW, int childH, int parentFrameW, int parentFrameH, int parentW, int parentH)
{
	return { av_rescale(parentFrameW, childW, parentW), av_rescale(parentFrameH, childH, parentH) };
}

template<typename UI>
void ScreenLive<UI>::resetRecord()
{
	m_recordState = false;
	m_recordStateIsInit = false;
	std::lock_guard<std::mutex>lock(m_mutexRecord);
	m_once_flat_record.reset();
}

template<typename UI>
void ScreenLive<UI>::resetpushStream()
{
	m_pushStreamState = false;
	m_pushStreamStateIsInit = false;
	std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
	m_once_flat_stream_protocol.reset();
}

template<typename UI>
void ScreenLive<UI>::startRecord()
{
	if (!bstop_)
	{
		std::lock_guard<std::mutex>lock(m_mutexRecord);
		m_once_flat_record.reset(new std::once_flag);
	}
	//m_encodeHelp.startRecord();
	m_recordStateIsInit = false;
	m_recordState = true;
	if (bstop_)
		start();
}

template<typename UI>
void ScreenLive<UI>::startPushStream()
{
	if (!bstop_)
	{
		std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
		m_once_flat_stream_protocol.reset(new std::once_flag);
	}
	m_pushStreamStateIsInit = false;
	m_pushStreamState = true;
	if (bstop_)
		start();
}

template<typename UI>
void ScreenLive<UI>::stopRecord()
{
	if (!m_pushStreamState)
		stop();
	else
	{
		m_recordState = false;
		m_encodeHelp.EndRecord();
		{
			std::lock_guard<std::mutex>lock(m_mutexRecord);
			m_once_flat_record.reset();
		}
		m_recordStateIsInit = false;
	}
}

template<typename UI>
void ScreenLive<UI>::stopPushStream()
{
	if (!m_recordState)
		stop();
	else
	{
		m_pushStreamState = false;
		if (rtmp_)
		{
			rtmp_->Close();
			rtmp_.reset();
		}
		//event_.reset();
		{
			std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
			m_once_flat_stream_protocol.reset();
		}
		m_pushStreamStateIsInit = false;
	}
}

template<typename UI>
int ScreenLive<UI>::start()
{
	if (!ScreenCapture_)
		return -1;
	if (!bstop_)
		return 0;
	bstop_ = false;

	m_once_flat_stream_protocol.reset(new std::once_flag);
	m_once_flat_record.reset(new std::once_flag);
	m_once_flag_send_mediainfo.reset(new std::once_flag);

	audiothread_ = std::shared_ptr<std::thread>(new std::thread(&ScreenLive::audio_thread, this), [](std::thread* t) {if (t&&t->joinable())t->join(); delete t; });
	//videothread_ = std::shared_ptr<std::thread>(new std::thread(&ScreenLive::video_thread, this), [](std::thread* t) {if (t->joinable())t->join(); delete t; });
	videothread_ = std::shared_ptr<std::thread>(new std::thread(&ScreenLive::video_thread_openGL, this), [](std::thread* t) {if (t&&t->joinable())t->join(); delete t; });

	if (!audiothread_ || !videothread_)
	{
		stop();
		return -1;
	}
	return 0;
}

template<typename UI>
void ScreenLive<UI>::stop()
{
	bstop_ = true;
	//cond_w_audio_video_sync.notify_all();
	//cond_w_video_.notify_all();
	if (rtmp_)
	{
		rtmp_->Close();
		rtmp_.reset();
	}
	if (rtsp_)
	{
		rtsp_->Close();
		rtsp_.reset();
	}

	if (videothread_ && videothread_->joinable())
	{
		if (videothread_->get_id() != std::this_thread::get_id())
		{
			videothread_->join();
			videothread_.reset();
		}
	}
	if (audiothread_ && audiothread_->joinable())
	{
		std::vector<PSSource>AudioSources = ((UI*)this)->GetPSSource();
		for (auto& ele : AudioSources)
			SetEvent(ele->getDataLengthNoticeSignal());
		if (audiothread_->get_id() != std::this_thread::get_id())
		{
			audiothread_->join();
			audiothread_.reset();
		}
	}
	{
		std::unique_lock<std::mutex>lock(mutexVideoSrc_);
		getvframe_.clear();
	}
	vmixer_.reset();
	{
		//std::unique_lock<std::mutex>lock(mutexAudioSrc_);
		//getaframe_.clear();
	}
	amixer_.reset();
	//event_.reset();
	m_encodeHelp.EndRecord();
	m_pushStreamState = false;
	m_pushStreamStateIsInit = false;
	m_recordState = false;
	m_recordStateIsInit = false;
	m_once_flat_stream_protocol.reset();
	m_once_flat_record.reset();
	m_once_flag_send_mediainfo.reset();
	m_audio_previous_frame_pts = AV_NOPTS_VALUE;
}

template<typename UI>
void ScreenLive<UI>::audio_thread()
{
	//{
		//std::unique_lock<std::mutex>lock(mutex_audio_video_sync_);
		/*if (aac_.init(samplerate_, ch_layout_, afmt_) < 0)
		{
			bstop_ = true;
			qDebug() << "aac_.init failed";
			return;
		}*/

		/*if (initAudioFilter() < 0)
		{
			bstop_ = true;
			qDebug() << "initAudioFilter failed";
			return;
		}*/
		//qDebug() << "initAudioFilter success";
		//cond_w_audio_video_sync.notify_one();
		//cond_w_audio_video_sync.wait(lock, [this]()
		//	{
		//		return h264_.isinited() | bstop_;
		//	});
		////qDebug() << "audio thread: cond_w_video_ return";
		//cond_w_audio_video_sync.notify_one();
	//}
	//qDebug() << "sync success";
	int frame_len = (0x7FF + 1) / 2;//aac_.framelen();
	std::function<bool(AVFrame*, int, AVSampleFormat, AVChannelLayout*)> initframe = [frame_len](AVFrame* f, int sampleRate, AVSampleFormat fmt, AVChannelLayout* chlay)->bool
		{
			//if(f == NULL)
			//f = av_frame_alloc();
			f->sample_rate = sampleRate;
			f->ch_layout = *chlay;
			f->format = fmt;
			f->nb_samples = frame_len;
			f->time_base = { 1, sampleRate };
			f->pts = 0;
			int err = av_frame_get_buffer(f, 0);
			if (err != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), err));
				return false;
			}
			err = av_frame_make_writable(f);
			if (err != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_frame_make_writable error:%s", av_make_error_string(szErr, sizeof(szErr), err));
				return false;
			}
			return true;
		};


	
	
	if (bstop_)
		return;
	//std::call_once(*m_once_flag_send_mediainfo.get(), [this] {setMediaInfo(); });
	//qDebug() << "call_once success";
	int ret = 0;
	//std::vector<UPTR_PKT>vpkts;
	std::vector<std::shared_ptr<xop::AVFrame>>vpkts;
	AVFrame* sinktmpf = av_frame_alloc();
	UPTR_FME autofree_sinktmpf(sinktmpf);


	std::vector<PSSource>audioSource_sets_record = ((UI*)this)->GetPSSource();
	int source_size = audioSource_sets_record.size();
	std::vector<HANDLE>handle;
	handle.reserve(source_size);
	for (auto& ele : audioSource_sets_record)
		handle.push_back(ele->getDataLengthNoticeSignal());


	//设置所有音频源起始时间为0，队列会自动丢弃数据,并重置数据可读信号源
	for (int i = 0; i < source_size; ++i)
	{
		audioSource_sets_record[i]->setAudioTs(0);
		ResetEvent(handle[i]);
	}

	if (initAudioFilter(audioSource_sets_record) < 0)
	{
		bstop_ = true;
		PrintE("initAudioFilter failed");
		//qDebug() << "initAudioFilter failed";
		return;
	}
	std::vector<UPTR_FME>ptrFrameAutoFreeSet;

	ptrFrameAutoFreeSet.reserve(source_size);
	for (int i = 0; i < source_size; ++i) 
	{
		AVFrame*tmpframe = av_frame_alloc();
		if (!tmpframe) 
		{
			PrintE("av_frame_alloc failed:no momery");
			return;
		}
		ptrFrameAutoFreeSet.emplace_back(tmpframe);
	}

	uint64_t minTs = os_gettime_ns();
	while (!bstop_)
	{
		/*Sleep(1000);
		continue;*/

		std::vector<PSSource>audioSource_tmp = ((UI*)this)->GetPSSource();
		//去掉已经停止的线程
		audioSource_tmp.erase(std::remove_if(audioSource_tmp.begin(), audioSource_tmp.end(), [](PSSource& ele)->bool
			{
				return ele->isExit() == 1;
			}), audioSource_tmp.end());
		

		bool isChanged = true;
		int tmp_audio_size = audioSource_tmp.size();
		//判断音频源是否改变
		if (tmp_audio_size == audioSource_sets_record.size())
		{
			int i = 0;
			for (; i < tmp_audio_size; ++i) {
				if (audioSource_tmp[i] != audioSource_sets_record[i])
					break;
			}
			if (i == tmp_audio_size)
				isChanged = false;
		}
		//如果改变重新初始化音频混音过滤器
		if (isChanged) 
		{
			audioSource_sets_record = audioSource_tmp;
			source_size = tmp_audio_size;

			int icurPtrFrameAutoFreeSetSize = ptrFrameAutoFreeSet.size();
			if (source_size > icurPtrFrameAutoFreeSetSize)
			{
				//ptrFrameAutoFreeSet.resize(source_size);
				int iNeedInput = source_size - icurPtrFrameAutoFreeSetSize;
				ptrFrameAutoFreeSet.reserve(iNeedInput);
				for (int i = 0; i < iNeedInput; ++i)
				{
					AVFrame* tmpframe = av_frame_alloc();
					if (!tmpframe)
					{
						PrintE("av_frame_alloc failed:no momery");
						return;
					}
					ptrFrameAutoFreeSet.emplace_back(tmpframe);
				}
			}
			else
				ptrFrameAutoFreeSet.resize(source_size);

			if (initAudioFilter(audioSource_sets_record) < 0)
			{
				bstop_ = true;
				PrintE("initAudioFilter failed");
				//qDebug() << "initAudioFilter failed";
				return;
			}
			handle.clear();
			handle.reserve(tmp_audio_size);
			for (auto& ele : audioSource_sets_record)
				handle.push_back(ele->getDataLengthNoticeSignal());
		}
		
		if (tmp_audio_size == 0) 
		{
			//没有音频源
			//XXXXXXXXXXXXXXXXXXXXX
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		else
		{
			DWORD waitres = WaitForMultipleObjects(handle.size(), &handle[0], true, INFINITE);
			bool bFailed = true;
			switch (waitres)
			{
			case WAIT_OBJECT_0:
				bFailed = false;
				break;
			case WAIT_OBJECT_0 + 1:
				bFailed = false;
				break;
			case  WAIT_ABANDONED_0:
				bFailed = true;
				break;
			case WAIT_IO_COMPLETION:
				bFailed = true;
				break;
			case WAIT_TIMEOUT:
				bFailed = true;
				break;
			case WAIT_FAILED:
			{
				DWORD error = GetLastError();
				printf("%u\n", error);
			}
			default:
				bFailed = true;
				break;
			};
			if (bFailed)
				continue;
			//assert(w == WAIT_OBJECT_0 + 1);
			if (bstop_)
				break;
			//检查是否有audio线程已经停止，如果有则说明数据可读信号是为了防止阻塞线程而设置的,经过这步所有数据必可读
			auto iter = std::find_if(audioSource_sets_record.begin(), audioSource_sets_record.end(), [](PSSource& ele)->bool
				{
					return ele->isExit() == 1;
				});
			if (iter != audioSource_sets_record.end()) 
			{
				PrintD("some audio threads exits,so just continue: reinit amixer");
				//重新初始化音频混音
				continue;
			}
			
			//取得音频源数据队列起始时间
			bool isSomeAudioSourcesTsZero = false;
			std::vector<uint64_t>AudioTsSets;
			AudioTsSets.reserve(tmp_audio_size);
			for (auto& ele : audioSource_sets_record)
			{
				uint64_t t = ele->getAudioTs();
				//有未初始化队列
				if (t == 0)
				{
					isSomeAudioSourcesTsZero = true;
					break;
				}
				AudioTsSets.push_back(t);
			}

			if (isSomeAudioSourcesTsZero)
				continue;

			auto iterMinMax = std::minmax_element(AudioTsSets.cbegin(), AudioTsSets.cend());
			minTs = *iterMinMax.first;
			int64_t diff_ts = *iterMinMax.second - minTs;
			if (diff_ts != 0)
			{
				////音频源最大和最小时间差大于2S，丢弃所有数据
				if (diff_ts >= 2000000000)
				{
					uint8_t* ff[8];
					memset(ff, 0, sizeof(ff));
					size_t sizeMic, sizeDesk;
					std::vector<size_t>samplesSets(tmp_audio_size);
					std::stringstream ss;
					for (int i = 0; i < tmp_audio_size; ++i)
					{
						audioSource_sets_record[i]->getSamples(ff, 0, &samplesSets[i]);
						ss << samplesSets[i]<<",";
					}
					
					//设置所有音频源起始时间为0，队列会自动丢弃数据,并重置数据可读信号源
					for (int i = 0; i < tmp_audio_size;++i)
					{
						audioSource_sets_record[i]->setAudioTs(0);
						//如果重置的已经停止的线程的可读信号，在线程刚开始会过滤掉停止线程，因此问题不大
						ResetEvent(handle[i]);
					}
		
					std::string str_time = ss.str();
					assert(!str_time.empty());
					str_time = str_time.substr(0, str_time.length() - 1);
					PrintD("audio diff large than 2seconds(%lld),then them drop all data,sources samples:%s", diff_ts, str_time.c_str());
					continue;
				}

				//根据时间差在各个音频源队列插入silence音频数据,填充使得各个音频数据起始时间相同，来同步时间
				
				for (int i = 0; i < tmp_audio_size; ++i)
				{
					uint64_t tmp_diff_ts =  AudioTsSets[i] - minTs;
					if (tmp_diff_ts > 0)
					{
						//根据时间差计算需要填充的数据samples
						uint64_t PadSamples = util_mul_div64(tmp_diff_ts, audioSource_sets_record[i]->get_out_samplerate(), 1000000000ULL);
						if (PadSamples > 0)
						{
							//由于音频源数据队列各个声道分开存储(planar),故channel = 1
							int num_bytes = av_samples_get_buffer_size(NULL, 1, PadSamples, audioSource_sets_record[i]->get_out_format(), 1);
							audioSource_sets_record[i]->push_front_zero_then_set_audio_ts(num_bytes, minTs);
						}
					}
				}
			}
			AVChannelLayout chlay = audioSource_sets_record[0]->get_out_chlay();

			/*for (int i = 0; i < tmp_audio_size; ++i) 
			{
				AVFrame* ftmp = NULL;
				bool b = initframe(ftmp, audioSource_sets_record[i]->get_out_samplerate(), audioSource_sets_record[i]->get_out_format(), &chlay);
				if (!b)
				{
					bstop_ = true;
					return;
				}
				ptrFrameAutoFreeSet[i].reset(ftmp);
			}*/


			/*
			AVFrame* pfdesk = NULL, *pfmic = NULL;
			bool b = initframe(pfdesk, AudioMic_->get_out_samplerate(), AudioMic_->get_out_format(), &chlay);
			if (!b)
			{
				bstop_ = true;
				return;
			}
			fdesk.reset(pfdesk);
			chlay = AudioDesk_->get_out_chlay();
			b = initframe(pfmic, AudioDesk_->get_out_samplerate(), AudioDesk_->get_out_format(), &chlay);
			if (!b)
			{
				bstop_ = true;
				return;
			}
			fmic.reset(pfmic);
			*/

			std::vector<uint64_t>save_ts_sets;
			save_ts_sets.reserve(tmp_audio_size);
			bool isNeedContinue = false;
			for (int i = 0; i < tmp_audio_size; ++i)
			{
				//AVFrame* ftmp = NULL;
				bool b = initframe(ptrFrameAutoFreeSet[i].get(), audioSource_sets_record[i]->get_out_samplerate(), audioSource_sets_record[i]->get_out_format(), &chlay);
				if (!b)
				{
					bstop_ = true;
					return;
				}
				//ptrFrameAutoFreeSet[i].reset(ftmp);


				size_t leftLen = 0;
				uint64_t ts = audioSource_sets_record[i]->getSamples(ptrFrameAutoFreeSet[i]->data, frame_len, &leftLen);
				//未初始化数据队列,或者未期望起始时间
				if (ts == 0 || ts != minTs)
				{
					PrintI("device:%s ts:%lld != expected ts:%lld", audioSource_sets_record[i]->DeviceName().c_str(), ts, minTs);
					ResetEvent(handle[i]);
					isNeedContinue = true;
					//把数据塞回原队列
					while (--i > -1) 
					{
						size_t bytes = frame_len * av_get_bytes_per_sample((AVSampleFormat)ptrFrameAutoFreeSet[i]->format);
						audioSource_sets_record[i]->push_front_data_bytes_then_set_audio_ts(ptrFrameAutoFreeSet[i]->data, bytes, minTs);
					}
					break;
				}
				else if (ts > 0 && leftLen < audioSource_sets_record[i]->getDataLengthSizeNotice())
					ResetEvent(handle[i]);
			}
		}

		if(bstop_)
			break;

		if (tmp_audio_size != 0)
		{
			std::unique_lock<std::mutex>lock(mutexAudioSrc_);
			int framelen = (0x7FF + 1) / 2;//aac_.framelen();
			//pfmic->pts = 0;
			//pfdesk->pts = 0;
			for (int i = 0; i < tmp_audio_size; ++i)
			{
				ptrFrameAutoFreeSet[i]->time_base = { 1, ptrFrameAutoFreeSet[i]->sample_rate };
				ptrFrameAutoFreeSet[i]->pts = util_mul_div64(minTs, samplerate_, 1000000000ULL);
				if ((ret = amixer_.addFrame(i, ptrFrameAutoFreeSet[i].get())) < 0)// || (ret = amixer_.addFrame(1, pfdesk)) < 0)
				{
					bstop_ = true;
					qDebug() << "amixer_.addFrame failed";
					break;
					//av_frame_unref(ptrFrameAutoFreeSet[i].get());
				}
				//else
				//	ptrFrameAutoFreeSet[i].release();
			}

			if (bstop_)
				break;

			av_frame_unref(sinktmpf);
			if ((ret = amixer_.getFrame(sinktmpf, framelen)) != 0)
			{
				if (ret != AVERROR(EAGAIN))
				{
					bstop_ = true;
					qDebug() << "amixer_.getFrame error";
				}
				assert(0);
				//break;
			}
		}
		else//没有音频源 
		{
			/*av_frame_unref(sinktmpf);
			sinktmpf->time_base = { 1, samplerate_ };
			sinktmpf->sample_rate = samplerate_;
			ret = av_channel_layout_from_mask(&sinktmpf->ch_layout, ch_layout_);
			assert(ret == 0);
			sinktmpf->nb_samples = framelen;
			sinktmpf->format = afmt_;
			ret = av_frame_get_buffer(sinktmpf, 0);
			if (ret != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr,sizeof(szErr), ret));
			}
			else
			{
				uint64_t t = os_gettime_ns();
				sinktmpf->pts = 
			}*/
			continue;
		}
		if (bstop_)
			break;
				
		sinktmpf->time_base = { 1, sinktmpf->sample_rate };
		sinktmpf->pts = util_mul_div64(minTs - m_init_time, samplerate_, 1000000000ULL);

		/***debug***************************************/
		if(m_audio_previous_frame_pts != AV_NOPTS_VALUE)
		{
			if (sinktmpf->pts != m_audio_previous_frame_pts + sinktmpf->nb_samples)
				PrintW("sinktmpf->pts(%lld) - previous sinktmpf->pts(%lld)=%lld\n", sinktmpf->pts, m_audio_previous_frame_pts, sinktmpf->pts - m_audio_previous_frame_pts);
		}
		m_audio_previous_frame_pts = sinktmpf->pts;
		/***end********************************************/

		if (m_recordState)
		{
			if (!m_recordStateIsInit)
			{
				initRecord_init_once();
				if (!m_recordStateIsInit)
					m_recordState = false;
				else
				{
					m_encodeHelp.push(sinktmpf, ENCODER_TYPE::STREAM_TYPE::ENCODE_AUDIO);
				}
			}
			else
			{
				m_encodeHelp.push(sinktmpf, ENCODER_TYPE::STREAM_TYPE::ENCODE_AUDIO);
			}
		}
		if (m_pushStreamState)
		{
			if (!m_pushStreamStateIsInit)
			{
				initPushStreamProtocol_init_once();
				if (!m_pushStreamStateIsInit)
					m_pushStreamState = false;

			}
			else
			{
				if ((ret = aac_.send_frame_init(sinktmpf, vpkts)) < 0)
				{
					if (ret != AVERROR(EAGAIN))
					{
						//av_frame_unref(sinktmpf);
						//bstop_ = true;
						qDebug() << "aac_.send_frame_init error";
						//break;
					}
					//assert(vpkts.empty());
					//break;
				}
				//av_frame_unref(sinktmpf);
				for (auto& ele : vpkts)
				{
					if (rtmp_ && rtmp_->PushAudioFrame(ele->buffer.get(), ele->size) < 0)
					{
						bstop_ = true;
						qDebug() << "PushAudioFrame error";
						//av_frame_unref(sinktmpf);
						break;
					}
					if (rtsp_)
					{
						//xop::AVFrame frame(ele->size);
						//memcpy(frame.buffer.get(), ele->data, ele->size);
						//frame.type = xop::FrameType::AUDIO_FRAME;
						//frame.timestamp = xop::AACSource::GetTimestamp(samplerate_);//ele->pts;

						//int64_t ts64= av_rescale_q_rnd(ele->pts, ele->time_base, { 1,1000 }, AVRounding::AV_ROUND_UP);
						//printf("audio ts:%lld ms\n", frame.timestamp);//av_rescale_q(frame.timestamp, ele->time_base, {1,1000}));
						rtsp_->PushFrame(xop::MediaChannelId::channel_1, *ele);
					}


					//qDebug() << "send audio pkt:" << ele->size;
				}
			}
		}
		//av_frame_unref(sinktmpf);
		//std::this_thread::sleep_for(sleeptime);
	}

	vpkts.clear();
}

//
//template<typename UI>
//void ScreenLive<UI>::video_thread()
//{
//	//m_publishTime.setVideoDuring(1.0*1000 / framerate_);
//	//m_publishTime->setVideoThreshold((long long)(1.0 * 1000 / framerate_));
//	{
//		//std::unique_lock<std::mutex>lock(mutex_audio_video_sync_);
//		h264_.setframerate(framerate_);
//		h264_.setgopsize(framerate_);
//		if (h264_.init(ScreenCapture_->GetWidth(), ScreenCapture_->GetHeight(), vfmt_, "libx264") < 0)
//		{
//			bstop_ = true;
//			//qDebug() << "h264_.init error";
//			return;
//		}
//		/*cond_w_audio_video_sync.notify_one();
//		cond_w_audio_video_sync.wait(lock, [this]()
//			{
//				return aac_.isinited() | bstop_;
//			});
//		cond_w_audio_video_sync.notify_one();*/
//	}
//	//qDebug() << "sync success";
//	if (bstop_)
//		return;
//	//std::call_once(*m_once_flag_send_mediainfo.get(), [this] {setMediaInfo(); });
//	//qDebug() << "call_once success";
//	if (initVideoFilter() < 0)
//	{
//		bstop_ = true;
//		qDebug() << "initVideoFilter error";
//		return;
//	}
//	qDebug() << "initVideoFilter success";
//	std::chrono::steady_clock::time_point s = std::chrono::steady_clock::now(), tmpt;
//	const std::chrono::steady_clock::duration sleeptime(std::chrono::steady_clock::duration::period::den / framerate_);
//	std::chrono::steady_clock::duration realSleepTime = sleeptime;
//	int ret = 0;
//	//std::vector<UPTR_PKT>vpkts;
//	std::vector<std::shared_ptr<xop::AVFrame>>vpkts;
//	AVFrame* sinktmpf = av_frame_alloc();
//	UPTR_FME auto_freeSinktmpf(sinktmpf);
//	while (!bstop_)
//	{
//		std::unique_lock<std::mutex>lock(mutexVideoSrc_);
//		for (auto& ele : getvframe_)
//		{
//			if (bstop_)
//				break;
//			AVFrame* tmpf = av_frame_alloc();
//			if (!ele.second(tmpf)
//				|| (ret = vmixer_.addFrame(ele.first, tmpf)) < 0)
//			{
//				//av_frame_unref(tmpf);
//				av_frame_free(&tmpf);
//				bstop_ = true;
//				qDebug() << "vmixer_.addFrame or getframe error";
//				break;
//			}
//		}
//		if (!bstop_)
//		{
//			av_frame_unref(sinktmpf);
//			if ((ret = vmixer_.getFrame(sinktmpf)) < 0)
//			{
//				if (ret != AVERROR(EAGAIN))
//				{
//					bstop_ = true;
//					qDebug() << "vmixer_.getFrame error";
//					break;
//				}
//			}
//			else
//			{
//				if (bstop_)
//					break;
//				uint64_t cur_time = os_gettime_ns();
//
//				uint64_t pts_time = util_mul_div64(cur_time, framerate_, 1000000000ULL);
//				sinktmpf->pts = pts_time;
//				sinktmpf->time_base = { 1, framerate_ };
//
//				if (m_recordState)
//				{
//					if (!m_recordStateIsInit)
//					{
//						initRecord_init_once();
//						if (!m_recordStateIsInit)
//							m_recordState = false;
//						else
//							m_encodeHelp.push(sinktmpf, CEncode::STREAM_TYPE::ENCODE_VIDEO);
//					}
//					else
//						m_encodeHelp.push(sinktmpf, CEncode::STREAM_TYPE::ENCODE_VIDEO);
//				}
//
//				if (m_pushStreamState)
//				{
//					if (!m_pushStreamStateIsInit)
//					{
//						initPushStreamProtocol_init_once();
//						if (!m_pushStreamStateIsInit)
//							m_pushStreamState = false;
//						else
//						{
//
//						}
//					}
//					else
//					{
//						if ((ret = h264_.send_frame_init(sinktmpf, vpkts)) < 0)
//						{
//							if (ret != AVERROR(EAGAIN))
//							{
//								//av_frame_unref(sinktmpf);
//								bstop_ = true;
//								qDebug() << "h264_.send_frame_init error";
//								break;
//							}
//						}
//						for (auto& ele : vpkts)
//						{
//							if (rtmp_ && rtmp_->PushVideoFrame(ele->buffer.get(), ele->size) < 0)
//							{
//								//av_frame_unref(sinktmpf);
//								bstop_ = true;
//								qDebug() << "PushVideoFrame error";
//								break;
//							}
//						}
//					}
//				}
//			}
//			tmpt = std::chrono::steady_clock::now();
//			realSleepTime = sleeptime - (tmpt - s);
//			if (realSleepTime.count() < 0)
//				realSleepTime = realSleepTime.zero();
//			s = tmpt;
//		}
//		std::this_thread::sleep_for(realSleepTime);
//	}
//	vpkts.clear();
//}

template<typename UI>
inline void ScreenLive<UI>::video_thread_openGL()
{
	//{
	//	//std::unique_lock<std::mutex>lock(mutex_audio_video_sync_);
	//	h264_.setframerate(framerate_);
	//	h264_.setgopsize(framerate_);
	//	if (h264_.init(ScreenCapture_->GetWidth(), ScreenCapture_->GetHeight(), vfmt_, "libx264") < 0)
	//	{
	//		bstop_ = true;
	//		return;
	//	}
	//}
	if (bstop_)
		return;
	std::chrono::steady_clock::time_point s = std::chrono::steady_clock::now(), tmpt;
	const std::chrono::steady_clock::duration sleeptime(std::chrono::steady_clock::duration::period::den / framerate_);
	std::chrono::steady_clock::duration realSleepTime = sleeptime;
	int ret = 0;
	//std::vector<UPTR_PKT>vpkts;
	std::vector<std::shared_ptr<xop::AVFrame>>vpkts;
	AVFrame* sinktmpf = NULL;// = av_frame_alloc();
	//UPTR_FME auto_freeSinktmpf(sinktmpf);
	while (!bstop_)
	{
		if (sinktmpf)
			av_frame_free(&sinktmpf);
		sinktmpf = ((UI*)(this))->getFrameYUV420P().release();
		if(sinktmpf)
		{
			if (bstop_)
				break;
			uint64_t cur_time = os_gettime_ns() - m_init_time;
			uint64_t pts_time = util_mul_div64(cur_time, framerate_, 1000000000ULL);
			sinktmpf->pts = pts_time;
			sinktmpf->time_base = { 1, framerate_ };
			if (m_recordState)
			{
				if (!m_recordStateIsInit)
				{
					initRecord_init_once();
					if (!m_recordStateIsInit)
						m_recordState = false;
					else
						m_encodeHelp.push(sinktmpf, ENCODER_TYPE::STREAM_TYPE::ENCODE_VIDEO);
				}
				else
					m_encodeHelp.push(sinktmpf, ENCODER_TYPE::STREAM_TYPE::ENCODE_VIDEO);
			}

			if (m_pushStreamState)
			{
				if (!m_pushStreamStateIsInit)
				{
					initPushStreamProtocol_init_once();
					if (!m_pushStreamStateIsInit)
						m_pushStreamState = false;
					else
					{

					}
				}
				else
				{

					if ((ret = h264_.send_frame_init(sinktmpf, vpkts, {1,90000})) < 0)
					{
						if (ret != AVERROR(EAGAIN))
						{
								//av_frame_unref(sinktmpf);
							bstop_ = true;
							qDebug() << "h264_.send_frame_init error";
							break;
						}
					}
					for (auto& ele : vpkts)
					{
						if (rtmp_)
						{
							if (rtmp_->PushVideoFrame(ele->buffer.get(), ele->size) < 0)
							{
								bstop_ = true;
								qDebug() << "PushVideoFrame error";
								break;
							}
						}
						if (rtsp_)
						{
							if (!rtsp_->PushFrame(xop::MediaChannelId::channel_0, *ele)) 
							{
								bstop_ = true;
								qDebug() << "PushVideoFrame error";
								break;
							}
						}
					}
				}
			}
		}
		tmpt = std::chrono::steady_clock::now();
		realSleepTime = sleeptime - (tmpt - s);
		if (realSleepTime.count() < 0)
			realSleepTime = realSleepTime.zero();
		s = tmpt;
		std::this_thread::sleep_for(realSleepTime);
	}
	if (sinktmpf)
		av_frame_free(&sinktmpf);
	vpkts.clear();
}

template<typename UI>
int ScreenLive<UI>::initVideoFilter()
{
	int w = ScreenCapture_->GetWidth();
	int h = ScreenCapture_->GetHeight();
	std::pair<decltype(getvframe_)::iterator, bool> iter;

	{
		std::unique_lock<std::mutex>lock(mutexVideoSrc_);
		getvframe_.clear();
		iter = getvframe_.insert({ vmixer_.addVideoSource(w
			, h
			, AV_PIX_FMT_YUV420P
			, { framerate_,1 }, { 1,framerate_ }, std::to_string(w), std::to_string(h), "0", "", ""),
			std::bind(&VideoCapture::CaptureFrame, ScreenCapture_, std::placeholders::_1) });
		if (!iter.second)
		{
			getvframe_.clear();
			return false;
		}
	}

	if (desktop_w_ <= 0)
		desktop_w_ = 1920;
	if (desktop_h_ <= 0)
		desktop_h_ = 1080;
	auto scale_fun = std::bind(&ScreenLive::scale_accord, this, std::placeholders::_1, std::placeholders::_2, w, h, desktop_w_, desktop_h_);

	/*for (auto& ele : vcamera_)
	{
		auto c = ele->camera();
		int w = c->GetWidth();
		int h = c->GetHeight();
		QRect rect = ele->geometry();
		auto wh = scale_fun(rect.width(), rect.height());
		auto xy = scale_fun(rect.left(), rect.top());
		{
			std::unique_lock<std::mutex>lock(mutexVideoSrc_);
			iter = getvframe_.insert({ vmixer_.addVideoSource(w, h, AV_PIX_FMT_YUV420P, { framerate_,1 }, { 1, framerate_ }, std::to_string(wh.first), std::to_string(wh.second), std::to_string(0), std::to_string(xy.first), std::to_string(xy.second)),
				std::bind(&VideoCapture::CaptureFrame, c, std::placeholders::_1) });
			if (!iter.second)
			{
				getvframe_.clear();
				return false;
			}
		}
	}*/

	int ret = vmixer_.init();
	if (ret < 0)
	{
		vmixer_.reset();
		std::unique_lock<std::mutex>lock(mutexVideoSrc_);
		getvframe_.clear();
	}
	return ret;
}

template<typename UI>
int ScreenLive<UI>::initAudioFilter(const std::vector<PSSource>& sets)
{
	int ret;
	//std::lock_guard<std::mutex>lock(mutexAudioSrc_);
	amixer_.reset();
	if (sets.empty())
	{
		PrintW("init audio filter warnning:there doesn't have any audio sources");
		return 0;
	}

	amixer_.setOutput(samplerate_, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP);


	std::vector<float> dBSets = ((UI*)this)->getDBSets();

	int iSetsSize = sets.size();
	int iDBSets = dBSets.size();

	if (iDBSets != iSetsSize)
	{
		PrintW("init audio filter warnning:audio sources size:%d != dB sets size:%d", sets.size(), dBSets.size());
		dBSets.resize(iSetsSize, 0.0f);
	}

	for (int i = 0; i < iSetsSize; ++i)
	{
		auto& ele = sets[i];
		amixer_.addInput((uint32_t)ele->get_out_samplerate(), ele->get_out_lay(), ele->get_out_format(), 1.0, dBSets[i]);
	}
	if ((ret = amixer_.init(AudioMixer::MixerMode::longest) != 0))
		amixer_.reset();
	return ret;
}

template<typename UI>
void ScreenLive<UI>::setMediaInfo()
{
	auto copydata = [](std::shared_ptr<uint8_t>& dst, uint32_t& dstsize, const std::vector<uint8_t>& src)
		{
			dstsize = src.size();
			if (dstsize > 0)
			{
				dst.reset((uint8_t*)malloc(dstsize), std::default_delete<uint8_t[]>());
				memcpy(dst.get(), &src[0], dstsize);
			}
		};
	//std::lock(mutex_audio_, mutex_video_);
	std::unique_lock<std::mutex>lock(mutex_audio_video_sync_);
	xop::MediaInfo media_info;
	media_info.video_framerate = h264_.framerate();
	media_info.video_width = h264_.width();
	media_info.video_height = h264_.height();
	auto spspps = h264_.getSpsPps();
	copydata(media_info.sps, media_info.sps_size, spspps.first);
	copydata(media_info.pps, media_info.pps_size, spspps.second);
	media_info.audio_channel = aac_.channels();
	media_info.audio_frame_len = aac_.framelen();
	media_info.audio_samplerate = aac_.samplerate();
	auto extradata = aac_.getextradata();
	copydata(media_info.audio_specific_config, media_info.audio_specific_config_size, extradata);
	rtmp_->SetMediaInfo(media_info);
}

template<typename UI>
int ScreenLive<UI>::initPushStreamProtocol()
{
	h264_.setframerate(framerate_);
	h264_.setgopsize(framerate_);
	if (h264_.init(ScreenCapture_->GetWidth(), ScreenCapture_->GetHeight(), vfmt_, "libx264") < 0)
	{
		bstop_ = true;
		return -1;
	}

	if (aac_.init(samplerate_, ch_layout_, afmt_) < 0)
	{
		bstop_ = true;
		//qDebug() << "aac_.init failed";
		return -1;
	}


	if (rtmp_)
	{
		rtmp_->Close();
		rtmp_.reset();
	}
	if (rtsp_) 
	{
		rtsp_->Close();
		rtsp_.reset();
	}
	/*if (url_.empty())
		return -1;*/

	if (url_.length() < 8)
		return -1;

	/*if (!(event_ = std::make_unique<decltype(event_)::element_type>()))
	{
		stop();
		return -1;
	}*/
	
	
	std::string proto = url_.substr(0,url_.find_first_of(':'));

	if (proto == "rtmp")
	{
		if (!(rtmp_ = decltype(rtmp_)::element_type::Create(/*event_.get()*/)))
		{
			stop();
			return -1;
		}
		std::string status;
		int ret = rtmp_->OpenUrl(url_, 10000, status);
		if (ret < 0)
		{
			stop();
			return -1;
		}
		setMediaInfo();
		//event_.release();
	}
	else if (proto == "rtsp") 
	{
		if (!(rtsp_ = decltype(rtsp_)::element_type::Create(/*event_.get()*/))) 
		{
			stop();
			return -1;
		}
		//std::string status;
		//std::string subfix = rtsp_->GetRtspSuffix();
		std::string subfix;

		
		std::string ip_suffix = url_.substr(sizeof("rtsp://"));

		auto pos = ip_suffix.find_first_of('/');
		if (pos != std::string::npos) 
		{
			subfix = ip_suffix.substr(pos+1);
			if (subfix.empty()) 
			{
				stop();
				return -1;
			}
			pos = subfix.find_first_of('/');
			if (pos != std::string::npos) 
			{
				subfix = subfix.substr(0, pos);
			}
		}
		else 
		{
			stop();
			return -1;
		}

		xop::MediaSession* session = xop::MediaSession::CreateNew(subfix);
		if (!session)
		{
			stop();
			return -1;
		}

		if (!session->AddSource(xop::MediaChannelId::channel_0, xop::H264Source::CreateNew(framerate_)))
		{
			stop();
			return -1;
		}
		AVChannelLayout ch;
		memset(&ch, 0, sizeof(ch));
		if (av_channel_layout_from_mask(&ch, ch_layout_) != 0) 
		{
			stop();
			return -1;
		}
		if (!session->AddSource(xop::MediaChannelId::channel_1, xop::AACSource::CreateNew(samplerate_, ch.nb_channels,false)))
		{
			stop();
			return -1;
		}

		/*xop::MediaSession* session = xop::MediaSession::CreateNew();
		session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
		session->AddSource(xop::channel_1, xop::AACSource::CreateNew(audio_capture_.GetSamplerate(), audio_capture_.GetChannels(), false));
		rtsp_pusher->AddSession(session);*/



		rtsp_->AddSession(session);
		int ret = rtsp_->OpenUrl(url_, 10000);
		if (ret < 0)
		{
			stop();
			return -1;
		}
		//event_.release();
	}
	return 0;
}

template<typename UI>
int ScreenLive<UI>::initRecord()
{
	CMediaOption option = m_encodeHelp.getOption();


	option["b:a"] = 128000;
	option["ar"] = 48000;
	option["channel_layout"] = AV_CH_LAYOUT_STEREO;
	//option["sample_fmt"] = AV_SAMPLE_FMT_FLTP;
	option["sample_fmt"] = AV_SAMPLE_FMT_NONE;
	//option["codecid:a"] = (AVCodecID)AV_CODEC_ID_AAC;
	option["codecid:a"] = (AVCodecID)AV_CODEC_ID_OPUS;

	option["b:vMin"] = 2560 * 1024;
	option["b:vMax"] = 5120 * 1024;
	option["aspect"] = AVRational{ 1,1 };
	option["codecid:v"] = (AVCodecID)AV_CODEC_ID_H265;
	option["s"] = AVRational{ (int)ScreenCapture_->GetWidth() ,(int)ScreenCapture_->GetHeight() };
	option["v:time_base"] = AVRational{ 1,framerate_ };
	option["r"] = AVRational{ framerate_,1 };
	option["pix_fmt"] = AV_PIX_FMT_YUV420P;


	m_encodeHelp.setOption(option);
	m_encodeHelp.startRecord();
	return 0;
}

template<typename UI>
void ScreenLive<UI>::initPushStreamProtocol_init_once()
{
	std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
	std::call_once(*m_once_flat_stream_protocol.get(), [this]()
		{
			int err = initPushStreamProtocol();
			m_pushStreamStateIsInit = err == 0 ? true : false;
		});
}

template<typename UI>
void ScreenLive<UI>::initRecord_init_once()
{
	std::lock_guard<std::mutex>lock(m_mutexRecord);
	std::call_once(*m_once_flat_record.get(), [this]()
		{
			int err = initRecord();
			m_recordStateIsInit = err == 0 ? true : false;
		});
}

template<typename UI>
void ScreenLive<UI>::Restart(CaptureEvents::CaptureEventsType type, int index) 
{
	switch (type) 
	{
	case CaptureEvents::CaptureEventsType::Type_Video:
		ScreenCapture_.reset(new DXGIScreenCapture(this), freeType<DXGIScreenCapture>());
		ScreenCapture_->Init();
		ScreenCapture_->CaptureStarted();
		break;
	case CaptureEvents::CaptureEventsType::Type_Audio:

		break;
	default:
		break;
	}
}

template<typename UI>
void ScreenLive<UI>::Close(CaptureEvents::CaptureEventsType type, int index) 
{

}
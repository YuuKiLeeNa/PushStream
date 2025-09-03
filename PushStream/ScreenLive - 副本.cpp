#include "ScreenLive.h"
//#include "AudioCapture/AudioCapture.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif
#include "VideoCapture/camera.hpp"
#include "RenderCameraWidget.h"
#include "xop/rtmp.h"
#include<chrono>
#include<functional>
#include<algorithm>
#include<numeric>
#include "CAudioFrameCast.h"
#include<assert.h>
#include "AVPublishTime.h"
#include "CWasapi.h"

class free_frame
{
public:
	void operator()(AVFrame* f)
	{
		//av_frame_unref(f);
		av_frame_free(&f);
	};
};

ScreenLive::ScreenLive() :ScreenCapture_(new DXGIScreenCapture(), freeType<DXGIScreenCapture>())
{
	ScreenCapture_->Init();
	//AudioMic_->Init(WASAPICapture::CAPTURE_TYPE::MICROPHONE);
	//AudioDesk_->Init(WASAPICapture::CAPTURE_TYPE::DESKTOP);
	/*m_ScreenCapture->setCaptureFrameCallBack([this](std::shared_ptr<uint8_t>& bgra_image, const uint32_t& width, const uint32_t& height)
		{
			emit signalsFrameUpdate(bgra_image, width, height);
		});*/
	ScreenCapture_->CaptureStarted();

}

ScreenLive::~ScreenLive() 
{
	stop();
}

//void ScreenLive::addRemAudioSrc(std::shared_ptr<AudioCapture>audio, bool bisadd) 
//{
//	auto iter = std::find(vaudio_.begin(), vaudio_.end(), audio);
//	if (iter != vaudio_.end())
//	{
//		if (!bisadd)
//			vaudio_.erase(iter);
//	}
//	else if (bisadd) 
//		vaudio_.push_back(audio);
//}

void ScreenLive::addRemCamera(RenderCameraWidget* camera, bool bisadd)
{
	auto iter = std::find(vcamera_.begin(), vcamera_.end(), camera);
	if (iter != vcamera_.end())
	{
		if (!bisadd)
			vcamera_.erase(iter);
	}
	else if (bisadd)
		vcamera_.push_back(camera);
}

void ScreenLive::setDesktopWidgetWidthHeight(int w, int h) 
{
	desktop_w_ = w;
	desktop_h_ = h;
}



std::pair<int, int>ScreenLive::scale_accord(int childW, int childH, int parentFrameW, int parentFrameH, int parentW, int parentH)
{
	return { av_rescale(parentFrameW, childW, parentW), av_rescale(parentFrameH, childH, parentH) };
}

void ScreenLive::resetRecord()
{
	m_recordState = false;
	m_recordStateIsInit = false;
	std::lock_guard<std::mutex>lock(m_mutexRecord);
	m_once_flat_record.reset();
}

void ScreenLive::resetpushStream() 
{
	m_pushStreamState = false;
	m_pushStreamStateIsInit = false;
	std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
	m_once_flat_stream_protocol.reset();
}

void ScreenLive::startRecord() 
{
	if (!bstop_) 
	{
		std::lock_guard<std::mutex>lock(m_mutexRecord);
		m_once_flat_record.reset(new std::once_flag);
	}
	//m_encodeHelp.startRecord();
	m_recordStateIsInit = false;
	m_recordState = true;
	if(bstop_)
		start();
}

void ScreenLive::startPushStream() 
{
	if (!bstop_) 
	{
		std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
		m_once_flat_stream_protocol.reset(new std::once_flag);
	}
	m_pushStreamStateIsInit = false;
	m_pushStreamState = true;
	if(bstop_)
		start();
}

void ScreenLive::stopRecord() 
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

void ScreenLive::stopPushStream() 
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
		event_.reset();
		{
			std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
			m_once_flat_stream_protocol.reset();
		}
		m_pushStreamStateIsInit = false;
	}
}

int ScreenLive::start()
{
	if (!ScreenCapture_)
		return -1;
	if (!bstop_)
		return 0;
	bstop_ = false;

	/*if (rtmp_)
	{
		rtmp_->Close();
		rtmp_.reset();
	}
	if(!(event_ = std::make_unique<decltype(event_)::element_type>()))
	{
		stop();
		return -1;
	}
	if(!(rtmp_ = decltype(rtmp_)::element_type::Create(event_.get())))
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
	}*/
	//m_encodeHelp.startRecord();
	//m_publishTime->reset();
	m_once_flat_stream_protocol.reset(new std::once_flag);
	m_once_flat_record.reset(new std::once_flag);
	m_once_flag_send_mediainfo.reset(new std::once_flag);
	

	if(!AudioDevice_.empty())
		audiothread_ = std::shared_ptr<std::thread>(new std::thread(&ScreenLive::audio_thread, this), [](std::thread* t) {if (t->joinable())t->join(); delete t; });
	videothread_ = std::shared_ptr<std::thread>(new std::thread(&ScreenLive::video_thread, this), [](std::thread* t) {if (t->joinable())t->join(); delete t; });
	if ((!AudioDevice_.empty() && !audiothread_) || !videothread_)
	{
		stop();
		return -1;
	}
	return 0;
}

void ScreenLive::stop()
{

	bstop_ = true;
	cond_w_audio_video_sync.notify_all();
	//cond_w_video_.notify_all();
	if (videothread_ && videothread_->joinable())
		videothread_->join();
	if (audiothread_ && audiothread_->joinable())
	{
		for (auto& ele : AudioDevice_)
			SetEvent(ele.second);

		audiothread_->join();
	}
	videothread_.reset();
	audiothread_.reset();
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

	if (rtmp_)
	{
		rtmp_->Close();
		rtmp_.reset();
	}
	event_.reset();
	m_encodeHelp.EndRecord();
	m_pushStreamState = false;
	m_pushStreamStateIsInit = false;
	m_recordState = false;
	m_recordStateIsInit = false;
	m_once_flat_stream_protocol.reset();
	m_once_flat_record.reset();
	m_once_flag_send_mediainfo.reset();
}

void ScreenLive::audio_thread() 
{
	{
		//std::unique_lock<std::mutex>lock(mutex_audio_video_sync_);
		if (aac_.init(samplerate_, ch_layout_, afmt_) < 0)
		{
			bstop_ = true;
			qDebug() << "aac_.init failed";
			return;
		}

		if (initAudioFilter() < 0)
		{
			bstop_ = true;
			qDebug() << "initAudioFilter failed";
			return;
		}
		//qDebug() << "initAudioFilter success";
		//cond_w_audio_video_sync.notify_one();
		//cond_w_audio_video_sync.wait(lock, [this]()
		//	{
		//		return h264_.isinited() | bstop_;
		//	});
		////qDebug() << "audio thread: cond_w_video_ return";
		//cond_w_audio_video_sync.notify_one();
	}
	qDebug() << "sync success";
	int frame_len = aac_.framelen();
	std::function<bool(AVFrame*& , int , AVSampleFormat , AVChannelLayout*)> initframe = [frame_len](AVFrame*& f, int sampleRate, AVSampleFormat fmt, AVChannelLayout* chlay)->bool
		{
			//if(f == NULL)
			f = av_frame_alloc();
			f->sample_rate = sampleRate;
			f->ch_layout = *chlay;
			f->format = fmt;
			f->nb_samples = frame_len;
			f->time_base = {1, sampleRate };
			f->pts = 0;
			int err = av_frame_get_buffer(f, 0);
			if (err != 0) 
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), err));
				return false;
			}
			return true;
		};

	
	
	std::unique_ptr<AVFrame, free_frame> fdesk, fmic;
	//AVChannelLayout chlay = AudioMic_->get_out_chlay();
	//make sure AudioMic_ samplerate == AudioDesk_ samplerate;
	//AudioMic_ AVSampleFormat == AudioDesk_ AVSampleFormat;
	//AudioMic_ AVChannelLayout == AudioDesk_ AVChannelLayout;
	/*bool b = initframe(pfdesk, AudioMic_->get_out_samplerate(), AudioMic_->get_out_format(), &chlay);
	if(!b) 
	{
		bstop_ = true;
		return;
	}
	chlay = AudioDesk_->get_out_chlay();
	b = initframe(pfmic, AudioDesk_->get_out_samplerate(), AudioDesk_->get_out_format(), &chlay);
	if (!b)
	{
		bstop_ = true;
		return;
	}*/
	
	if (bstop_)
		return;
	//std::call_once(*m_once_flag_send_mediainfo.get(), [this] {setMediaInfo(); });
	//qDebug() << "call_once success";
	//std::chrono::steady_clock::time_point s = std::chrono::steady_clock::now(), tmpt;
	const std::chrono::steady_clock::duration sleeptime(std::chrono::steady_clock::duration::period::den / 1000);
	//std::chrono::steady_clock::duration realSleepTime = sleeptime;
	int ret = 0;
	std::vector<AVPacket*>vpkts;
	AVFrame* sinktmpf = av_frame_alloc();
	//AudioMic_->OnRestart();
	//AudioDesk_->OnRestart();
	while (!bstop_)
	{
		//std::this_thread::sleep_for(realSleepTime);
		//std::this_thread::sleep_for(sleeptime);
		const HANDLE sigs[] = { m_micNoticeSigal, m_deskNoticeSigal };
		DWORD waitres = WaitForMultipleObjects(_countof(sigs), sigs, true, INFINITE);
		bool bFailed = true;
		switch (waitres)
		{
		case WAIT_OBJECT_0:
			bFailed = false;
			break;
		case WAIT_OBJECT_0+1:
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
			printf("%u\n",error);
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

		//calc sync
		uint64_t micTime = AudioMic_->getAudioTs();
		uint64_t deskTime = AudioDesk_->getAudioTs();
		if (micTime == 0 || deskTime == 0)
			continue;

		int64_t diff_ts = micTime - deskTime;

		if (diff_ts != 0)
		{
			uint64_t abs_diff_ts = std::abs(diff_ts);

			////音频时间差大于2S，丢弃所有数据
			if (abs_diff_ts >= 2000000000)
			{
				//AudioMic_->pop_front_all(NULL);
				//AudioDesk_->pop_front_all(NULL);
				uint8_t* ff[8];
				memset(ff, 0 , sizeof(ff));
				size_t sizeMic, sizeDesk;
				uint64_t t1 = AudioMic_->getSamples(ff, 0, &sizeMic);
				uint64_t t2 = AudioDesk_->getSamples(ff, 0, &sizeDesk);

				AudioMic_->setAudioTs(0);
				ResetEvent(m_micNoticeSigal);
				AudioDesk_->setAudioTs(0);
				ResetEvent(m_deskNoticeSigal);
				//AudioMic_->Stop();
				//AudioDesk_->Stop();
				//AudioMic_->Start();
				//AudioDesk_->Start();
				
				PrintD("microphone audio(%d) - desktop audio(%d) = %lld", sizeMic, sizeDesk,diff_ts);
				continue;
			}

			uint64_t PadSamples = util_mul_div64(abs_diff_ts, diff_ts > 0 ? AudioDesk_->get_out_samplerate() : AudioMic_->get_out_samplerate(), 1000000000ULL);
			/*auto fun_frame_data_set_zero = [](AVFrame* f, int off,int samples)->void
				{
					AVSampleFormat format = (AVSampleFormat)f->format;
					int bytes = av_get_bytes_per_sample((AVSampleFormat)f->format);
					bool bisPlaner = av_sample_fmt_is_planar(format);
					if (bisPlaner)
						memset(f->data[0], 0, bytes * f->nb_samples * f->ch_layout.nb_channels);
					for(int i = 0; i < f->ch_layout.nb_channels; ++i)
				};*/

			if (PadSamples > 0)
			{
				int num_bytes = av_samples_get_buffer_size(NULL, 1, PadSamples, AudioMic_->get_out_format(), 1);
				if (diff_ts > 0)
				{
					AudioMic_->push_front_zero_then_set_audio_ts(num_bytes, deskTime);
					AudioMic_->setAudioTs(deskTime);
				}
				else
				{
					AudioDesk_->push_front_zero_then_set_audio_ts(num_bytes, micTime);
					AudioDesk_->setAudioTs(micTime);
				}
			}
		}

		AVFrame* pfdesk = NULL, * pfmic = NULL;
		AVChannelLayout chlay = AudioMic_->get_out_chlay();
		bool b = initframe(pfdesk, AudioMic_->get_out_samplerate(), AudioMic_->get_out_format(), &chlay);
		if (!b)
		{
			bstop_ = true;
			return;
		}
		//if(fdesk.get() != pfdesk)
		fdesk.reset(pfdesk);
		chlay = AudioDesk_->get_out_chlay();
		b = initframe(pfmic, AudioDesk_->get_out_samplerate(), AudioDesk_->get_out_format(), &chlay);
		if (!b)
		{
			bstop_ = true;
			return;
		}
		//if(fmic.get() != pfmic)
		fmic.reset(pfmic);
		size_t leftLenMic = 0, leftLenDesk = 0;
		uint64_t ts_mic = AudioMic_->getSamples(pfmic->data, frame_len, &leftLenMic);
		uint64_t ts_desk = AudioDesk_->getSamples(pfdesk->data, frame_len , &leftLenDesk);
		int noticeMic = AudioMic_->getDataLengthSizeNotice();
		int noticeDesk = AudioDesk_->getDataLengthSizeNotice();




		if ((ts_mic > 0 && leftLenMic < noticeMic) || ts_mic == 0)
		{
			//printf("mic reset signal\n");
			ResetEvent(m_micNoticeSigal);
		}
		//else 
		//{
		//	printf("got mic data left:%llu\n", leftLenMic);
		//}
		if ((ts_desk > 0 && leftLenDesk < noticeDesk) || ts_desk == 0)
		{
			//printf("desk reset signal\n");
			ResetEvent(m_deskNoticeSigal);
		}
		/*else 
		{
			printf("got desk data left:%llu\n", leftLenDesk);
		}*/

		if (ts_mic != ts_desk) 
		{
			if (ts_mic != 0)
			{
				size_t bytes = frame_len* av_get_bytes_per_sample((AVSampleFormat)fmic->format);
				AudioMic_->push_front_data_bytes_then_set_audio_ts(pfmic->data, bytes, ts_mic);
			}

			if (ts_desk != 0) 
			{
				size_t bytes = frame_len * av_get_bytes_per_sample((AVSampleFormat)fdesk->format);
				AudioDesk_->push_front_data_bytes_then_set_audio_ts(pfdesk->data, bytes, ts_mic);
			}

			PrintD("AudioMic_ getSamples ts != AudioDesk_ getSamples ts  (%lld != %lld)", ts_mic, ts_desk);
			fmic.reset();
			fdesk.reset();
			continue;
		}

		//decltype(AudioDesk_)ptr_tmp;
		//ptr_tmp = diff_ts > 0 ? AudioMic_ : AudioDesk_;

		std::unique_lock<std::mutex>lock(mutexAudioSrc_);
		int framelen = aac_.framelen();
		bool isFrameAdded = false;


		//for (auto& ele : getaframe_)
		//{
			if (bstop_)
				break;
			//AVFrame* tmpf = av_frame_alloc();
			//int getSamples = ele.second.second->GetSamplerate() * 3 / 100;
			//if ((ret = ele.second.first(ele.second.second, tmpf, getSamples/*framelen*/)) == getSamples)//framelen)
			{
				//ele.second.second->read_samples += getSamples;
				//++ele.second.second->read_times;
				//qDebug() << ele.first <<" times:" << ele.second.second->read_times << "\tsamples" << ele.second.second->read_samples;

				pfmic->pts = 0;
				pfdesk->pts = 0;
				if ((ret = amixer_.addFrame(0, pfmic)) < 0)// || (ret = amixer_.addFrame(1, pfdesk)) < 0)
				{
					//av_frame_unref(tmpf);
					//av_frame_free(&tmpf);
					bstop_ = true;
					qDebug() << "amixer_.addFrame failed";
					break;
				}
				fmic.release();
				if ((ret = amixer_.addFrame(1, pfdesk)) < 0)
				{
					//av_frame_unref(tmpf);
					//av_frame_free(&tmpf);
					bstop_ = true;
					qDebug() << "amixer_.addFrame failed";
					break;
				}
				fdesk.release();
				
				isFrameAdded = true;
					//fdesk.release();
					//fmic.release();
				
			}
			/*else
			{
				av_frame_free(&tmpf);
				if (ret < 0)
				{
					bstop_ = true;
					qDebug() << "mic gather data error";
					break;
				}
			}*/
		//}



		if (isFrameAdded && !bstop_)
		{
			while (true)
			{
				if ((ret = amixer_.getFrame(sinktmpf, framelen)) != 0)
				{
					if (ret != AVERROR(EAGAIN))
					{
						bstop_ = true;
						qDebug() << "amixer_.getFrame error";
					}
					break;
				}
				if (bstop_)
					break;
				//sinktmpf->pts = m_publishTime.getAudioPts()* sinktmpf->sample_rate/1000.0;//pts_audio;
				sinktmpf->time_base = { 1, sinktmpf->sample_rate };

				uint64_t PadSamples = util_mul_div64(ts_mic, samplerate_,1000000000ULL);
				sinktmpf->pts = PadSamples;
				/*pts_audio += sinktmpf->nb_samples;
				sinktmpf->pts = pts_audio;*/


				if (m_recordState)
				{
					if (!m_recordStateIsInit)
					{
						initRecord_init_once();
						if (!m_recordStateIsInit)
							m_recordState = false;
						else
						{
							m_encodeHelp.push(sinktmpf, CEncode::STREAM_TYPE::ENCODE_AUDIO);
						}
					}
					else
					{
						m_encodeHelp.push(sinktmpf, CEncode::STREAM_TYPE::ENCODE_AUDIO);
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
								av_frame_unref(sinktmpf);
								//bstop_ = true;
								qDebug() << "aac_.send_frame_init error";
								//break;
							}
							assert(vpkts.empty());
							break;
						}
						//av_frame_unref(sinktmpf);
						for (auto& ele : vpkts)
						{
							if (rtmp_->PushAudioFrame(ele->data, ele->size) < 0)
							{
								bstop_ = true;
								qDebug() << "PushAudioFrame error";
								av_frame_unref(sinktmpf);
								break;
							}
							//qDebug() << "send audio pkt:" << ele->size;
						}
					}
				}
				av_frame_unref(sinktmpf);
				//av_frame_free(&sinktmpf);
			}
		}
		std::this_thread::sleep_for(sleeptime);
	}
	av_frame_unref(sinktmpf);
	av_frame_free(&sinktmpf);
	std::for_each(vpkts.begin(), vpkts.end(), [](decltype(vpkts)::value_type v) {av_packet_unref(v); av_packet_free(&v); });
	vpkts.clear();
}

void ScreenLive::video_thread() 
{
	//m_publishTime.setVideoDuring(1.0*1000 / framerate_);
	//m_publishTime->setVideoThreshold((long long)(1.0 * 1000 / framerate_));
	{
		//std::unique_lock<std::mutex>lock(mutex_audio_video_sync_);
		h264_.setframerate(framerate_);
		h264_.setgopsize(framerate_);
		if (h264_.init(ScreenCapture_->GetWidth(), ScreenCapture_->GetHeight(), vfmt_, "libx264") < 0)
		{
			bstop_ = true;
			//qDebug() << "h264_.init error";
			return;
		}
		/*cond_w_audio_video_sync.notify_one();
		cond_w_audio_video_sync.wait(lock, [this]()
			{
				return aac_.isinited() | bstop_;
			});
		cond_w_audio_video_sync.notify_one();*/
	}
	//qDebug() << "sync success";
	if (bstop_)
		return;
	//std::call_once(*m_once_flag_send_mediainfo.get(), [this] {setMediaInfo(); });
	//qDebug() << "call_once success";
	if (initVideoFilter() < 0)
	{
		bstop_ = true;
		qDebug() << "initVideoFilter error";
		return;
	}
	qDebug() << "initVideoFilter success";
	std::chrono::steady_clock::time_point s = std::chrono::steady_clock::now(),tmpt;
	const std::chrono::steady_clock::duration sleeptime(std::chrono::steady_clock::duration::period::den / framerate_);
	std::chrono::steady_clock::duration realSleepTime = sleeptime;
	int ret = 0;
	std::vector<AVPacket*>vpkts;
	AVFrame* sinktmpf = av_frame_alloc();
	while (!bstop_)
	{
		//std::this_thread::sleep_for(realSleepTime);
		std::unique_lock<std::mutex>lock(mutexVideoSrc_);
		for (auto& ele : getvframe_)
		{
			if (bstop_)
				break;
			AVFrame* tmpf = av_frame_alloc();
			if (!ele.second(tmpf)
				|| (ret = vmixer_.addFrame(ele.first, tmpf)) < 0)
			{
				av_frame_unref(tmpf);
				av_frame_free(&tmpf);
				bstop_ = true;
				qDebug() << "vmixer_.addFrame or getframe error";
				break;
			}
		}
		if (!bstop_)
		{
			//av_frame_unref(sinktmpf);
			if ((ret = vmixer_.getFrame(sinktmpf)) < 0)
			{
				if (ret != AVERROR(EAGAIN))
				{
					bstop_ = true;
					qDebug() << "vmixer_.getFrame error";
					break;
				}
			}
			else
			{
				if (bstop_)
					break;
				uint64_t cur_time = os_gettime_ns();

				uint64_t pts_time = util_mul_div64(cur_time, framerate_, 1000000000ULL);
				sinktmpf->pts = pts_time;
				sinktmpf->time_base = {1, framerate_};

				if (m_recordState) 
				{
					if (!m_recordStateIsInit) 
					{
						initRecord_init_once();
						if (!m_recordStateIsInit)
							m_recordState = false;
						else 
							m_encodeHelp.push(sinktmpf, CEncode::STREAM_TYPE::ENCODE_VIDEO);
					}
					else
						m_encodeHelp.push(sinktmpf, CEncode::STREAM_TYPE::ENCODE_VIDEO);
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
						if ((ret = h264_.send_frame_init(sinktmpf, vpkts)) < 0)
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
							if (rtmp_->PushVideoFrame(ele->data, ele->size) < 0)
							{
								//av_frame_unref(sinktmpf);
								bstop_ = true;
								qDebug() << "PushVideoFrame error";
								break;
							}
							//qDebug() << "send video pkt:" << ele->size;
						}
					}
				}
				av_frame_unref(sinktmpf);
				//av_frame_free(&sinktmpf);
			}

			tmpt = std::chrono::steady_clock::now();
			realSleepTime = sleeptime - (tmpt - s);
			if (realSleepTime.count() < 0)
				realSleepTime = realSleepTime.zero();
			s = tmpt;
		}
		std::this_thread::sleep_for(realSleepTime);
	}
	av_frame_unref(sinktmpf);
	av_frame_free(&sinktmpf);
	std::for_each(vpkts.begin(), vpkts.end(), [](decltype(vpkts)::value_type v) {av_packet_unref(v); av_packet_free(&v); });
	vpkts.clear();
}

int ScreenLive::initVideoFilter()
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

	for (auto& ele : vcamera_)
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
	}
	int ret = vmixer_.init();
	if (ret < 0)
	{
		vmixer_.reset();
		std::unique_lock<std::mutex>lock(mutexVideoSrc_);
		getvframe_.clear();
	}
	return ret;
}

int ScreenLive::initAudioFilter()
{
	int ret;
	//std::lock_guard<std::mutex>lock(mutexAudioSrc_);
	amixer_.reset();
	amixer_.setOutput(samplerate_, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP);

	for (auto& ele : AudioDevice_)
	{
		auto ptr = ele.first;
		amixer_.addInput((uint32_t)ptr->get_out_samplerate(), ptr->get_out_lay(), ptr->get_out_format(), 1.0, 1.0);
	}
	if ((ret = amixer_.init(AudioMixer::MixerMode::longest) != 0))
		amixer_.reset();
	return ret;
}

void ScreenLive::setMediaInfo() 
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

int ScreenLive::initPushStreamProtocol() 
{
	if (rtmp_)
	{
		rtmp_->Close();
		rtmp_.reset();
	}
	if (!(event_ = std::make_unique<decltype(event_)::element_type>()))
	{
		stop();
		return -1;
	}
	if (!(rtmp_ = decltype(rtmp_)::element_type::Create(event_.get())))
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
	return 0;
}

int ScreenLive::initRecord() 
{
	CMediaOption option = m_encodeHelp.getOption();

	option["b:a"] = 128000;
	option["b:v"] = 2048 * 1024;
	option["ar"] = 48000;
	option["channel_layout"] = AV_CH_LAYOUT_STEREO;
	option["sample_fmt"]= AV_SAMPLE_FMT_FLTP;
	option["s"] = AVRational{ 1920 ,1080 };
	option["aspect"] = AVRational{ 1,1 };
	option["codecid:v"] = (AVCodecID)AV_CODEC_ID_H265;
	option["codecid:a"] = (AVCodecID)AV_CODEC_ID_AAC;
	option["v:time_base"] = AVRational{ 1019,30577 };
	option["r"] = AVRational{ 30577,1019 };
	option["pix_fmt"] = AV_PIX_FMT_YUV420P;

	m_encodeHelp.setOption(option);
	m_encodeHelp.startRecord();
	return 0;
}

void ScreenLive::initPushStreamProtocol_init_once() 
{
	std::lock_guard<std::mutex>lock(m_mutexStreamProtocol);
	std::call_once(*m_once_flat_stream_protocol.get(), [this]() 
		{
			int err = initPushStreamProtocol(); 
			m_pushStreamStateIsInit = err == 0 ? true:false;
		});
}

void ScreenLive::initRecord_init_once() 
{
	std::lock_guard<std::mutex>lock(m_mutexRecord);
	std::call_once(*m_once_flat_record.get(), [this]() 
		{
			int err = initRecord(); 
			m_recordStateIsInit = err == 0 ? true: false;
		});
}
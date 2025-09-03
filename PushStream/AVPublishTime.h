#pragma once
#include<chrono>
#include<thread>
#include<atomic>
#include<memory>

template<typename TIME_BASE = std::chrono::microseconds, unsigned OUT_DIVISION = TIME_BASE::period::den>
class X_AVPublishTime
{
public:
	using TIME_BASE_TYPE = TIME_BASE;
	static constexpr unsigned DIVISION = OUT_DIVISION;
	enum PTS_STRATEGY { PTS_STRATEGY_REEAL_TIME, PTS_STRATEGY_RECTIFY};

	/*static X_AVPublishTime& Instance() 
	{
		static X_AVPublishTime pubtime;
		return pubtime;
	}*/

	X_AVPublishTime():X_AVPublishTime(PTS_STRATEGY::PTS_STRATEGY_RECTIFY, true)
	{
	}
	/////////////bEnableCalibrate:设置启用时间校准
	X_AVPublishTime(PTS_STRATEGY type, bool bEnableCalibrate = true) :m_type(type)
	{
		reset();
		m_bStop = false;
		m_thread.reset(new std::thread(&X_AVPublishTime<TIME_BASE, DIVISION>::Calibrate_thread_fun, this));
	}
	~X_AVPublishTime() { m_bStop = true; m_thread.reset(); }
	
	long long getCurrentTime() 
	{
		return std::chrono::duration_cast<TIME_BASE>(std::chrono::system_clock::now().time_since_epoch()).count();// / (long double)DEN;
	}

	void reset()
	{
		m_iStartTime = getCurrentTime();
		m_iAudioMicPts = 0.0;
		m_iAudioDesktopPts = 0.0;
		m_iVideoPts = 0.0;
	}
	//须转换成TIME_BASE单位
	long double getAudioMicPts(long long cur_during)
	{
		return getPts(m_iAudioMicPts, cur_during,&m_iAudioMicDuring, m_iAudioMicThreshold, "mic");
		/*long long t = getCurrentTime();
		if (m_type == PTS_STRATEGY_RECTIFY)
		{
			long long diff = (t - m_iStartTime);
			m_iAudioMicPts += m_iAudioMicDuring;
			if (std::abs(diff - m_iAudioMicPts) > m_iAudioMicThreshold)
			{
				printf("audio time changed:real=%lld    excepted=%lld\n", (long long)(diff), (long long)(m_iAudioMicPts));
				m_iAudioMicPts = diff;
			}
		}
		else
		{
			m_iAudioMicPts = (t - m_iStartTime);
		}
		return (long double)m_iAudioMicPts/DIVISION;*/
	}
	//须转换成TIME_BASE单位
	long double getAudioDesktopPts(long long cur_during)
	{
		return getPts(m_iAudioDesktopPts, cur_during,&m_iAudioDesktopDuring, m_iAudioDesktopThreshold, "desktop audio");
		/*long long t = getCurrentTime();
		if (m_type == PTS_STRATEGY_RECTIFY)
		{
			long long diff = (t - m_iStartTime);
			m_iAudioDesktopPts += m_iAudioDesktopDuring;
			if (std::abs(diff - m_iAudioDesktopPts) > m_iAudioDesktopThreshold)
			{
				printf("audio time changed:real=%lld    excepted=%lld\n", (long long)(diff), (long long)(m_iAudioDesktopPts));
				m_iAudioDesktopPts = diff;
			}
		}
		else
		{
			m_iAudioDesktopPts = (t - m_iStartTime);
		}
		return (long double)m_iAudioDesktopPts/DIVISION;*/
	}

	//须转换成TIME_BASE单位
	long double getVideoPts(long long cur_during)
	{
		return getPts(m_iVideoPts, cur_during,&m_iVideoDuring,m_iVideoThreshold, "video");
		//long long t = getCurrentTime();
		//if (m_type == PTS_STRATEGY_RECTIFY)
		//{
		//	long long diff = (t - m_iStartTime);// / (long double)DEN;
		//	m_iVideoPts += m_iVideoDuring;
		//	if (std::abs(diff - m_iVideoPts) > m_iVideoThreshold)
		//		m_iVideoPts = diff;
		//}
		//else
		//{
		//	m_iVideoPts = (t - m_iStartTime);// / (long double)DEN;
		//}
		//return (long double)m_iVideoPts / DIVISION;
	}

	//须转换成TIME_BASE单位
	long double getPts(long long& pts, long long cur_during, long long* last_during, long threshold, const char* pinfo)
	{
		long long t = getCurrentTime();
		if (m_type == PTS_STRATEGY_RECTIFY)
		{
			long long diff = (t - m_iStartTime);//(long double)DEN;
			pts += *last_during;
			if (std::abs(diff - pts) > threshold)
			{
				printf("%s adjust pts:%s:%lf\n", pinfo, diff - pts > 0 ? "adjust time large":"adjust time small", (diff - pts)/(long double)TIME_BASE_TYPE::period::den);
				pts = diff;
			}
			*last_during = cur_during;
		}
		else
		{
			pts = (t - m_iStartTime);// / (long double)DEN;
		}
		return (long double)pts / DIVISION;
	}


	//须转换成TIME_BASE单位
	void setAudioMicDuring(long long during)
	{
		m_iAudioMicDuring = during;
		//m_iAudioMicThreshold = during / 2;
	}
	//须转换成TIME_BASE单位
	void setAudioMicThreshold(long long Threshold)
	{
		m_iAudioMicThreshold = Threshold;
	}
	//须转换成TIME_BASE单位
	void setAudioDesktopDuring(long long during)
	{
		m_iAudioDesktopDuring = during;
		//m_iAudioDesktopThreshold = during / 2;
	}
	//须转换成TIME_BASE单位
	void setAudioDesktopThreshold(long long Threshold)
	{
		m_iAudioDesktopThreshold = Threshold;
	}
	//须转换成TIME_BASE单位
	void setVideoDuring(long long during)
	{
		m_iVideoDuring = during;
		//m_iVideoThreshold = during / 2;
	}
	void setVideoThreshold(long long during) 
	{
		m_iVideoThreshold = during / 2;
	}
protected:
	void Calibrate_thread_fun() 
	{
		std::chrono::system_clock::time_point t1,t2 = std::chrono::system_clock::now();
		std::chrono::system_clock::time_point::duration d;
		std::chrono::milliseconds tt;
		long double delta = 0.0;
		long double factor = 0.8;
		long long diff;
		while (m_bStop) 
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			t1 = std::chrono::system_clock::now();
			d = t1 - t2;
			tt = std::chrono::duration_cast<std::chrono::milliseconds>(d);
			
			diff = std::chrono::duration_cast<TIME_BASE>(d).count();
			
			if (std::abs(tt.count()) >= 1000) 
			{
				//系统时间已被调整
				diff += (decltype(diff))std::ceil(delta);
				std::atomic_fetch_add_explicit(&m_iStartTime, diff, std::memory_order_acq_rel);
				//std::cout << "warming:system time changed" << std::endl;
			}
			else 
			{
				if (std::abs(delta - 0.0) < 0.000001)
				{
					delta = diff;
				}
				else
					delta = diff * factor + 0.2 * delta;
			}
			t2 = t1;
		}
	}

protected:
	std::atomic<long long>m_iStartTime;

	long long m_iAudioMicDuring = 0;
	long long m_iAudioMicThreshold = 0;
	long long m_iAudioMicPts = 0;

	long long m_iAudioDesktopDuring = 0;
	long long m_iAudioDesktopThreshold = 0;
	long long m_iAudioDesktopPts = 0;

	long long m_iVideoDuring = 0;
	long long m_iVideoThreshold = 0;
	long long m_iVideoPts = 0;
	PTS_STRATEGY m_type;
	std::atomic_bool m_bStop = true;

protected:
	typedef struct FREE_THREAD 
	{
		void operator()(std::thread* t) 
		{
			if (t) 
			{
				if (t->joinable()) 
				{
					t->join();
				}
				delete t;
			}
		}
	}FREE_THREAD;
	std::unique_ptr<std::thread, FREE_THREAD>m_thread;
};

using AVPublishTime = X_AVPublishTime<std::chrono::microseconds, std::chrono::microseconds::period::den>;


//#include "AVPublishTime.h"
//#include<chrono>

//
//AVPublishTime::AVPublishTime(PTS_STRATEGY type) :m_type(type)
//{
//	reset();
//}
//
//void AVPublishTime::reset()
//{
//	m_iStartTime = std::chrono::duration_cast<TIME_TYPE>(std::chrono::system_clock::now().time_since_epoch()).count()/1000.0;
//	m_iAudioPts = 0;
//	m_iVideoPts = 0;
//}
//
//long double AVPublishTime::getAudioPts()
//{
//	long double t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
//	if (m_type == PTS_STRATEGY_RECTIFY)
//	{
//		long double diff = t - m_iStartTime;
//		m_iAudioPts += m_iAudioDuring;
//		if (std::abs(diff - m_iAudioPts) > m_iAudioThreshold)
//			m_iAudioPts = t;
//	}
//	else 
//	{
//		m_iAudioPts = t;
//	}
//	return m_iAudioPts;
//}
//
//long double AVPublishTime::getVideoPts()
//{
//	long double t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
//	if (m_type == PTS_STRATEGY_RECTIFY)
//	{
//		long double diff = t - m_iStartTime;
//		m_iVideoPts += m_iVideoDuring;
//		if (std::abs(diff - m_iVideoPts) > m_iVideoThreshold)
//			m_iVideoPts = t;
//	}
//	else 
//	{
//		m_iVideoPts = t;
//	}
//	return m_iVideoPts;
//}
//
//void AVPublishTime::setAudioDuring(long double during) 
//{
//	m_iAudioDuring = during;
//	m_iAudioThreshold = during / 2;
//}
//
//void AVPublishTime::setVideoPtsDuring(long double during) 
//{
//	m_iVideoDuring = during;
//	m_iVideoThreshold = during / 2;
//}
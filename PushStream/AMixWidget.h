#pragma once
#include "QtScrollWidget.h"
#include<qslider.h>
#include <vector>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avutil.h"
#include "libavutil/samplefmt.h"
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif
#include "CSource.h"
#include "volume-control.hpp"

class QLabel;

class AMixWidget :public QtScrollWidget
{
	Q_OBJECT
public:
	AMixWidget(QWidget*parentWidget = nullptr);
	inline std::vector<PSSource>GetPSSource()
	{
		std::vector<PSSource>sets;
		sets.reserve(m_control.size());
		for (auto& ele : m_control)
			sets.push_back(ele->GetSource());
		return sets;
	}

	std::vector<float>getDBSets() { return m_dbSets; }

public:
	PSSource addAudioDevice(const std::string& name, const std::string& device_id, obs_peak_meter_type peak_calc_type, typename CWasapi<CSource>::SourceType type);
	void removeAudioDevice(PSSource source);


signals:
	void signalSendDB(int, float);


protected slots:
	void slotSignalSendDB(float db);
//signals:
//	void signalsDesktop(int);
//	void signalsMicrophone(int);
protected:
	void initUI();
	void initConnect();
	void initCaptureAudioDefaultDevice(QWidget* parentWidget);
	
protected:
	QSlider* m_sliderDesktop = nullptr;
	QSlider* m_sliderMicrophone = nullptr;
	std::vector<VolControl*>m_control;
	std::vector<float>m_dbSets;
};


#pragma once
#include "QtBaseTitleMoveWidget.h"
#include "CSource.h"


class QComboBox;
class QLineEdit;

class QtAudioSelDlg:public QtBaseTitleMoveWidget
{
public:
	QtAudioSelDlg(bool bIsInput, QWidget*parentWid = NULL);
	bool isInputDevice() {return m_bIsInputDevice;}
	CWasapi<CSource>::AudioDeviceInfo getSeledItemInfo();
	std::string name();
protected:
	void initUI();
protected:
	QLineEdit* m_EditName;
	QComboBox* m_sel;
	bool m_bIsInputDevice;
	std::vector<CWasapi<CSource>::AudioDeviceInfo>m_deviceSets;
};



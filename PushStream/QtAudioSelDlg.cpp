#include "QtAudioSelDlg.h"
#include<QLabel>
#include<QComboBox>
#include<QBoxLayout>
#include"Audio/CWasapi.h"
#include<QStringList>
#include<QString>
#include<QLineEdit>


QtAudioSelDlg::QtAudioSelDlg(bool bIsInput, QWidget* parentWid):QtBaseTitleMoveWidget(parentWid), m_bIsInputDevice(bIsInput)
{
	setTitleText(bIsInput ? QString::fromWCharArray(L"音频输入采集"): QString::fromWCharArray(L"音频输出采集"));
	resize(600,400);
	initUI();
}

CWasapi<CSource>::AudioDeviceInfo QtAudioSelDlg::getSeledItemInfo()
{
	if(m_deviceSets.empty())
		return CWasapi<CSource>::AudioDeviceInfo{ "","" };
	int index = m_sel->currentIndex();
	if(index == 0)
		return CWasapi<CSource>::AudioDeviceInfo{ "default","default" };
	return m_deviceSets[index -1];
}

std::string QtAudioSelDlg::name() 
{
	return m_EditName->text().toStdString();
}

void QtAudioSelDlg::initUI()
{
	QWidget*wid = getBodyWidget();
	QLabel*nameLabel = new QLabel(QString::fromWCharArray(L"音源名称"), wid);
	nameLabel->setScaledContents(true);
	m_EditName = new QLineEdit(wid);
	m_EditName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_EditName->setText(m_bIsInputDevice ? QString::fromWCharArray(L"音频输入采集") : QString::fromWCharArray(L"音频输出采集"));
	QHBoxLayout* nameLay = new QHBoxLayout;
	nameLay->setAlignment(Qt::AlignCenter);
	nameLay->addWidget(nameLabel);
	nameLay->addWidget(m_EditName);


	QLabel* label = new QLabel(QString::fromWCharArray(L"设备"), wid);
	label->setScaledContents(true);
	m_sel = new QComboBox(wid);
	m_deviceSets = m_bIsInputDevice ? CWasapi<CSource>::getMicrophoneDevices() : CWasapi<CSource>::getDesktopAudioDevices();


	QStringList strlist;
	if(!m_deviceSets.empty())
		strlist << QString::fromWCharArray(L"默认");
	for (auto& ele : m_deviceSets)
		strlist << QString::fromStdString(ele.name);
	m_sel->addItems(strlist);
	if (!m_deviceSets.empty())
		m_sel->setCurrentIndex(0);

	m_sel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	QHBoxLayout* selLay = new QHBoxLayout;
	selLay->setAlignment(Qt::AlignCenter);
	selLay->addWidget(label);
	selLay->addWidget(m_sel);

	QVBoxLayout * mainL = new QVBoxLayout;
	mainL->setAlignment(Qt::AlignTop);
	mainL->addLayout(nameLay);
	mainL->addLayout(selLay);
	wid->setLayout(mainL);
	m_EditName->setFocus();
}

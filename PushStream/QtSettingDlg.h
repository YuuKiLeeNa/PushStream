#pragma once
#include "QtBaseTitleMoveWidget.h"
#include<QString>
#include<tuple>
#include<QStackedWidget>
#include<qscrollarea.h>
#include<string>
#include"CMediaOption.h"
#include<QBoxLayout>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QTabWidget;
class QSpinBox;
struct AVCodec;

class QtSettingDlg:public QtBaseTitleMoveWidget
{
	Q_OBJECT
public:
	QtSettingDlg(const QString&url, const CMediaOption&opt,QWidget*parentWid = NULL);
	QString getPushUrl();
	CMediaOption getRecordOpt() { return m_recordOpt; }

protected:
	void initUI(const QString& url);
	void initData();
protected:
	void SlotBtnPushStreamClicked(bool);
	void SlotBtnRecordClicked(bool);

protected:
	template<typename Control, typename ParentWid>
	QHBoxLayout* create_label_control(const QString& name, Control*& control, ParentWid* wid)const 
	{
		QLabel* label = new QLabel(name, wid);
		label->setScaledContents(true);
		control = new Control(wid);
		control->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		QHBoxLayout* lay = new QHBoxLayout;
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		lay->addWidget(label);
		lay->addWidget(control);
		return lay;
	}


protected:
	QPushButton* m_btnPushStream = NULL;
	QPushButton* m_btnRecord = NULL;

	std::vector<std::tuple<QString, QPushButton* (QtSettingDlg::*), void (QPushButton::*)(bool),void (QtSettingDlg::*)(bool)>>m_vInitInfo =
	{
		{QString::fromWCharArray(L"推流"), &QtSettingDlg::m_btnPushStream, &QPushButton::clicked, &QtSettingDlg::SlotBtnPushStreamClicked}
		,{QString::fromWCharArray(L"录制"), &QtSettingDlg::m_btnPushStream, &QPushButton::clicked, &QtSettingDlg::SlotBtnRecordClicked}
	};

#define getName_vInitInfo(index)							std::get<0>(m_vInitInfo[index])
#define getBtn_vInitInfo(index)							this->*std::get<1>(m_vInitInfo[index])
#define getSignalFunMemberPointer_vInitInfo(index)		get<2>(m_vInitInfo[index])
#define getSlotFunMemberPointer_vInitInfo(index)			get<3>(m_vInitInfo[index])

	QScrollArea* m_scrollArea = NULL;
	QStackedWidget* m_stack = NULL;

protected:
	void initWidPushStream(const QString &url);
	QWidget* m_widPushStream = NULL;
	QLineEdit* m_editAccount = NULL;
	QLineEdit* m_editPassword = NULL;

protected:
	void initWidRecord();
	QWidget* initVideoPage(QWidget*parent);
	QWidget* initAudioPage(QWidget*parent);

	void slotBtnSelPathClicked();
	void slotComboxVideoCodecIdChanged(int index);
	void slotComboxAudioCodecIdChanged(int index);
	void comboxCodecIdChanged(int index, QComboBox*comboxCodecId, QComboBox*comboxCodec, std::vector<const AVCodec*>&saveCodecs);
	void initPresetProfileTune(int codecSelIndex);
	//void initProfile(int codecSelIndex);
	//void initTune(int codecSelIndex);



	QWidget* m_widRecord = NULL;
	CMediaOption m_recordOpt;
	QLineEdit* m_editRecordPath = NULL;
	QPushButton* m_btnSelPath = NULL;

	QTabWidget* m_tabRecord = NULL;
	QComboBox* m_comboxRecordVideo_codecid = NULL;
	QCheckBox* m_checkRecordBoxEnableHardwareAccelator = NULL;
	QComboBox* m_comBoxRecordPrioritySelVideoCodec = NULL;

	QStringList m_strListVideoEncoderMode = {"vbr", "abr", "cbr", "CQP", "crf"};
	QComboBox* m_comboxRecordVideoMode = NULL;//vbr abr cbr CQP crf

	QSpinBox* m_spinVideoMinBitrate = NULL;
	QSpinBox* m_spinVideoMaxBitrate = NULL;
	QSpinBox* m_spinVideoBuffSize = NULL;
	QSpinBox* m_spinCQP = NULL;
	QSpinBox* m_spinCrf = NULL;
	QComboBox* m_comboxProfile = NULL;
	QComboBox* m_comboxTune = NULL;
	QComboBox* m_comboxPreset = NULL;

	QHBoxLayout* m_layMin = NULL;
	QHBoxLayout* m_layMax = NULL;
	QHBoxLayout* m_layBuffer = NULL;
	QHBoxLayout* m_layCQP = NULL;
	QHBoxLayout* m_layCRF = NULL;
	QHBoxLayout* m_layProfile = NULL;
	QHBoxLayout* m_layTune = NULL;
	QHBoxLayout* m_layPreset = NULL;


	QComboBox* m_comboxRecordAudio_codecid = NULL;
	QComboBox* m_comBoxRecordPrioritySelAudioCodec = NULL;
	QSpinBox* m_spinAudioBitrate = NULL;


	std::vector<const AVCodec*>m_vecVideoCodec;
	std::vector<const AVCodec*>m_vecAudioCodec;

};


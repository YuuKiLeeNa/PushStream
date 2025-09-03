#include "QtAudioFilterDlg.h"
#include "Audio/CSource.h"
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include<QStringList>
#include<QLabel>
#include<QBoxLayout>

QtAudioFilterDlg::QtAudioFilterDlg(std::shared_ptr<CSource> src, QWidget* parentWid):QtBaseTitleMoveWidget(parentWid), m_ptr(src)
{
	setTitleText(QString::fromWCharArray(L"音频滤镜"));
	resize(600,400);
	initUI();
	initState();
	initconnect();
}

void QtAudioFilterDlg::initUI()
{
	QWidget*wid = getBodyWidget();
	m_on_CheckBox = new QCheckBox(QString::fromWCharArray(L"开启音频滤镜"), wid);
	
	/*QComboBox* m_denoise_Combo;
	QSlider* m_speexAEC_slider;
	QSlider* m_speexAES_slider;
	QCheckBox* m_onVAD_CheckBox;
	QSlider* m_speexAGC_slider;*/
	m_denoise_Combo = new QComboBox(wid);
	QStringList strlist;
	strlist << QString::fromWCharArray(L"rnnoise降噪") << QString::fromWCharArray(L"speex降噪");
	m_denoise_Combo->addItems(strlist);
	m_denoise_Combo->setCurrentIndex(0);

	QLabel* AEC_label = new QLabel(QString::fromWCharArray(L"speex降噪(dB):"), wid);
	AEC_label->setScaledContents(true);
	m_speexAEC_slider = new QSlider(Qt::Orientation::Horizontal,wid);
	m_speexAEC_slider->setRange(-30, 0);
	QHBoxLayout* aec_lay = new QHBoxLayout;
	aec_lay->setAlignment(Qt::AlignCenter);
	aec_lay->addWidget(AEC_label);
	aec_lay->addWidget(m_speexAEC_slider);

	QLabel*AES_label = new QLabel(QString::fromWCharArray(L"speex去回声(dB):"), wid);
	AES_label->setScaledContents(true);
	m_speexAES_slider = new QSlider(Qt::Orientation::Horizontal, wid);
	m_speexAES_slider->setRange(0, 30);
	QHBoxLayout* aes_lay = new QHBoxLayout;
	aes_lay->setAlignment(Qt::AlignCenter);
	aes_lay->addWidget(AES_label);
	aes_lay->addWidget(m_speexAES_slider);

	m_onVAD_CheckBox = new QCheckBox(QString::fromWCharArray(L"开启人声检测"), wid);


	m_onAGC_CheckBox = new QCheckBox(QString::fromWCharArray(L"开启人声检测自动增益"), wid);


	QLabel*vad_label = new QLabel(QString::fromWCharArray(L"人声检测自动增益等级"), wid);
	m_speexAGC_slider = new QSlider(Qt::Orientation::Horizontal, wid);
	m_speexAGC_slider->setRange(5000, 150000);
	QHBoxLayout* vad_slide_lay = new QHBoxLayout;
	vad_slide_lay->setAlignment(Qt::AlignCenter);
	vad_slide_lay->addWidget(vad_label);
	vad_slide_lay->addWidget(m_speexAGC_slider);

	QVBoxLayout* mainL = new QVBoxLayout;
	mainL->setAlignment(Qt::AlignLeft);
	mainL->addWidget(m_on_CheckBox);
	mainL->addWidget(m_denoise_Combo);
	mainL->addLayout(aec_lay);
	mainL->addLayout(aes_lay);
	mainL->addWidget(m_onVAD_CheckBox);
	mainL->addWidget(m_onAGC_CheckBox);
	mainL->addLayout(vad_slide_lay);


	wid->setLayout(mainL);

}

void QtAudioFilterDlg::initconnect() 
{
	QObject::connect(m_on_CheckBox, SIGNAL(checkStateChanged(Qt::CheckState)), this, SLOT(slot_m_on_CheckBox_clicked(Qt::CheckState)));
	QObject::connect(m_denoise_Combo, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_m_denoise_Combo_selected_changed(int)));
	QObject::connect(m_speexAEC_slider, SIGNAL(valueChanged(int)), this, SLOT(slot_m_speexAEC_slider_value_changed(int)));
	QObject::connect(m_speexAES_slider, SIGNAL(valueChanged(int)), this, SLOT(slot_m_speexAES_slider_value_changed(int)));
	QObject::connect(m_onVAD_CheckBox, SIGNAL(checkStateChanged(Qt::CheckState)), this, SLOT(slot_m_onVAD_CheckBox(Qt::CheckState)));
	QObject::connect(m_onAGC_CheckBox, SIGNAL(checkStateChanged(Qt::CheckState)), this, SLOT(slot_m_onAGC_CheckBox(Qt::CheckState)));
	QObject::connect(m_speexAGC_slider, SIGNAL(valueChanged(int)), this, SLOT(slot_m_speexAGC_slider_value_changed(int)));
}

void QtAudioFilterDlg::initState()
{
	m_on_CheckBox->setChecked(m_ptr->getAudioFilterStatus());
	slot_m_denoise_Combo_selected_changed(m_denoise_Combo->currentIndex());
	bool b;

	if(m_ptr->getOption(rnnoiseHelp::SPEEX_VAD_VOICE_TYPE, b, NULL))
		m_onVAD_CheckBox->setChecked(b);

	rnnoiseHelp::VARIANT ant;
	if (m_ptr->getOption(rnnoiseHelp::SPEEX_AGC_VOICE_TYPE, b, &ant)) 
	{
		m_onAGC_CheckBox->setChecked(b);
		m_speexAGC_slider->setEnabled(b);
		if (b)
		{
			float v = std::get<0>(ant);
			m_speexAGC_slider->setValue(v);
		}
	}

}

void QtAudioFilterDlg::slot_m_on_CheckBox_clicked(Qt::CheckState state) 
{
	switch (state) 
	{
	case Qt::CheckState::Checked:
		m_ptr->setAudioFilterStatus(true);
		break;
	case Qt::CheckState::Unchecked:
		m_ptr->setAudioFilterStatus(false);
		break;
	}
}

void QtAudioFilterDlg::slot_m_denoise_Combo_selected_changed(int index) 
{
	switch (index) 
	{
	case 0:
	{
		rnnoiseHelp::VARIANT ant;
		m_ptr->setOption(rnnoiseHelp::RNNOISE_VOICE_TYPE, true, ant);
		ant = (int)0;
		m_ptr->setOption(rnnoiseHelp::SPEEX_AEC_VOICE_TYPE, false, ant);
		ant = 0.0f;
		m_ptr->setOption(rnnoiseHelp::SPEEX_AES_VOICE_TYPE, false, ant);
		m_speexAEC_slider->setEnabled(false);
		m_speexAES_slider->setEnabled(false);
	}
		break;
	case 1:
	{
		rnnoiseHelp::VARIANT ant;
		m_ptr->setOption(rnnoiseHelp::RNNOISE_VOICE_TYPE, false, ant);
		ant = (int)m_speexAEC_slider->value();
		m_ptr->setOption(rnnoiseHelp::SPEEX_AEC_VOICE_TYPE, true, ant);
		ant = (float)m_speexAES_slider->value();
		m_ptr->setOption(rnnoiseHelp::SPEEX_AES_VOICE_TYPE, true, ant);
		m_speexAEC_slider->setEnabled(true);
		m_speexAES_slider->setEnabled(true);
	}
		break;
	}
}
void QtAudioFilterDlg::slot_m_speexAEC_slider_value_changed(int value) 
{
	rnnoiseHelp::VARIANT ant = (int)m_speexAEC_slider->value();
	m_ptr->setOption(rnnoiseHelp::SPEEX_AEC_VOICE_TYPE, true, ant);
}
void QtAudioFilterDlg::slot_m_speexAES_slider_value_changed(int value) 
{
	rnnoiseHelp::VARIANT ant = (float)m_speexAES_slider->value();
	m_ptr->setOption(rnnoiseHelp::SPEEX_AES_VOICE_TYPE, true, ant);
}
void QtAudioFilterDlg::slot_m_onVAD_CheckBox(Qt::CheckState state) 
{
	rnnoiseHelp::VARIANT ant;
	m_ptr->setOption(rnnoiseHelp::SPEEX_VAD_VOICE_TYPE, state == Qt::CheckState::Checked ? true:false,ant);
}

void QtAudioFilterDlg::slot_m_onAGC_CheckBox(Qt::CheckState state) 
{
	int v = m_speexAGC_slider->value();
	rnnoiseHelp::VARIANT ant = (float)v;
	bool b = state == Qt::CheckState::Checked ? true : false;
	m_ptr->setOption(rnnoiseHelp::SPEEX_AGC_VOICE_TYPE, b, ant);
	m_speexAGC_slider->setEnabled(b);
}

void QtAudioFilterDlg::slot_m_speexAGC_slider_value_changed(int value) 
{
	rnnoiseHelp::VARIANT ant = (float)m_speexAGC_slider->value();
	m_ptr->setOption(rnnoiseHelp::SPEEX_AGC_VOICE_TYPE, true, ant);
}
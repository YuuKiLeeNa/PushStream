#pragma once
#include<memory>
#include "QtBaseTitleMoveWidget.h"

//#include<QSlider>


class CSource;
class QCheckBox;
class QComboBox;
class QSlider;

class QtAudioFilterDlg :public QtBaseTitleMoveWidget
{
	Q_OBJECT
public:
	QtAudioFilterDlg(std::shared_ptr<CSource>src,QWidget*parentWid = NULL);

protected:
	void initUI();
	void initconnect();
	void initState();
protected slots:
	void slot_m_on_CheckBox_clicked(Qt::CheckState state);
	void slot_m_denoise_Combo_selected_changed(int index);
	void slot_m_speexAEC_slider_value_changed(int value);
	void slot_m_speexAES_slider_value_changed(int value);
	void slot_m_onVAD_CheckBox(Qt::CheckState state);
	void slot_m_onAGC_CheckBox(Qt::CheckState state);
	void slot_m_speexAGC_slider_value_changed(int value);
protected:
	std::shared_ptr<CSource> m_ptr;
	QCheckBox* m_on_CheckBox;
	QComboBox* m_denoise_Combo;
	QSlider* m_speexAEC_slider;
	QSlider* m_speexAES_slider;
	QCheckBox* m_onVAD_CheckBox;
	QCheckBox* m_onAGC_CheckBox;
	QSlider* m_speexAGC_slider;
};
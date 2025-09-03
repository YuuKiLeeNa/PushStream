#pragma once

#include"QtScrollWidget.h"
#include<QPushButton>
#include<QButtonGroup>
#include<tuple>

class ButtonSets:public QWidget
{
	Q_OBJECT
public:
	ButtonSets(QWidget* parentWidget = nullptr);
	QPushButton* getBtn(int id);
	void updateBtnText();

signals:
	void signalsBtnClicked(int);
	void signalsPushBtnClicked(bool);
	void signalsRecordBtnClicked(bool);
	void signalsBtnSettingClicked(bool);

protected:
	void initUI();
	void initConnect();
protected:
	QPushButton* m_btnPush = nullptr;
	QPushButton* m_btnRecord = nullptr;
	QPushButton* m_btnSetting = nullptr;
	//QPushButton* m_btnCamera = nullptr;
	//name,btn, group id, is checked, is add to btn group 

	std::vector<std::tuple<QString, QString, QPushButton* ButtonSets::*>>btnTextState =
	{
		{tr("开始推流"),tr("停止推流"), &ButtonSets::m_btnPush}
		,{tr("开始录制"),tr("停止录制"), &ButtonSets::m_btnRecord}
	};

	std::vector<std::tuple<QString, QPushButton* ButtonSets::*, int,bool, bool>>initbtn =
	{
		{tr("开始推流"),&ButtonSets::m_btnPush, 0, true, false}
		,{tr("开始录制"),& ButtonSets::m_btnRecord, 1,true, false}
		//,{tr("添加相机"),&ButtonSets::m_btnCamera, 2,false, true}
		,{tr("设置"),&ButtonSets::m_btnSetting, 3,false, true}
	};
	QButtonGroup* m_group = nullptr;
};


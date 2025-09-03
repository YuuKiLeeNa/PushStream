#pragma once
#include "QtBaseTitleMoveWidget.h"
#include "CalcGLDraw.h"
#include "openImg/openImg.h"
#include<QLabel>
#include<QLineEdit>
#include<QPushButton>
#include<QSlider>
#include "PushStream/ResizeGLWidget.h"


class QtOpenImgDlg :public QtBaseTitleMoveWidget 
{
	Q_OBJECT
public:
	QtOpenImgDlg(QWidget* parentWid = NULL);
	QtOpenImgDlg(std::shared_ptr<openImg>openImg, QWidget*parentWid = NULL);
	~QtOpenImgDlg();
	std::shared_ptr<openImg> getOpenImg() { return m_openImg; }

public slots:
	void slotBtnClicked(bool);
	void slotValueChanged(int v);
protected:
	void initUI();
	void initConnect();

	std::shared_ptr<openImg>m_openImg;
	QtGLVideoWidget* m_glwidget = NULL;
	QLineEdit* m_edit = NULL;
	QPushButton* m_btn = NULL;
	QSlider* m_slider = NULL;

};
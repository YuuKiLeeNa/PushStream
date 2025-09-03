#pragma once
#include "QtBaseTitleMoveWidget.h"
#include <memory>
#include <vector>
#include <string>
#include <QComboBox>
#include "define_type.h"
class GLDrawCamera;
class QtGLVideoWidget;
class PicToYuv;
struct AVFrame;
class ResizeGLWidget;
class QSlider;


class SelCamWidget:public QtBaseTitleMoveWidget,public std::enable_shared_from_this<SelCamWidget>
{
	Q_OBJECT
public:
	SelCamWidget(QWidget* pParentWidget = nullptr);
	SelCamWidget(std::shared_ptr<GLDrawCamera>pCamera,QWidget* pParentWidget = nullptr);
	~SelCamWidget();
	std::shared_ptr<GLDrawCamera>getCamera();
	std::wstring getCameraName();
protected:
	void initUI();
public slots:
	void selBoxChanged(int index);
	void SlotSliderValueChanged(int value);
protected:
	//void GetFrame(long, uint8_t* buff, long len);
	void GetFrame(UPTR_FME f);
	void initConnect();
protected:
	//std::unique_ptr<Camera,std::function<void(Camera*)>>m_camera;
	std::shared_ptr<GLDrawCamera>m_camera;
	QComboBox* m_selBox;
	QtGLVideoWidget* m_glWidget;
	QSlider* m_scaleSlider;

	//ResizeGLWidget* m_glWidget;
	std::vector<std::wstring>m_strCamSets;
	/*int m_lWidth = 0;
	int m_lHeight = 0;
	int m_iBitCount = 0;
	std::shared_ptr<PicToYuv>m_con;*/
	AVFrame* f_;
};


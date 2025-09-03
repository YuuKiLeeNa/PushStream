#pragma once
#include "QtBaseTitleMoveWidget.h"
#include <memory>
#include <vector>
#include <string>
#include <QComboBox>
class Camera;
//class QComboBox;
//class GLVideoWidget;
class QtGLVideoWidget;
class PicToYuv;
struct AVFrame;
class SelCamWidget:public QtBaseTitleMoveWidget
{
	Q_OBJECT
public:
	SelCamWidget(QWidget*pParentWidget = nullptr);
	~SelCamWidget();
	std::unique_ptr<Camera, std::function<void(Camera*)>>getCamera();
	std::wstring getCameraName();
protected:
	void initUI();
protected slots:
	void selBoxChanged(int index);
protected:
	void GetFrame(long, uint8_t* buff, long len);
	void initConnect();
protected:
	std::unique_ptr<Camera,std::function<void(Camera*)>>m_camera;
	QComboBox* m_selBox;
	//GLVideoWidget* m_glWidget;
	QtGLVideoWidget* m_glWidget;
	std::vector<std::wstring>m_strCamSets;
	int m_lWidth = 0;
	int m_lHeight = 0;
	int m_iBitCount = 0;
	std::shared_ptr<PicToYuv>m_con;
	AVFrame* f_;
};


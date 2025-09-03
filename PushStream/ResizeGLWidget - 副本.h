#pragma once
//#include "GLVideoWidget.h"
#include "QtGLVideoWidget.h"
#include<functional>
class Camera;
class PicToYuv;

class ResizeGLWidget :public QtGLVideoWidget//GLVideoWidget
{
public:
	ResizeGLWidget(QWidget*pParentWidget);
    virtual ~ResizeGLWidget();
    bool openCamera(const std::wstring& strCamera);
    bool openCamera(std::unique_ptr<Camera, std::function<void(Camera*)>>&&camera, const std::wstring& strCamera);
public:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    std::pair<int,int>setFrameSizeAccordCamera();
private:
    void checkEdge();

private:
    //Ui::DragResizeWgt* ui;

    QPoint m_startCursor;

    int m_nLeftOff = 0;//鼠标开始拖拽时子窗口左边相对父窗口左边的距离
    int m_nRightOff = 0;//鼠标开始拖拽时子窗口右边相对父窗口左边的距离
    int m_nTopOff = 0;//鼠标开始拖拽时子窗口上边相对父窗口上边的距离
    int m_nBottomOff = 0;//鼠标开始拖拽时子窗口下边相对父窗口上边的距离

    QPoint dragPosition;   //鼠标拖动的位置
    int    edgeMargin = 4;     //鼠标检测的边缘距离
    enum {
        nodir,
        top = 0x01,
        bottom = 0x02,
        left = 0x04,
        right = 0x08,
        topLeft = 0x01 | 0x04,
        topRight = 0x01 | 0x08,
        bottomLeft = 0x02 | 0x04,
        bottomRight = 0x02 | 0x08
    } resizeDir; //更改尺寸的方向


protected:
    void GetFrame(long, uint8_t* buff, long len);

    int m_lWidth = 0;
    int m_lHeight = 0;
    int m_iBitCount = 0;
    std::shared_ptr<PicToYuv>m_con;
    std::unique_ptr<Camera, std::function<void(Camera*)>>m_camera;
};


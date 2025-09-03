#pragma once
#include "QtGLVideoWidget.h"
#include <functional>
#include "define_type.h"

class ResizeGLWidget :public QtGLVideoWidget
{
public:
	ResizeGLWidget(QWidget*pParentWidget = nullptr);
    virtual ~ResizeGLWidget();
    int64_t calcWidthOrHeighBasesOneOfTwo(int64_t v, int FrameWidth, int FrameHeight, bool isWidth = true);
    void calcWidthAndHeight(int w, int h, int*nw, int*nh, int FrameWidth, int FrameHeight, bool iszoomin = true);
    double calcFrameWHRatio();
    //static ResizeGLWidget* g_wid;
public:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    //virtual void setFrameSize(int w, int h)override;
    void resizeGL(int width, int height) override;
    void renderFrame(UPTR_FME f);
    void setFrame(AVFrame* f);
  /*  void resizeEvent(QResizeEvent* event) {
        QOpenGLWidget::resizeEvent(event);
        glViewport(0, 0, width(), height());
    }*/

protected:
    void focusInEvent(QFocusEvent* event)override;
    void focusOutEvent(QFocusEvent* event)override;
private:
    void checkEdge(bool changedState = true);

private:
    QPointF m_startCursor;

    int m_nLeftOff = 0;
    int m_nRightOff = 0;
    int m_nTopOff = 0;
    int m_nBottomOff = 0;

    QPointF dragPosition;
    int    edgeMargin = 4;
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
    } resizeDir;

};


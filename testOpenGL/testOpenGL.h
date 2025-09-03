#pragma once

#include <QtWidgets/QMainWindow>







class QtDrawFrameBuffer;
class QtResizeDrawFrameBuffer;
class QtGLVideoWidget;
class ResizeGLWidget;
struct AVFrame;

//typedef QtResizeDrawFrameBuffer GLWIDGET;
typedef QtGLVideoWidget GLWIDGET;
class QResizeEvent;

class testOpenGL : public QMainWindow
{
    Q_OBJECT

public:
    testOpenGL(QWidget *parent = nullptr);
    ~testOpenGL();
protected:
    void initUI(QWidget*parentWid = NULL);
    void resizeEvent(QResizeEvent* event)override;
private:
    GLWIDGET* m_wid;
    //AVFrame* m_frame = NULL;
};

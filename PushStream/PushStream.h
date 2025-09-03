#pragma once

#include <QtWidgets/QMainWindow>
#include<memory>
#include<functional>
#include <vector>
#include<unordered_map>
#include<mutex>
#include "AACEncoder.h"
#include "ScreenLive.h"
#include<QEvent>
#include "AMixWidget.h"
#include "QtAddSourceWidget.h"
#include "define_type.h"



class QtGLVideoWidget;
class AMixWidget;
class ButtonSets;
class Camera;
class PicToYuv;
class RenderCameraWidget;
class ResizeGLWidget;
class QtResizeDrawFrameBuffer;


typedef QtResizeDrawFrameBuffer QT_RENDER_WID;
Q_DECLARE_METATYPE(CaptureEvents::CaptureEventsType)

class PushStream : public QMainWindow, public ScreenLive<PushStream>
{
    Q_OBJECT
public:
    PushStream(QWidget *parent = Q_NULLPTR);

    inline std::vector<PSSource>GetPSSource() { return m_amixWidget->GetPSSource(); }

    std::vector<float>getDBSets() { return m_amixWidget->getDBSets(); }

    inline UPTR_FME getFrameYUV420P();

signals:
    void signalsFrameUpdate(AVFrame*);
    void signalsCaptureRestart(CaptureEvents::CaptureEventsType type, int index);
    void signalsCaptureClose(CaptureEvents::CaptureEventsType type, int index);

protected:
    void FrameUpdate(AVFrame* f);
protected:
    void initUI();
    void initConnect();
    //void pushCamera(const QString&name, RenderCameraWidget*f);
    //void audioEncoderThread
    //void focusInEvent(QFocusEvent* event)override;
    void Restart(CaptureEvents::CaptureEventsType type, int index) override;
    void Close(CaptureEvents::CaptureEventsType type, int index) override;


protected slots:
    void slotBtnClicked(int id);
    void slotPushBtnClicked(bool b);
    void slotRecordBtnClicked(bool b);
    void slotBtnSettingClicked(bool b);
    void slotSendDB(int index, float dB);
    void slotCaptureRestart(CaptureEvents::CaptureEventsType type, int index);
    void slotCaptureClose(CaptureEvents::CaptureEventsType type, int index);

protected:
    //bool eventFilter(QObject* watched, QEvent* event)override;
    //ResizeGLWidget* curSeled_ = nullptr;
private:
    //std::vector<ResizeGLWidget*>focusSets_;
    //QtGLVideoWidget* m_glWidget = nullptr;
    QT_RENDER_WID* m_glWidget = nullptr;
    QtAddSourceWidget* m_addWidget = nullptr; 
    AMixWidget* m_amixWidget = nullptr;
    ButtonSets* m_btnWidget = nullptr;
    int m_wid;
    int m_hei;
    std::vector<QT_RENDER_WID*>sets_;
    //std::unordered_map<int, RenderCameraWidget*>sets_;
    std::mutex mutexOfsets_;
};

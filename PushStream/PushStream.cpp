#include "PushStream.h"
#include "RenderCameraWidget.h"
#include "AMixWidget.h"
#include "ButtonSets.h"
#include "VideoCapture/DXGIScreenCapture.h"
//#include "AudioCapture/AudioCapture.h"
#include "SelCamWidget.h"
#include "VideoCapture/GLDrawCamera.h"
#include "PicToYuv.h"
#include "ResizeGLWidget.h"
#include<QBoxLayout>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif
#include<QMouseEvent>
#include<QMessageBox>
#include "QtResizeDrawFrameBuffer.h"
#include "QtSettingDlg.h"


PushStream::PushStream(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1200,800);
    initUI();
    initConnect();
    ScreenCapture_->setCaptureFrameCallBack([this](UPTR_FME f)
        {
            emit signalsFrameUpdate(f.release());
        });

    //ScreenCapture_->Init();
    //ScreenCapture_->CaptureStarted();
}


UPTR_FME PushStream::getFrameYUV420P() 
{
    return m_glWidget->getFrameYUV420P();
}


void PushStream::FrameUpdate(AVFrame* f)
{
    m_glWidget->renderFrame(UPTR_FME(f));
}

void PushStream::initUI() 
{
    QWidget* centerWid = new QWidget(this);
    //centerWid->setObjectName("centerWid");
    //centerWid->setStyleSheet("QWidget#centerWid{background-color:#4c4c4c;}");
    QVBoxLayout* centerWidLay = new QVBoxLayout;
    centerWidLay->setContentsMargins(0,0,0,0);
    centerWidLay->setSpacing(0);

    QWidget* glWidgetParent = new QWidget(centerWid);
    glWidgetParent->setObjectName("glWidgetParent");
    glWidgetParent->setStyleSheet("QWidget#glWidgetParent{background-color:#4c4c4c;}");
    
    m_glWidget = new std::remove_pointer<decltype(m_glWidget)>::type(glWidgetParent);
    m_glWidget->setObjectName("desktop");

    //m_glWidget->setFixedSize(400,400);
    sets_.push_back(m_glWidget);
    //m_glWidget->installEventFilter(this);
    //focusSets_.push_back(m_glWidget);
    //m_glWidget->setObjectName("m_glWidget");
    //m_glWidget->setStyleSheet("ResizeGLWidget#m_glWidget{border:1px solid red;}");
    QHBoxLayout* glWidgetParentLay = new QHBoxLayout;
    glWidgetParentLay->setContentsMargins(0,0,0,0);
    glWidgetParentLay->setSpacing(0);
    glWidgetParentLay->addWidget(m_glWidget);
    glWidgetParent->setLayout(glWidgetParentLay);

    //m_glWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //m_glWidget->setYUV420pParameters(1920, 1080);
    //m_glWidgetLay = new QHBoxLayout;
    //m_glWidgetLay->setSpacing(0);
    //m_glWidgetLay->setContentsMargins(0,0,0,0);
    //m_glWidgetLay->layoutstretch();
    //m_glWidgetLay->addWidget(m_glWidget);
    m_addWidget = new QtAddSourceWidget(m_glWidget, centerWid);
    m_addWidget->setMaximumHeight(160);
    m_addWidget->setMaximumWidth(140);
    m_addWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_amixWidget = new AMixWidget(centerWid);
    m_amixWidget->setMaximumHeight(160);

    m_addWidget->setAMixWidget(m_amixWidget);


    m_btnWidget = new ButtonSets(centerWid);
    m_btnWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    m_btnWidget->setMaximumHeight(160);
    m_btnWidget->setMaximumWidth(140);



    //m_amixWidget->setFixedSize(200,120);

    //m_btnWidget->setFixedSize(200,120);
    //m_btnWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setContentsMargins(0, 0, 0, 0);
    hlay->addWidget(m_addWidget);
    hlay->addWidget(m_amixWidget);
    hlay->addWidget(m_btnWidget);

    centerWidLay->addWidget(glWidgetParent);
    centerWidLay->addLayout(hlay);
    centerWid->setLayout(centerWidLay);
    setCentralWidget(centerWid);



}

void PushStream::initConnect()
{
    //qRegisterMetaType<std::shared_ptr<uint8_t>>("std::shared_ptr<uint8_t>");
    QObject::connect(this, &PushStream::signalsFrameUpdate,this, &PushStream::FrameUpdate);
    QObject::connect(m_btnWidget, &ButtonSets::signalsBtnClicked, this, &PushStream::slotBtnClicked);
    QObject::connect(m_btnWidget, &ButtonSets::signalsPushBtnClicked, this, &PushStream::slotPushBtnClicked);
    QObject::connect(m_btnWidget, &ButtonSets::signalsRecordBtnClicked, this, &PushStream::slotRecordBtnClicked);
    QObject::connect(m_btnWidget, &ButtonSets::signalsBtnSettingClicked, this, &PushStream::slotBtnSettingClicked);
    QObject::connect(m_amixWidget, &AMixWidget::signalSendDB, this, &PushStream::slotSendDB);

    QObject::connect(this, &PushStream::signalsCaptureRestart, this, &PushStream::slotCaptureRestart);
    QObject::connect(this, &PushStream::signalsCaptureClose, this, &PushStream::slotCaptureClose);
}

void PushStream::Restart(CaptureEvents::CaptureEventsType type, int index) 
{
    emit signalsCaptureRestart(type, index);
}

void PushStream::Close(CaptureEvents::CaptureEventsType type, int index)
{
    emit signalsCaptureClose(type, index);
}


//void PushStream::focusInEvent(QFocusEvent* event) 
//{
//    m_glWidget->clearFocus();
//}
//void PushStream::pushCamera() 
//{
//    
//}

void PushStream::slotBtnClicked(int id) 
{
    switch (id) 
    {
        case 2:
        {
            SelCamWidget dlg(nullptr);
            if (dlg.exec() == QDialog::Accepted) 
            {
                RenderCameraWidget* p = new RenderCameraWidget(m_glWidget);
                static int order = 0;
                ++order;
                p->setObjectName("camera"+QString("%1").arg(order));
                //p->setObjectName("camera");
                //p->installEventFilter(this);
                //focusSets_.push_back(p);
                if (!p->openCamera(std::static_pointer_cast<Camera>(dlg.getCamera()), dlg.getCameraName())) 
                {
                    QMessageBox::warning(nullptr, "error", QString::fromStdWString(L"open camera:" + dlg.getCameraName() + L"error"), QMessageBox::Ok, QMessageBox::Ok);
                    delete p;
                    return;
                }
                //sets_.push_back(p);
                //addRemCamera(p);
                auto si = p->setFrameSizeAccordCamera();
                p->resize(si.first, si.second);
                p->move(m_glWidget->width() - p->width(), m_glWidget->height() - p->height());
                p->show();
            }
        }
        break;
        case 3:
            break;
        default:
            break;
    }
}

void PushStream::slotPushBtnClicked(bool b) 
{
    if (b)
        startPushStream();
        //start();
    else
        stopPushStream();
    m_btnWidget->updateBtnText();
}

void PushStream::slotRecordBtnClicked(bool b) 
{
    if (b)
        startRecord();
    else
        stopRecord();
    m_btnWidget->updateBtnText();
}

void PushStream::slotBtnSettingClicked(bool b) 
{
    QtSettingDlg dlg(QString::fromStdString(url_), m_encodeHelp.getOption());
    if (dlg.exec() == QDialog::Accepted) 
    {
       url_ = dlg.getPushUrl().toStdString();
       m_encodeHelp.setOption(dlg.getRecordOpt());
    }
}

void PushStream::slotSendDB(int index, float dB)
{
    amixer_.setVolumeDB(index, dB);
}

void PushStream::slotCaptureRestart(CaptureEvents::CaptureEventsType type, int index) 
{
    ScreenLive<PushStream>::Restart(type, index);
}

void PushStream::slotCaptureClose(CaptureEvents::CaptureEventsType type, int index) 
{
    ScreenLive<PushStream>::Close(type, index);
}



//bool PushStream::eventFilter(QObject* watched, QEvent* event)
//{
//
//    if (QEvent::MouseButtonPress == event->type())
//    {
//        auto iter = std::find(focusSets_.begin(), focusSets_.end(), watched);
//        if (iter != focusSets_.end())
//        {
//            for (auto& ele : focusSets_)
//            {
//                if (*iter == ele)
//                    ele->setFocus();
//                else
//                    ele->clearFocus();
//            }
//            ResizeGLWidget::g_wid = *iter;
//            qDebug() << "MouseButtonPress:" << (*iter)->objectName();
//            QMouseEvent* qMouse = static_cast<QMouseEvent*>(event);
//            qMouse->accept();
//            ResizeGLWidget::g_wid->mousePressEvent(qMouse);
//            return true;
//           /* if (ResizeGLWidget::g_wid != ) 
//            {
//            
//            }*/
//                //QMouseEvent* qMouse = static_cast<QMouseEvent*>(event);
//                //curSeled_->mousePressEvent(qMouse);
//               // if (watched == *curSeled_)
//                //    return false;
//        }
//    }
//    else if (QEvent::MouseMove == event->type()) 
//    {
//            //if (watched != curSeled_)
//                //return true;
//        auto iter = std::find(focusSets_.begin(), focusSets_.end(), watched);
//        if (iter != focusSets_.end())
//        {
//            if (watched != ResizeGLWidget::g_wid)
//            {
//                qDebug() << "discarded MouseMove:" << (*iter)->objectName();
//                return true;
//            }
//            else
//            {
//                qDebug() << "MouseMove:" << (*iter)->objectName();
//            }
//        }
//    }
//    else if (QEvent::MouseButtonRelease == event->type())
//    {
//        auto iter = std::find(focusSets_.begin(), focusSets_.end(), watched);
//        if (iter != focusSets_.end())
//        {
//            if (watched != ResizeGLWidget::g_wid)
//            {
//                qDebug() << "discarded MouseButtonRelease:" << (*iter)->objectName();
//                return true;
//                //return true;
//                //QMouseEvent* qMouse = static_cast<QMouseEvent*>(event);
//                //curSeled_->mouseReleaseEvent(qMouse);
//                //return true;
//            }
//            else
//            {
//                qDebug() << "MouseButtonRelease:" << (*iter)->objectName();
//            }
//        }
//    }
//    return QWidget::eventFilter(watched, event);
//}
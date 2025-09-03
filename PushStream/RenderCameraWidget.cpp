#include "RenderCameraWidget.h"
#include "VideoCapture/camera.hpp"
#include "PicToYuv.h"
#include <QMouseEvent>
#include<qmessagebox.h>
#include <QDebug>

#ifndef min
#define min(a,b) ((a)<(b)? (a) :(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)? (a) :(b))
#endif

RenderCameraWidget::RenderCameraWidget(QWidget* pParentWidget)
    : ResizeGLWidget(pParentWidget)
    ,f_(av_frame_alloc())
{
}

RenderCameraWidget::~RenderCameraWidget()
{
    m_camera->setCaptureFrameCallBack(nullptr);
    if (f_) 
    {
        //av_frame_unref(f_);
        av_frame_free(&f_);
    }
}

bool RenderCameraWidget::openCamera(const std::wstring& strCamera)
{
    decltype(m_camera) camera = decltype(m_camera)(new Camera, [](Camera* p) {p->Close(), delete p; });
    return openCamera(std::move(camera), strCamera);
}

bool RenderCameraWidget::openCamera(std::shared_ptr<Camera>camera, const std::wstring& strCamera)
{
    //m_camera = std::move(camera);
    m_camera = camera;
    m_camera->Close();
    std::function<bool(UPTR_FME)>fun = [this](UPTR_FME f)->bool
        {
            GetFrame(std::move(f));
            return true;
        };

    m_camera->addCaptureFrameCallBack(fun);
    if (!m_camera->Open(strCamera))
    {
        QMessageBox::warning(nullptr, "error", QString("open %1 error\n").arg(QString::fromStdWString(strCamera)), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return false;
    }
    return true;
}

std::pair<int, int> RenderCameraWidget::setFrameSizeAccordCamera()
{
    int w = m_camera->GetWidth();
    int h = m_camera->GetHeight();
    setFrameSize(w, h);
    return { w,h };
}

std::shared_ptr<Camera>RenderCameraWidget::camera() 
{
    return m_camera;
}

void RenderCameraWidget::GetFrame(UPTR_FME f)
{
        {
            std::lock_guard<std::mutex>lock(mutex_);
            av_frame_unref(f_);
            av_frame_ref(f_, &*f);
            //av_frame_copy_props(f_, f);
        }
        renderFrame(std::move(f));
}

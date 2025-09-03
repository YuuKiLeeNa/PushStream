#include "ResizeGLWidget.h"
class Camera;
class PicToYuv;
struct AVFrame;

class RenderCameraWidget :public ResizeGLWidget 
{
public:
    RenderCameraWidget(QWidget* pParentWidget);
    virtual ~RenderCameraWidget();
    bool openCamera(const std::wstring& strCamera);
    bool openCamera(std::shared_ptr<Camera> camera, const std::wstring& strCamera);
    std::pair<int, int>setFrameSizeAccordCamera();
    std::shared_ptr<Camera>camera();

protected:
    void GetFrame(UPTR_FME f);
    std::shared_ptr<Camera>m_camera;
    std::mutex mutex_;
    AVFrame* f_ = nullptr;
};
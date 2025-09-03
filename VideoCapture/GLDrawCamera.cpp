#include "GLDrawCamera.h"
#include <functional>
#include <opencv2/opencv.hpp>

GLDrawCamera::GLDrawCamera()
{
}

GLDrawCamera::GLDrawCamera(int frameW, int frameH, float scaleFactor) 
{
	setFrameWH(frameW, frameH);
	setScaleFactor(scaleFactor);
}

void GLDrawCamera::setCameraCallGenTex()
{
	/*std::function<void(AVFrame*)>rgba_callback = [this](AVFrame*f)
		{
			std::shared_ptr<CSourceType>ptr = this->shared_from_this();
			if (ptr) {
				std::shared_ptr<GLDrawCamera>ptr_camera = std::static_pointer_cast<GLDrawCamera>(ptr);
				ptr_camera->setData(f);
			}
		};*/

	auto ptr = std::static_pointer_cast<GLDrawCamera>(this->shared_from_this());
	std::weak_ptr<decltype(ptr)::element_type>wptr(ptr);
	std::function<bool(UPTR_FME f)>fun = [wptr](UPTR_FME f)
		{
			auto ptr_s = wptr.lock();
			if (ptr_s)
			{
				ptr_s->setData_callback(std::move(f));
				return true;
			}
			return false;
		};
	ptr->setCaptureFrameCallBack(fun);
	//setCaptureFrameCallBack(rgba_callback);
}

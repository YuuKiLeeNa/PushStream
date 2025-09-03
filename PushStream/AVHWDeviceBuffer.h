#pragma once
#ifdef  __cplusplus
extern "C" {
#endif
#include "libavutil/hwcontext.h"
#ifdef  __cplusplus
}
#endif

class AVHWDeviceBuffer 
{
public:
	 AVBufferRef* hwdevice_ = nullptr;
	 AVBufferRef* hwframe_ = nullptr;
	 void reset();
	 ~AVHWDeviceBuffer();
	 int initCuda(int w, int h);
};

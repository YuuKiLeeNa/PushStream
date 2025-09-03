#pragma once
#include "camera.hpp"
#include "vmath.h"
//#include "CalcGLDraw.h"

class GLDrawCamera :public Camera
{
public:
	GLDrawCamera();
	GLDrawCamera(int frameW, int frameH,float scaleFactor = 1.0f);
	void setCameraCallGenTex();
public:

};

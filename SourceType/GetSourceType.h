#pragma once
#include "CSourceType.h"

#include "OpenGLFrameBuff/GLDrawText.h"
#include "Audio/CSource.h"
#include "VideoCapture/GLDrawCamera.h"
#include "openImg/openImg.h"

//CSourceType
//enum SourceDefineType { ST_FREE_TYPE_TEXT, ST_PIC, ST_AUDIO, ST_MEDIA, ST_CAMERA };

template<int N>
struct GetSourceType;

template<>
struct GetSourceType<CSourceType::ST_FREE_TYPE_TEXT>
{
	using value_type = GLDrawText;
};

template<>
struct GetSourceType<CSourceType::ST_AUDIO>
{
	using value_type = CSource;
};

template<>
struct GetSourceType<CSourceType::ST_CAMERA>
{
	using value_type = GLDrawCamera;
};

template<>
struct GetSourceType<CSourceType::ST_PIC>
{
	using value_type = openImg;
};

template<int N>
using GetSourceType_t = typename GetSourceType<N>::value_type;
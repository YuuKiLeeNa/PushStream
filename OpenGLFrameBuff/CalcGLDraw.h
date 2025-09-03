#pragma once
#include<QOpenGLTexture>
#include "PushStream/qt_define_type.h"
#include <opencv2/opencv.hpp>
#include "vmath.h"
#include <atomic>
#include "SourceType/CSourceType.h"
#include"PushStream/define_type.h"
#include<mutex>
#include<QOpenGLFunctions>
#include<QOffscreenSurface>

class CalcGLDraw :public CSourceType
{
public:
	CalcGLDraw(CSourceType::SourceDefineType type);
	CalcGLDraw(CalcGLDraw&& obj)noexcept;
	CalcGLDraw& operator=(CalcGLDraw&& obj)noexcept;

	void setPosition(int x, int y) { m_xCol = x; m_yRow = y; }
	void setFrameWH(int frameW, int frameH) {
		m_wFrame = frameW;
		m_hFrame = frameH;
	}
	//void init(cv::Mat imageRGB);
	void setData(cv::Mat imageRGBA);
	void setData_callback(UPTR_FME f);
	void setData_callback(cv::Mat imageRGBA);
	void setData();
	void calcGLDrawRect(cv::Mat imageRGBA);
	void setScaleFactor(float f) { m_fScale = f; }
	
	
	
	UPTR_QTex m_tex;
	//顶点坐标
	vmath::vec4 m_vec;
	//帧缓冲区(GLFrameBuffer),帧宽，帧高
	int m_wFrame = 0;
	int m_hFrame = 0;
	//左上角0,0
	int m_xCol = 0;
	int m_yRow = 0;
	//图像宽高
	std::atomic_int m_wImg = 0;
	std::atomic_int m_hImg = 0;
	float m_fScale = 1.0;

	cv::Mat RGBA_data_;
	std::mutex m_mutexRGBA;

	//only use for async.for example camera
	std::atomic_bool m_bIsTexChanged = false;
	//std::atomic_bool m_bIsAsync = false;
	


};
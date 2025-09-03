#pragma once
#include "OpenGLFrameBuff/CalcGLDraw.h"
#include<string>
#include "opencv2/opencv.hpp"
#include<thread>
#include<atomic>
#include<functional>
#include<mutex>
#include"define_type.h"

class openImg :public CalcGLDraw 
{
public:
	openImg();
	~openImg();
	bool open(const std::string& pathFile);
	void stop();
	void setCallBack(const std::function<void(UPTR_FME)>& callback);

	void thread_fun(const std::string& pathFile);
	UPTR_FME makeFrame(cv::Mat imgRgba=cv::Mat())const;

	std::function<void(UPTR_FME)>m_RGBA_callback;
	std::atomic_bool m_bIsStop;
	cv::Mat m_img;
	std::thread m_th;
	std::string m_file;
	std::mutex m_mutex_callback;

};
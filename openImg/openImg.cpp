#include "openImg.h"
#include<chrono>
#include "define_type.h"

#ifdef __cplusplus
extern "C"{
#endif

#include "libavutil/frame.h"

#ifdef __cplusplus
}
#endif


openImg::openImg():CalcGLDraw(CSourceType::ST_PIC)
{
}

openImg::~openImg()
{
	stop();
}

bool openImg::open(const std::string& pathFile)
{
	//setAsync(false);
	stop();
	m_file = pathFile;
	cv::VideoCapture vc;// (pathFile);


	//CAP_FFMPEG
	if (!vc.open(pathFile, cv::VideoCaptureAPIs::CAP_IMAGES))
	{
		if(!vc.open(pathFile, cv::VideoCaptureAPIs::CAP_ANY))
			return false;
	}
	cv::Mat tmp;
	if (!vc.read(tmp))
		return false;

	if (!tmp.empty())
	{
		int itype = tmp.type();
		switch (itype)
		{
		case CV_8UC3:
			cv::cvtColor(tmp, m_img, cv::COLOR_BGR2RGBA);
			break;
		case CV_8UC1:
			cv::cvtColor(tmp, m_img, cv::COLOR_GRAY2RGBA);
			break;
		case CV_8UC4:
			cv::cvtColor(tmp, m_img, cv::COLOR_BGRA2RGBA);
			break;
		default:
			break;
		}
		if (m_img.empty())
			return false;
		
		if (vc.read(tmp)) 
		{
			if (!tmp.empty()) 
			{
				m_bIsStop = false;
				 m_th = std::thread(&openImg::thread_fun, this, pathFile);
				 return true;
			}
		}

		//setData(m_img);
		setData_callback(m_img);
		std::lock_guard<std::mutex>lock(m_mutex_callback);
		if (m_RGBA_callback)
		{
			UPTR_FME fme(av_frame_alloc());
			if (!fme)
				return false;
			fme->width = m_img.cols;
			fme->height = m_img.rows;
			fme->format = AV_PIX_FMT_RGBA;
			if (av_frame_get_buffer(fme.get(), 1)!=0) 
				return false;
			if(av_frame_make_writable(fme.get()) != 0)
				return false;
			memmove(fme->data[0], m_img.data, fme->width* fme->height*4);
			m_RGBA_callback(std::move(fme));
		}
		return true;
	}
	else 
		return false;
}

void openImg::stop() 
{
	m_bIsStop = true;
	if (m_th.joinable()) 
		m_th.join();
}

void openImg::setCallBack(const std::function<void(UPTR_FME)>& callback)
{
	std::lock_guard<std::mutex>lock(m_mutex_callback);
	m_RGBA_callback = callback;
}

void openImg::thread_fun(const std::string& pathFile)
{
	cv::Mat tmp;
	//setAsync(true);
	while (!m_bIsStop)
	{
		cv::VideoCapture capture(pathFile);
		double fps = capture.get(cv::CAP_PROP_FPS);
		if (fps <= 0.0)
			fps = 10;
		while (capture.read(tmp) && !m_bIsStop)
		{
			int itype = tmp.type();
			switch (itype)
			{
			case CV_8UC3:
				cv::cvtColor(tmp, m_img, cv::COLOR_BGR2RGBA);
				break;
			case CV_8UC1:
				cv::cvtColor(tmp, m_img, cv::COLOR_GRAY2RGBA);
				break;
			case CV_8UC4:
				cv::cvtColor(tmp, m_img, cv::COLOR_BGRA2RGBA);
				break;
			default:
				break;
			}
			if (!m_img.empty())
			{
				setData_callback(m_img);
				std::lock_guard<std::mutex>lock(m_mutex_callback);
				if (m_RGBA_callback)
				{
					UPTR_FME fme = makeFrame();
					if(fme)
						m_RGBA_callback(std::move(fme));
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000 / fps)));
		}
	}
}

UPTR_FME openImg::makeFrame(cv::Mat imgRgba) const
{
	if (imgRgba.empty())
	{
		if(m_img.empty())
			return UPTR_FME();
		imgRgba = m_img;
	}
	UPTR_FME fme(av_frame_alloc());
	if (fme)
	{
		fme->width = imgRgba.cols;
		fme->height = imgRgba.rows;
		fme->format = AV_PIX_FMT_RGBA;
		if (av_frame_get_buffer(fme.get(), 1) == 0)
		{
			if (av_frame_make_writable(fme.get()) == 0)
				memmove(fme->data[0], m_img.data, fme->width * fme->height * 4);
		}
	}
	return fme;
}

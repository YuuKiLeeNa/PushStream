#pragma once
#include "VideoCapture.h"
#include <vector>
#include <functional>
#include <dshow.h>
#include <windows.h>
#include<string>
#include<atomic>
#include "qedit.h"
#include<mutex>
//#include"filter/VideoFilter.h"
#include<Windows.h>
#include "PushStream/AVHWDeviceBuffer.h"
#include "SourceType/CSourceType.h"
#include"PushStream/imageConvert.h"
#include "PushStream/define_type.h"
#include "OpenGLFrameBuff/CalcGLDraw.h"



class Camera:/*public VideoFilter, public VideoCapture,*/ public CalcGLDraw
{
public:
	Camera();
	Camera(const Camera &) = delete;
	Camera& operator  =(const Camera&) = delete;
	virtual ~Camera();
	std::wstring cameraName() { return camera_name_; }
public:
	bool Init(int display_index = 0);
	bool Destroy() { return Close(); };
	bool  CaptureFrame(AVFrame* f);
	bool CaptureStarted() const { return false; };
	std::vector<std::wstring> EnumAllCamera(void);
	bool Open(const std::wstring &camera_name);
	bool Close(void);

	/*!
	* @param time : Starting time of the sample, in seconds.
	* @param buff : Pointer to a buffer that contains the sample data.
	* @param len  : Length of the buffer pointed to by pBuffer, in bytes.
	*/
	//void SetCallBack(const std::function<void(double time, BYTE *buff, LONG len)>& fun);
	//void SetFrameCallBack(const std::function<void(AVFrame*)>& fun);
	void setCaptureFrameCallBack(const std::function<bool(UPTR_FME)>& fun);
	void addCaptureFrameCallBack(const std::function<bool(UPTR_FME)>& fun);
	void clearVCaptureFrameCallBack() { std::lock_guard<std::mutex>lock(mutex_vCaptureCallBack); m_vCaptureCallBack.clear(); }
	uint32_t GetHeight()const { return mVideoHeight; }
	uint32_t GetWidth()const { return mVideoWidth; }
	int GetBitDepth()const { return mBitDepth; }

	void operator()(double time, BYTE* buff, LONG len);
protected:

	bool close_nomutex();

	//void resetFilter();
	//int initFilter();

protected:
	//AVFrame* f_ = nullptr;
	UPTR_FME f_;
	std::mutex RGBFrameMutex_;
	//AVFrame* fyuv420p_ = nullptr;
	UPTR_FME fRGBA_;
	bool mInitOK;
	bool mIsVideoOpened;

	int mVideoWidth, mVideoHeight, mBitDepth;
	AVRational framerate_;
	std::mutex mutex_;
	std::unique_ptr<std::function<bool(UPTR_FME)>> mCaptureCallBack;

	//std::unique_ptr<std::function<void(AVFrame*)>> mCaptureCallBack_RGB;
	//AVFilterContext* buffer_ = nullptr;
	//AVFilterContext* sink_ = nullptr;
	imageConvert cvt;
	std::wstring camera_name_;

	std::mutex mutex_vCaptureCallBack;
	std::vector<std::function<bool(UPTR_FME)>>m_vCaptureCallBack;

protected:
	class SampleGrabberCallback : public ISampleGrabberCB
	{
	public:
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
		HRESULT STDMETHODCALLTYPE SampleCB(double Time, IMediaSample* pSample);
		HRESULT STDMETHODCALLTYPE BufferCB(double Time, BYTE* pBuffer, long BufferLen);
		std::function<void(double, BYTE*, LONG)> mNewDataCallBack;
	};
	IGraphBuilder* mGraphBuilder;
	ICaptureGraphBuilder2* mCaptureGB;
	IMediaControl* mMediaControl;
	IBaseFilter* mDevFilter;
	ISampleGrabber* mSampGrabber;
	IMediaEventEx* mMediaEvent;

	SampleGrabberCallback mSampleGrabberCB;
	HRESULT InitializeEnv();
	HRESULT BindFilter(int deviceID, IBaseFilter** pBaseFilter);
	//AVHWDeviceBuffer hw_;
};
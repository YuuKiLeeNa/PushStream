#include "camera.hpp"
#ifdef __cplusplus
extern"C" {
#endif
#include"libavutil/frame.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif
#include<assert.h>
//#include<QDebug>
#include<iterator>
#include<algorithm>
#include<functional>
#include<numeric>
#include "opencv2/opencv.hpp"

#pragma comment(lib, "strmiids")

//define release maco
#define ReleaseInterface(x) \
	if ( NULL != x ) \
{ \
	x->Release( ); \
	x = NULL; \
}
// Application-defined message to notify app of filter graph events
#define WM_GRAPHNOTIFY  WM_APP+100

Camera::Camera():CalcGLDraw(CSourceType::ST_CAMERA),
	fRGBA_(av_frame_alloc()),
	mInitOK(false),
	mVideoHeight(0),
	mVideoWidth(0),
	mDevFilter(NULL),
	mCaptureGB(NULL),
	mGraphBuilder(NULL),
	mMediaControl(NULL),
	mMediaEvent(NULL),
	mSampGrabber(NULL),
	mIsVideoOpened(false)
{
	//HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//HRESULT hr = CoInitialize(NULL);
	//if (SUCCEEDED(hr))
	//{
	//	mInitOK = true;
	//}
}

Camera::~Camera()
{
	Close();
	CoUninitialize();
}

bool Camera::Init(int display_index)
{
	std::vector<std::wstring>sets = EnumAllCamera();
	if (display_index >= 0 && display_index < sets.size())
		return Open(sets[display_index]);
	return false;
}

bool Camera::CaptureFrame(AVFrame* f)
{
	av_frame_unref(f);
	std::lock_guard<std::mutex>lock(mutex_);
	if (!fRGBA_)
		return false;
	if (av_frame_ref(f, fRGBA_.get()) != 0)
		return false;
	/*if (av_frame_copy_props(f, fyuv420p_) < 0)
		return false;*/
	return true;
}

HRESULT Camera::InitializeEnv()
{
	HRESULT hr;

	//Create the filter graph
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, (LPVOID*)&mGraphBuilder);
	if (FAILED(hr))
		return hr;

	//Create the capture graph builder
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
		IID_ICaptureGraphBuilder2, (LPVOID*)&mCaptureGB);
	if (FAILED(hr))
		return hr;

	//Obtain interfaces for media control and Video Window
	hr = mGraphBuilder->QueryInterface(IID_IMediaControl, (LPVOID*)&mMediaControl);
	if (FAILED(hr))
		return hr;


	hr = mGraphBuilder->QueryInterface(IID_IMediaEventEx, (LPVOID*)&mMediaEvent);
	if (FAILED(hr))
		return hr;

	mCaptureGB->SetFiltergraph(mGraphBuilder);
	if (FAILED(hr))
		return hr;
	return hr;
}

std::vector<std::wstring> Camera::EnumAllCamera(void)
{
	//if (mInitOK == false)
	//	return std::vector<std::wstring>();

	std::vector<std::wstring> names;
	IEnumMoniker *pEnum;
	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the category.
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
		}
		pDevEnum->Release();
	}

	if (!SUCCEEDED(hr))
		return std::vector<std::wstring>();

	IMoniker *pMoniker = NULL;
	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag *pPropBag;
		IBindCtx* bindCtx = NULL;
		LPOLESTR str = NULL;
		VARIANT var;
		VariantInit(&var);

		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;
		}

		// Get description or friendly name.
		hr = pPropBag->Read(L"Description", &var, 0);
		if (FAILED(hr))
		{
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
		}
		if (SUCCEEDED(hr))
		{
			names.push_back(var.bstrVal);
			VariantClear(&var);
		}

		pPropBag->Release();
		pMoniker->Release();
	}

	pEnum->Release();

	return names;
}

HRESULT Camera::BindFilter(int deviceID, IBaseFilter **pBaseFilter)
{
	ICreateDevEnum *pDevEnum;
	IEnumMoniker   *pEnumMon;
	IMoniker	   *pMoniker;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (LPVOID*)&pDevEnum);
	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMon, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;
			return hr;
		}
		pEnumMon->Reset();
		ULONG cFetched;
		int index = 0;
		hr = pEnumMon->Next(1, &pMoniker, &cFetched);
		while (hr == S_OK && index <= deviceID)
		{
			IPropertyBag *pProBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (LPVOID*)&pProBag);
			if (SUCCEEDED(hr))
			{
				if (index == deviceID)
				{
					pMoniker->BindToObject(0, 0, IID_IBaseFilter, (LPVOID*)pBaseFilter);
				}
			}
			pMoniker->Release();
			index++;
			hr = pEnumMon->Next(1, &pMoniker, &cFetched);
		}
		pEnumMon->Release();
	}
	return hr;
}


bool Camera::Open(const std::wstring &camera_name)
{
	//if (mInitOK == false)
	//	return false;

	if (mIsVideoOpened)
		return true;
	
	HRESULT hr;

#define CHECK_HR(x) do{ hr = (x); if (FAILED(hr)){ Close(); return false;}}while(0)

	CHECK_HR(InitializeEnv());
	
	IBaseFilter *pSampleGrabberFilter , *dest_filter;

	std::vector<std::wstring> names = EnumAllCamera();

	if (names.empty())
	{
		Close(); return false;
	}

	auto name_it = find(names.begin(), names.end(), camera_name);
	if (name_it == names.end())
	{
		Close(); return false;
	}
	camera_name_ = camera_name;

	int deviceID = static_cast<int>(distance(names.begin(), name_it));


	// create grabber filter instance
	CHECK_HR(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, (LPVOID*)&pSampleGrabberFilter));

	// bind source device
	CHECK_HR(BindFilter(deviceID, &mDevFilter));
	
	// add src filter
	CHECK_HR(mGraphBuilder->AddFilter(mDevFilter, L"Video Filter"));
	
	// add grabber filter and query interface
	CHECK_HR(mGraphBuilder->AddFilter(pSampleGrabberFilter, L"Sample Grabber"));
	
	CHECK_HR(pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (LPVOID*)&mSampGrabber));
	
	// find the current bit depth
	HDC hdc = GetDC(NULL);
	mBitDepth = GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(NULL, hdc);

	// set the media type for grabber filter
	AM_MEDIA_TYPE mediaType;
	ZeroMemory(&mediaType, sizeof(AM_MEDIA_TYPE));
	mediaType.majortype = MEDIATYPE_Video;
	{
		std::lock_guard<std::mutex>lock(RGBFrameMutex_);
		f_.reset(av_frame_alloc());
		if (!f_)
			return false;
		switch (mBitDepth)
		{
		case  8:
			f_->format = AV_PIX_FMT_RGB8;
			mediaType.subtype = MEDIASUBTYPE_RGB8;
			break;
		case 16:
			f_->format = AV_PIX_FMT_RGB555;
			mediaType.subtype = MEDIASUBTYPE_RGB555;
			break;
		case 24:
			f_->format = AV_PIX_FMT_RGB24;
			mediaType.subtype = MEDIASUBTYPE_RGB24;
			break;
		case 32:
			f_->format = AV_PIX_FMT_RGB32;
			mediaType.subtype = MEDIASUBTYPE_RGB32;
			break;
		default:
			close_nomutex();
			return false;
		}

		mediaType.formattype = FORMAT_VideoInfo;
		hr = mSampGrabber->SetMediaType(&mediaType);

		CHECK_HR(CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&dest_filter)));
		mGraphBuilder->AddFilter(dest_filter, L"NullRenderer");

		// connect source filter to grabber filter
		CHECK_HR(mCaptureGB->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
			mDevFilter, pSampleGrabberFilter, dest_filter));
	
		// get connected media type
		CHECK_HR(mSampGrabber->GetConnectedMediaType(&mediaType));
		VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*)mediaType.pbFormat;
		mVideoWidth = vih->bmiHeader.biWidth;
		mVideoHeight = vih->bmiHeader.biHeight;

		//framerate_ = { (int)vih->AvgTimePerFrame * 10000000, 1 };

		f_->width = mVideoWidth;
		f_->height = mVideoHeight;
		f_->sample_aspect_ratio = { 1,1 };
		if (av_frame_get_buffer(f_.get(), 1) != 0)
		{
			close_nomutex();
			return false;
		}
		if (av_frame_make_writable(f_.get()) != 0)
		{
			close_nomutex();
			return false;
		}
		if ((AVPixelFormat)f_->format != AV_PIX_FMT_RGBA)
			cvt.init((AVPixelFormat)f_->format, f_->width, f_->height
				,AV_PIX_FMT_RGBA, f_->width, f_->height);


	}
	//assert(f_->linesize[0]);
	// configure grabber filter
	CHECK_HR(mSampGrabber->SetOneShot(0));
	
	CHECK_HR(mSampGrabber->SetBufferSamples(0));
	
	// Use the BufferCB callback method
	CHECK_HR(mSampGrabber->SetCallback(&mSampleGrabberCB, 1));

	mSampleGrabberCB.mNewDataCallBack = std::bind(&Camera::operator(), this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	//hw_.initCuda(mVideoWidth, mVideoHeight);
	/*if (initFilter() < 0)
	{
		Close();
		return false;
	}*/

	mMediaControl->Run();
	dest_filter->Release();
	pSampleGrabberFilter->Release();
	
	// release resource
	if (mediaType.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mediaType.pbFormat);
		mediaType.cbFormat = 0;
		mediaType.pbFormat = NULL;
	}
	if (mediaType.pUnk != NULL)
	{
		mediaType.pUnk->Release();
		mediaType.pUnk = NULL;
	}
	mIsVideoOpened = TRUE;
	return true;
}

bool Camera::Close()
{
	camera_name_.clear();
	if (mMediaControl)
	{
		mMediaControl->Stop();
	}
	if (mMediaEvent)
	{
		mMediaEvent->SetNotifyWindow(NULL, WM_GRAPHNOTIFY, 0);
	}
	//av_frame_unref(f_);
	{
		std::lock_guard<std::mutex>lock(RGBFrameMutex_);
		f_.reset();
		//av_frame_free(&f_);
	}
	mIsVideoOpened = false;
	//release interface
	ReleaseInterface(mDevFilter);
	ReleaseInterface(mCaptureGB);
	ReleaseInterface(mGraphBuilder);
	ReleaseInterface(mMediaControl);
	ReleaseInterface(mMediaEvent);
	ReleaseInterface(mSampGrabber);
	//resetFilter();
	//std::lock_guard<std::mutex>lock(mutex_);
	//mCaptureCallBack.reset();
	//hw_.reset();
	clearVCaptureFrameCallBack();

	return true;
}

//void Camera::SetFrameCallBack(const std::function<void(AVFrame*)>& fun)
//{
//	std::lock_guard<std::mutex>lock(mutex_);
//	mFrameCallBack = fun;
//}

void Camera::setCaptureFrameCallBack(const std::function<bool(UPTR_FME)>& fun)
{
	std::lock_guard<std::mutex>lock(mutex_);
	mCaptureCallBack = std::make_unique<decltype(mCaptureCallBack)::element_type>(fun);
}

void Camera::addCaptureFrameCallBack(const std::function<bool(UPTR_FME)>& fun)
{
	std::lock_guard<std::mutex>lock(mutex_vCaptureCallBack);
	m_vCaptureCallBack.emplace_back(fun);
}

//void Camera::setCaptureFrameCallBack_RGB(const std::function<void(AVFrame*)>& fun)
//{
//	std::lock_guard<std::mutex>lock(RGBFrameMutex_);
//	mCaptureCallBack_RGB = std::make_unique<decltype(mCaptureCallBack_RGB)::element_type>(fun);
//}

void Camera::operator()(double time, BYTE* buff, LONG len) 
{
	std::unique_lock<std::mutex>lockRGB(RGBFrameMutex_);
	int size = av_image_get_buffer_size((AVPixelFormat)f_->format, f_->width, f_->height, 1);
	if (len >= size)
	{
		if (av_frame_make_writable(f_.get()) != 0)
			return;
		memcpy(f_->data[0], buff, size);

		av_frame_unref(fRGBA_.get());
		if (cvt)
		{
			fRGBA_->width = f_->width;
			fRGBA_->height = f_->height;
			fRGBA_->format = AV_PIX_FMT_RGBA;
			fRGBA_->sample_aspect_ratio = { 1,1 };
			int ret;
			if ((ret = av_frame_get_buffer(fRGBA_.get(), 1)) != 0)
				return;
			if ((ret = av_frame_make_writable(fRGBA_.get())) != 0)
				return;
			if (!cvt(f_->data, f_->linesize, fRGBA_->data, fRGBA_->linesize))
				return;
		}
		else 
		{
			int ret = av_frame_ref(fRGBA_.get(), f_.get());
			if (ret != 0)
				return;
		}

		cv::Mat matRGBA(cv::Size(fRGBA_->width, fRGBA_->height), CV_8UC4, fRGBA_->data[0]), matRGBAflip;

		cv::flip(matRGBA, matRGBAflip, 0);

		memcpy(fRGBA_->data[0], matRGBAflip.data, fRGBA_->width * fRGBA_->height * 4);

		AVFrame *tmp = av_frame_clone(fRGBA_.get());
		lockRGB.unlock();
		if (!tmp)
			return;
		{
			std::lock_guard<std::mutex>lock(mutex_);
			if (mCaptureCallBack)
				if (!(*mCaptureCallBack)(UPTR_FME(tmp)))
					mCaptureCallBack = nullptr;
		}
		{
			std::lock_guard<std::mutex>lock(mutex_vCaptureCallBack);
			auto iter = m_vCaptureCallBack.begin();
			while (iter != m_vCaptureCallBack.end()) 
			{
				UPTR_FME tmp(av_frame_clone(fRGBA_.get()));
				if (!(*iter)(std::move(tmp)))
					iter = m_vCaptureCallBack.erase(iter);
				else
					++iter;
			}
		}
		//if (mCaptureCallBack_RGB)
		//	(*mCaptureCallBack_RGB)(f_);
		//av_frame_unref(tmp_);

		//std::unique_lock<std::mutex>lock(data_.mutex_);
		//if (av_buffersrc_add_frame_flags(buffer_, f_, AV_BUFFERSRC_FLAG_KEEP_REF | AV_BUFFERSRC_FLAG_PUSH) >= 0)
		//{
		//	lockRGB.unlock();
		//	if (av_buffersink_get_frame_flags(sink_, tmp_, AV_BUFFERSINK_FLAG_NO_REQUEST) >= 0)
		//	{
		//		lock.unlock();
		//		if (!(tmp_->flags & (AV_FRAME_FLAG_CORRUPT | AV_FRAME_FLAG_DISCARD)))
		//		{
		//			assert(tmp_->linesize[0] > 0);
		//			assert(tmp_->linesize[1] > 0);
		//			assert(tmp_->linesize[2] > 0);
		//			std::lock_guard<std::mutex>lock(mutex_);
		//			av_frame_unref(fyuv420p_);
		//			av_frame_ref(fyuv420p_, tmp_);
		//			//av_frame_copy_props(fyuv420p_, tmp_);
		//			AVFrame*tmp = av_frame_clone(fyuv420p_);
		//			if(mCaptureCallBack)
		//				(*mCaptureCallBack)(tmp);
		//		}
		//	}
		//}
	}
}


bool Camera::close_nomutex()
{
	if (mMediaControl)
	{
		mMediaControl->Stop();
	}
	if (mMediaEvent)
	{
		mMediaEvent->SetNotifyWindow(NULL, WM_GRAPHNOTIFY, 0);
	}

	//av_frame_free(&f_);
	
	mIsVideoOpened = false;
	//release interface
	ReleaseInterface(mDevFilter);
	ReleaseInterface(mCaptureGB);
	ReleaseInterface(mGraphBuilder);
	ReleaseInterface(mMediaControl);
	ReleaseInterface(mMediaEvent);
	ReleaseInterface(mSampGrabber);
	//resetFilter();
	std::lock_guard<std::mutex>lock(mutex_);
	mCaptureCallBack.reset();
	//hw_.reset();
	return true;
}



//void Camera::resetFilter() 
//{
//	std::lock_guard<std::mutex>lock(data_.mutex_);
//	data_.graph_.reset();
//	data_.filters_.clear();
//	buffer_ = nullptr;
//	sink_ = nullptr;
//}
//
//int Camera::initFilter() 
//{
//	int ret;
//	std::lock_guard<std::mutex>lock(data_.mutex_);
//	data_.graph_ = FilterBase::CORE_DATA::create_graph();
//	data_.filters_.clear();
//	std::shared_ptr<bufferFilter>ptrbuff = std::make_shared<bufferFilter>(mVideoWidth, mVideoHeight, AVRational{ 1,1 }, AVRational{ 1,25 }, AVRational{ 25,1 }, (AVPixelFormat)f_->format, data_);
//	//std::shared_ptr<bufferFilter>ptrbuff = std::make_shared<bufferFilter>(mVideoWidth, mVideoHeight, AVRational{ 1,1 }, AVRational{ 0,1 }, AVRational{ 0,1 }, (AVPixelFormat)f_->format, data_);
//	if (ptrbuff->createFilter(&buffer_) < 0)
//		return -1;
//
//
//	std::vector<std::unique_ptr<AVFilterContext, std::function<void(AVFilterContext*)>>>freeSets;
//	auto freeFun = [](AVFilterContext* ctx)
//	{
//		avfilter_free(ctx);
//	};
//	freeSets.reserve(5);
//	freeSets.push_back(decltype(freeSets)::value_type(buffer_, freeFun));
//	if (hw_.hwframe_)
//	{
//		AVBufferSrcParameters* par = av_buffersrc_parameters_alloc();
//		memset(par, 0, sizeof(AVBufferSrcParameters));
//		par->format = AV_PIX_FMT_NONE;
//		if (!(par->hw_frames_ctx = av_buffer_ref(hw_.hwframe_))) 
//		{
//			av_free(par);
//			return -1;
//		}
//		if (av_buffersrc_parameters_set(buffer_, par) != 0)
//		{
//			av_free(par);
//			return -1;
//		}
//	}
//
//	AVFilterContext* scale_cudaCtx = nullptr;
//	AVFilterContext* formatCtx = nullptr;
//	AVFilterContext* scaleCtx = nullptr;
//	AVFilterContext* flipCtx = nullptr;
//
//
//	auto setPtr = [this, &freeSets, &freeFun](std::shared_ptr<FilterBase> ptr, AVFilterContext** ctx)->int
//	{
//		if (ptr->createFilter(ctx) < 0)
//			return -1;
//		freeSets.push_back(std::remove_reference<decltype(freeSets)>::type::value_type(*ctx, freeFun));
//		if (hw_.hwdevice_ && !((*ctx)->hw_device_ctx = av_buffer_ref(hw_.hwdevice_)))
//			return -1;
//		return 0;
//	};
//
//	std::shared_ptr<formatFilter>ptrformat = std::make_shared<formatFilter>(AV_PIX_FMT_YUV420P, data_);
//	if (setPtr(ptrformat, &formatCtx) < 0)
//		return -1;
//
//	if (hw_.hwdevice_)
//	{
//		std::shared_ptr<scale_cudaFilter>ptrscale_cuda = std::make_shared<scale_cudaFilter>(mVideoWidth, mVideoHeight, 3, AV_PIX_FMT_YUV420P, data_);
//		if (setPtr(ptrscale_cuda, &scale_cudaCtx) < 0)
//			return -1;
//	}
//	if (!scale_cudaCtx)
//	{
//		std::shared_ptr<scaleFilter>ptrscale = std::make_shared<scaleFilter>(mVideoWidth, mVideoHeight, data_);
//		if (setPtr(ptrscale, &scaleCtx) < 0)
//			return -1;
//	}
//	std::shared_ptr<vflipFilter>ptrflip = std::make_shared<vflipFilter>(data_);
//	if (setPtr(ptrflip, &flipCtx) < 0)
//		return -1;
//
//	std::shared_ptr<buffersinkFilter>ptrsink = std::make_shared<buffersinkFilter>(AV_PIX_FMT_YUV420P, data_);
//	if (ptrsink->createFilter(&sink_) < 0)
//		return -1;
//
//	freeSets.push_back(decltype(freeSets)::value_type(sink_, freeFun));
//
//	if (scale_cudaCtx)
//	{
//		if(avfilter_link(buffer_, 0, formatCtx, 0) != 0
//			|| avfilter_link(formatCtx, 0, flipCtx, 0) != 0
//			|| avfilter_link(flipCtx, 0, scale_cudaCtx, 0) != 0
//			|| avfilter_link(scale_cudaCtx, 0, sink_, 0) != 0)
//			return -1;
//	}
//	else
//	{
//		/*if (avfilter_link(buffer_, 0, scaleCtx, 0) != 0
//			|| avfilter_link(scaleCtx, 0, flipCtx, 0) != 0
//			|| avfilter_link(flipCtx, 0, formatCtx, 0) != 0
//			|| avfilter_link(formatCtx, 0, sink_, 0) != 0)
//			return -1;*/
//		/*if (avfilter_link(buffer_, 0, formatCtx, 0) != 0
//			|| avfilter_link(formatCtx, 0, scaleCtx, 0) != 0
//			|| avfilter_link(scaleCtx, 0, flipCtx, 0) != 0
//			|| avfilter_link(flipCtx, 0, sink_, 0) != 0)
//			return -1;*/
//		if (avfilter_link(buffer_, 0, formatCtx, 0) != 0
//			|| avfilter_link(formatCtx, 0, flipCtx, 0) != 0
//			|| avfilter_link(flipCtx, 0, scaleCtx, 0) != 0
//			|| avfilter_link(scaleCtx, 0, sink_, 0) != 0)
//			return -1;
//	}
//
//	if ((ret = avfilter_graph_config(data_.graph_.get(), nullptr)) < 0)
//	{
//		char szErr[AV_ERROR_MAX_STRING_SIZE];
//		av_make_error_string(szErr, sizeof(szErr), ret);
//		return -1;
//	}
//
//	std::for_each(freeSets.begin(), freeSets.end(), [](decltype(freeSets)::reference c) {c.release(); });
//
//	return 0;
//}

ULONG STDMETHODCALLTYPE Camera::SampleGrabberCallback::AddRef()
{
	return 1;
}

ULONG STDMETHODCALLTYPE Camera::SampleGrabberCallback::Release()
{
	return 2;
}

HRESULT STDMETHODCALLTYPE Camera::SampleGrabberCallback::QueryInterface(REFIID riid, void** ppvObject)
{
	if (NULL == ppvObject) return E_POINTER;
	if (riid == __uuidof(IUnknown))
	{
		*ppvObject = static_cast<IUnknown*>(this);
		return S_OK;
	}
	if (riid == IID_ISampleGrabberCB)
	{
		*ppvObject = static_cast<ISampleGrabberCB*>(this);
		return S_OK;
	}
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE Camera::SampleGrabberCallback::SampleCB(double Time, IMediaSample *pSample)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE Camera::SampleGrabberCallback::BufferCB(double Time, BYTE * pBuffer, long BufferLen)
{
	if (mNewDataCallBack)
	{
		mNewDataCallBack(Time, pBuffer, BufferLen);
	}
	return S_OK;
}



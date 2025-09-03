#include "SelCamWidget.h"
#include "VideoCapture/GLDrawCamera.h"
#include "GLVideoWidget.h"
#include <QMessageBox>
#include "PicToYuv.h"
#include "VideoCapture/camera.hpp"
#include "QtGLVideoWidget.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif
#include"ResizeGLWidget.h"
#include<QBoxLayout>
#include<QSlider>

SelCamWidget::SelCamWidget(QWidget* pParentWidget) :SelCamWidget(std::make_shared<GLDrawCamera>(), pParentWidget) {}


SelCamWidget::SelCamWidget(std::shared_ptr<GLDrawCamera> pCamera, QWidget* pParentWidget) :QtBaseTitleMoveWidget(pParentWidget, true, false)
, m_camera(pCamera)
{
	f_ = av_frame_alloc();
	setTitleText(QString::fromWCharArray(L"选择相机"));
	setFixedSize(600, 400);
	initUI();
	initConnect();
}

SelCamWidget::~SelCamWidget() 
{
	//m_camera.reset();
	//m_camera->Close();
	//av_frame_unref(f_);
	av_frame_free(&f_);
}

std::shared_ptr<GLDrawCamera>SelCamWidget::getCamera()
{
	return m_camera;
}

std::wstring SelCamWidget::getCameraName()
{
	return m_selBox->currentText().toStdWString();
}

void SelCamWidget::initUI() 
{
	QWidget*wid = getBodyWidget();

	m_selBox = new QComboBox(wid);
	m_selBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	m_strCamSets = m_camera->EnumAllCamera();
	QStringList list;
	for (auto& ele : m_strCamSets)
		list << QString::fromStdWString(ele);
	m_selBox->addItems(list);
	m_glWidget = new std::remove_pointer<decltype(m_glWidget)>::type(wid);
	m_glWidget->setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);
	//m_glWidget->setYUV420pParameters(0,0);
	m_scaleSlider = new QSlider(Qt::Orientation::Horizontal, wid);
	m_scaleSlider->setRange(0, 1000);
	m_scaleSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_scaleSlider->setFixedHeight(18);
	//m_scaleSlider->setValue(1000);
	QLabel* label = new QLabel(QString::fromWCharArray(L"缩放"), wid);
	label->setScaledContents(true);
	QHBoxLayout* scaleLay = new QHBoxLayout;
	scaleLay->addWidget(label);
	scaleLay->addWidget(m_scaleSlider);

	QVBoxLayout* widLay = new QVBoxLayout;
	widLay->addWidget(m_selBox);
	widLay->addWidget(m_glWidget);
	widLay->addLayout(scaleLay);

	wid->setLayout(widLay);

	auto cameraName = m_camera->cameraName();
	auto iter = list.cend();
	if (!cameraName.empty()) 
	{
		iter = std::find_if(list.cbegin(), list.cend(), [cameraName](const decltype(list)::value_type &ele)
			{
				return ele.toStdWString() == cameraName;
			});

		if (iter != list.cend())
		{
			auto diff = std::distance(list.cbegin(), iter);
			m_selBox->setCurrentIndex(diff);
			//selBoxChanged(diff);
		}
		/*else if(!list.empty())
		{
			selBoxChanged(0);
		}*/
	}
	/*else if (!list.empty())
	{
		selBoxChanged(0);
	}*/


	int value = m_camera->m_fScale * 1000;
	if (value <= 0)
		value = 0;
	else if (value > 1000)
		value = 1000;
	m_scaleSlider->setValue(value);
}

void SelCamWidget::SlotSliderValueChanged(int value)
{
	m_camera->setScaleFactor((float)value/1000);
}

void SelCamWidget::selBoxChanged(int index) 
{
	/*int wid = m_camera->GetWidth();
	int hei = m_camera->GetHeight();
	int dep = m_camera->GetBitDepth();*/
	if (m_selBox->count() <= 0)
		return;
	if (index < 0)
		index = m_selBox->currentIndex();

	m_camera->Close();
	//int iisize = wid * hei * 4;
	//if (iisize > 0)
	//{
		av_frame_unref(f_);
		f_->width = 1920;
		f_->height = 1080;
		f_->format = AV_PIX_FMT_YUV420P;
		if (av_frame_get_buffer(f_, 1) != 0) 
			return;
		if (av_frame_make_writable(f_) != 0)
			return;
		const ptrdiff_t dst_linesize[4] = { 1920 ,1920 / 2, 1920 / 2,0 };
		if (av_image_fill_black(f_->data, dst_linesize, (AVPixelFormat)f_->format, AVCOL_RANGE_UNSPECIFIED, f_->width, f_->height) != 0)
			return;

		/*f_->format = AV_PIX_FMT_RGBA;
		if (av_frame_get_buffer(f_, 1) != 0)
			return;
		if (av_frame_make_writable(f_) != 0)
			return;
		const ptrdiff_t dst_linesize[4] = { f_->width*4, 0, 0, 0};
		if (av_image_fill_black(f_->data, dst_linesize, (AVPixelFormat)f_->format, AVCOL_RANGE_UNSPECIFIED, f_->width, f_->height) != 0)
			return;*/

		m_glWidget->renderFrame(UPTR_FME(av_frame_clone(f_)));

	//}



	if (m_strCamSets.empty() || index < 0 || index >= m_strCamSets.size())
		return;
	auto sptr = shared_from_this();

	std::weak_ptr<SelCamWidget>wptr(sptr);
	m_camera->addCaptureFrameCallBack([wptr](UPTR_FME f)->bool
		{
			auto sptr = wptr.lock();
			if (sptr)
			{
				sptr->GetFrame(std::move(f));
				return true;
			}
			return false;
		});
	if (!m_camera->Open(m_strCamSets[index]))
	{
		QMessageBox::warning(nullptr, "error", QString("open %1 error\n").arg(QString::fromStdWString(m_strCamSets[index])), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		return;
	}
}

//void SelCamWidget::GetFrame(long, uint8_t* buff, long len)
//{
//	/*static auto last_time = system_clock::now();
//	static auto last_avg_time = system_clock::now();
//	static int32_t dt_avg_v = 0, total_count = 0;
//	auto cur_time = system_clock::now();
//	auto dt = duration_cast<milliseconds>(cur_time - last_time);
//
//	total_count++;
//
//	if (total_count % 30 == 0) {
//		auto cur_avg_time = system_clock::now();
//		auto dt_avg = duration_cast<milliseconds>(cur_avg_time - last_avg_time);
//		dt_avg_v = (int32_t)dt_avg.count();
//		last_avg_time = cur_avg_time;
//	}
//
//	last_time = cur_time;
//	int32_t dt_v = (int32_t)dt.count();
//	printf("frame size: h = %d, w = %d, fps_rt: %0.02f fps: %0.02f (%d)\n",
//		camera->GetHeight(), camera->GetWidth(), 1000.0f / dt_v,
//		dt_avg_v ? 1000.0f * 30 / dt_avg_v : 0, total_count);
//
//	if (total_count == 100)
//	{*/
//
//		int iWidth = m_camera->GetWidth();
//		int iHeight = m_camera->GetHeight();
//		int iBitCount = m_camera->GetBitDepth();
//		if (m_lWidth != iWidth
//			|| m_lHeight != iHeight
//			|| m_iBitCount != iBitCount) 
//		{
//			m_lWidth = iWidth;
//			m_lHeight = iHeight;
//			m_iBitCount = iBitCount;
//			//m_glWidget->setYUV420pParameters(m_lWidth, m_lHeight);
//			m_con->setInfo(iWidth, iHeight, m_iBitCount);
//		}
//		//std::shared_ptr<uint8_t>data;
//		if ((*m_con)(buff, len, f_))
//			m_glWidget->renderFrame(av_frame_clone(f_));
//}

void SelCamWidget::GetFrame(UPTR_FME f)
{
	//int iWidth = m_camera->GetWidth();
	//int iHeight = m_camera->GetHeight();
	//int iBitCount = m_camera->GetBitDepth();
	//if (m_lWidth != iWidth
	//	|| m_lHeight != iHeight)
		//|| m_iBitCount != iBitCount)
	//{
	//	m_lWidth = iWidth;
	//	m_lHeight = iHeight;
		//m_iBitCount = iBitCount;
		//m_glWidget->setYUV420pParameters(m_lWidth, m_lHeight);
		//m_con->setInfo(iWidth, iHeight, m_iBitCount);
	//}
	//std::shared_ptr<uint8_t>data;
	//if ((*m_con)(f, f_))
	//av_frame_ref(f_, f);
	m_glWidget->renderFrame(std::move(f));
}

void SelCamWidget::initConnect() 
{
	QObject::connect(m_selBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selBoxChanged(int)));
	QObject::connect(m_scaleSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
}


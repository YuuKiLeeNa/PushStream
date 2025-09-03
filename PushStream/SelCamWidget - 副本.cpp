#include "SelCamWidget.h"
#include "CameraCapture/camera.hpp"
#include "GLVideoWidget.h"
#include <QMessageBox>
#include "PicToYuv.h"
#include "CameraCapture/camera.hpp"
#include "QtGLVideoWidget.h"

SelCamWidget::SelCamWidget(QWidget* pParentWidget) :QtBaseTitleMoveWidget(pParentWidget, true, false) 
, m_camera(new Camera, [](Camera* p) {p->Close(), delete p; })
, m_con(std::make_shared<PicToYuv>())
{
	f_ = av_frame_alloc();
	m_camera->SetCallBack([this](double d, BYTE*date, LONG len)
		{
			GetFrame(d, date, len);
		});
	setTitleText(QString::fromWCharArray(L"´ò¿ªÉãÏñÍ·"));
	setFixedSize(500,500);
	initUI();
	initConnect();
}

SelCamWidget::~SelCamWidget() 
{
	m_camera.reset();
	av_frame_unref(f_);
	av_frame_free(&f_);
}

std::unique_ptr<Camera, std::function<void(Camera*)>>SelCamWidget::getCamera()
{
	m_camera->Close();
	return std::move(m_camera);
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
	m_glWidget = new QtGLVideoWidget(wid);
	//m_glWidget->setYUV420pParameters(0,0);
	QVBoxLayout* widLay = new QVBoxLayout;
	widLay->addWidget(m_selBox);
	widLay->addWidget(m_glWidget);
	wid->setLayout(widLay);
	selBoxChanged(m_selBox->currentIndex());
}

void SelCamWidget::selBoxChanged(int index) 
{
	int wid = m_camera->GetWidth();
	int hei = m_camera->GetHeight();
	int dep = m_camera->GetBitDepth();

	m_camera->Close();
	//std::shared_ptr<uint8_t>data((uint8_t*)calloc(1,wid * hei*3/2));
	int iisize = wid * hei * 4;
	if (iisize > 0)
	{
		PicToYuv tran;
		if (tran.setInfo(wid, hei, dep))
		{
			std::shared_ptr<uint8_t>data((uint8_t*)calloc(1, iisize));
			if (data)
			{
				memset(data.get(), 0, iisize);
				std::shared_ptr<uint8_t>data1;
				if (tran(data.get(), iisize, f_))
					m_glWidget->setBackgroundImage(av_frame_clone(f_));
			}
		}
	}
	if (m_strCamSets.empty() || index < 0 || index >= m_strCamSets.size())
		return;

	if (!m_camera->Open(m_strCamSets[index]))
	{
		QMessageBox::warning(nullptr, "error", QString("open %1 error\n").arg(QString::fromStdWString(m_strCamSets[index])), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		return;
	}
}

void SelCamWidget::GetFrame(long, uint8_t* buff, long len)
{
	/*static auto last_time = system_clock::now();
	static auto last_avg_time = system_clock::now();
	static int32_t dt_avg_v = 0, total_count = 0;
	auto cur_time = system_clock::now();
	auto dt = duration_cast<milliseconds>(cur_time - last_time);

	total_count++;

	if (total_count % 30 == 0) {
		auto cur_avg_time = system_clock::now();
		auto dt_avg = duration_cast<milliseconds>(cur_avg_time - last_avg_time);
		dt_avg_v = (int32_t)dt_avg.count();
		last_avg_time = cur_avg_time;
	}

	last_time = cur_time;
	int32_t dt_v = (int32_t)dt.count();
	printf("frame size: h = %d, w = %d, fps_rt: %0.02f fps: %0.02f (%d)\n",
		camera->GetHeight(), camera->GetWidth(), 1000.0f / dt_v,
		dt_avg_v ? 1000.0f * 30 / dt_avg_v : 0, total_count);

	if (total_count == 100)
	{*/

		int iWidth = m_camera->GetWidth();
		int iHeight = m_camera->GetHeight();
		int iBitCount = m_camera->GetBitDepth();
		if (m_lWidth != iWidth
			|| m_lHeight != iHeight
			|| m_iBitCount != iBitCount) 
		{
			m_lWidth = iWidth;
			m_lHeight = iHeight;
			m_iBitCount = iBitCount;
			//m_glWidget->setYUV420pParameters(m_lWidth, m_lHeight);
			m_con->setInfo(iWidth, iHeight, m_iBitCount);
		}
		//std::shared_ptr<uint8_t>data;
		if ((*m_con)(buff, len, f_))
			m_glWidget->setBackgroundImage(av_frame_clone(f_));
}

void SelCamWidget::initConnect() 
{
	QObject::connect(m_selBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selBoxChanged(int)));
}


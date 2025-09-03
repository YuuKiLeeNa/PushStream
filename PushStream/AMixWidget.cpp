#include "AMixWidget.h"
#include<QLabel>
#include<QBoxLayout>

//#ifdef __cplusplus
//extern "C" {
//#endif
//#include "libavutil/avutil.h"
//#include "libavutil/samplefmt.h"
//#include "libavutil/channel_layout.h"
//#ifdef __cplusplus
//}
//#endif


AMixWidget::AMixWidget(QWidget* parentWidget) 
{
	initUI();
	initConnect();
}


PSSource AMixWidget::addAudioDevice(const std::string& name, const std::string& device_id, obs_peak_meter_type peak_calc_type, typename CWasapi<CSource>::SourceType type)
{
	PSSource source = std::make_shared<decltype(source)::element_type>(name, peak_calc_type, nullptr, type);
	if (device_id != "default")
	{
		source->setUseSpecifyDeviceId(device_id);
	}
	m_dbSets.push_back(0.0f);
	VolControl* volwid = new VolControl(source);
	source->setAudioDataCalcCallBack(std::bind(&VolControl::Audio_Data_Call_back, volwid, std::placeholders::_1, std::placeholders::_2));
	m_control.push_back(volwid);
	QWidget* wid = m_scrollArea->widget();
	volwid->setParent(wid);
	wid->layout()->addWidget(volwid);
	QObject::connect(volwid, SIGNAL(signalSendDB(float)), this, SLOT(slotSignalSendDB(float)));
	return source;
}

void AMixWidget::removeAudioDevice(PSSource source)
{
	auto iter = std::find_if(m_control.begin(), m_control.end(), [source](decltype(m_control)::value_type& ele)->bool
		{
			return ele->GetSource() == source;
		});

	if (iter != m_control.end())
	{
		std::ptrdiff_t dif = iter - m_control.begin();
		QWidget* wid = m_scrollArea->widget();
		wid->layout()->removeWidget(*iter);
		auto ptr = *iter;
		m_control.erase(iter);
		m_dbSets.erase(m_dbSets.begin() + dif);
		delete ptr;
	}
}

void AMixWidget::slotSignalSendDB(float db) 
{
	VolControl* pwid = (VolControl*)QObject::sender();
	if (!pwid)
		return;

	auto iter = std::find(m_control.begin(), m_control.end(), pwid);
	if (iter != m_control.end())
	{
		int index = iter - m_control.begin();
		//m_dbSets.resize(m_control.size(), 0.0f);
		m_dbSets[index] = db;
		emit signalSendDB((int)(iter - m_control.begin()), db);
	}
}

void AMixWidget::initUI() 
{
	setStyleSheet(R"(QSlider::groove:horizontal {
    border: 1px solid #999999;
    height: 8px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #6B8E23, stop:1 #8B008B);
    margin: 0 0;
}
QSlider::handle:horizontal {
    background: #CD4F39;
    border: 1px solid #5c5c5c;
    width: 18px;
    margin: -2px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */
    border-radius: 3px;
}
QCheckBox::indicator {
    width: 26px;
    height: 26px;
}

QCheckBox::indicator:unchecked {
    image: url(:/PushStream/icon/volume-normal.png);
}

QCheckBox::indicator:unchecked:hover {
    image: url(:/PushStream/icon/volume-hover.png);
}

QCheckBox::indicator:unchecked:pressed {
    image: url(:/PushStream/icon/volume-pressed.png);
}

QCheckBox::indicator:checked {
    image: url(:/PushStream/icon/volume-shut-normal.png);
}

QCheckBox::indicator:checked:hover {
    image: url(:/PushStream/icon/volume-shut-hover.png);
}

QCheckBox::indicator:checked:pressed {
    image: url(:/PushStream/icon/volume-shut-pressed.png);
}

QWidget{background-color:black;}

QLabel{color:white;}

)");


	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);


	QWidget* wid = new QWidget(this);

	QVBoxLayout* widLay = new QVBoxLayout;
	widLay->setAlignment(Qt::AlignTop);
	wid->setLayout(widLay);

	//initCaptureAudioDefaultDevice(wid);

	for (auto& ele : m_control)
		widLay->addWidget(ele);
	m_scrollArea->setWidget(wid);
}

void AMixWidget::initConnect() 
{
	//QObject::connect(m_sliderDesktop, SIGNAL(valueChanged(int)), this, SIGNAL(signalsDesktop(int)));
	//QObject::connect(m_sliderMicrophone, SIGNAL(valueChanged(int)), this, SIGNAL(signalsMicrophone(int)));
}

void AMixWidget::initCaptureAudioDefaultDevice(QWidget*parentWidget)
{
	PSSource source_desktop = std::make_shared<decltype(source_desktop)::element_type>("desktop audio", SAMPLE_PEAK_METER/*TRUE_PEAK_METER*/, nullptr, decltype(source_desktop)::element_type::SourceType::DeviceOutput);
	VolControl* WidgetDesktop = new VolControl(source_desktop);
	source_desktop->setAudioDataCalcCallBack(std::bind(&VolControl::Audio_Data_Call_back, WidgetDesktop,std::placeholders::_1, std::placeholders::_2));
	WidgetDesktop->setParent(parentWidget);
	m_control.push_back(WidgetDesktop);
	auto micDeviceSets = source_desktop->getMicrophoneDevices();
	if (micDeviceSets.empty())
		return;

	PSSource ptr_micphone = std::make_shared<decltype(ptr_micphone)::element_type>("micphone audio", SAMPLE_PEAK_METER/*TRUE_PEAK_METER*/, nullptr, decltype(ptr_micphone)::element_type::SourceType::Input);
	VolControl* pMicphone = new VolControl(ptr_micphone);
	ptr_micphone->setAudioDataCalcCallBack(std::bind(&VolControl::Audio_Data_Call_back, pMicphone, std::placeholders::_1, std::placeholders::_2));

	m_control.push_back(pMicphone);

	m_dbSets.resize(m_control.size(),0.0f);

	QObject::connect(WidgetDesktop, SIGNAL(signalSendDB(float)), this,SLOT(slotSignalSendDB(float)));

	QObject::connect(pMicphone, SIGNAL(signalSendDB(float)), this, SLOT(slotSignalSendDB(float)));
}

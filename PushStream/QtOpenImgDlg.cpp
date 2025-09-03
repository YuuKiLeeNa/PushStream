#include "QtOpenImgDlg.h"
#include<QBoxLayout>
#include<QFileDialog>
#include<QPixmap>
#include "Audio/utilfun.h"


QtOpenImgDlg::QtOpenImgDlg(QWidget* parentWid):QtOpenImgDlg(std::make_shared<openImg>(), parentWid)
{
}

QtOpenImgDlg::QtOpenImgDlg(std::shared_ptr<openImg> openImg, QWidget* parentWid):QtBaseTitleMoveWidget(parentWid)
, m_openImg(openImg)
{
	setTitleText(QString::fromWCharArray(L"图片"));
	setFixedSize(600,400);
	initUI();
	initConnect();
	auto fme = openImg->makeFrame();
	if (fme)
		m_glwidget->renderFrame(std::move(fme));
}

QtOpenImgDlg::~QtOpenImgDlg() 
{
	m_openImg->setCallBack(nullptr);
}

void QtOpenImgDlg::slotBtnClicked(bool) 
{
	//std::string path = GetExePath();
	//QString filePath = QFileDialog::getOpenFileName(nullptr, QObject::tr("Open File"),
	//	QDir::homePath(), QObject::tr("All Files (*)"));
	//tr("Image Files (*.png *.jpg *.bmp)")
	//QString fileName = QFileDialog::getOpenFileName(nullptr,
	//	tr("Open Image"), ".", tr("All Files (*)"));
	//QString fileName = QFileDialog::getOpenFileName(NULL, "open image", "C:/", tr("Image Files (*.png *.jpg *.bmp)"));
	/*QString fileName = QFileDialog::getOpenFileName(NULL, tr("Open Image"),
		".",
		tr("Images (*.png *.xpm *.jpg)"));*/

	//QString fileName(R"(G:\360MoveData\Users\Administrator\Desktop\abc.gif)");

	QFileDialog dialog(NULL);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter("Images (*.png *.gif *.jpg *.bmp)");
	dialog.setViewMode(QFileDialog::Detail);

	if (dialog.exec()) {
		QStringList selectedFiles = dialog.selectedFiles();
		for(auto& fileName : selectedFiles)
			if (!fileName.isEmpty())
			{
				m_edit->setText(fileName);
				m_openImg->open(fileName.toStdString());
				break;
			}
	}

	
}

void QtOpenImgDlg::slotValueChanged(int v) 
{
	m_openImg->setScaleFactor((float)v/1000);
}

void QtOpenImgDlg::initUI()
{
	QWidget *wid = getBodyWidget();
	m_glwidget = new QtGLVideoWidget(wid);
	m_glwidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QLabel* label = new QLabel(QString::fromWCharArray(L"图像文件"), wid);
	label->setScaledContents(true);
	m_edit = new QLineEdit(wid);
	m_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_edit->setText(QString::fromStdString(m_openImg->m_file));
	m_btn = new QPushButton(QString::fromWCharArray(L"浏览"), wid);
	QHBoxLayout* btnLay = new QHBoxLayout;
	btnLay->setAlignment(Qt::AlignVCenter);
	btnLay->addWidget(label);
	btnLay->addWidget(m_edit);
	btnLay->addWidget(m_btn);


	QLabel* sliderLabel = new QLabel(QString::fromWCharArray(L"缩放"), wid);
	sliderLabel->setScaledContents(true);
	m_slider = new QSlider(Qt::Orientation::Horizontal, wid);
	m_slider->setRange(0,1000);
	m_slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_slider->setFixedHeight(22);
	QHBoxLayout* silderLay = new QHBoxLayout;
	silderLay->addWidget(sliderLabel);
	silderLay->addWidget(m_slider);

	
	QVBoxLayout* mainLay = new QVBoxLayout;
	mainLay->addWidget(m_glwidget);
	mainLay->addLayout(btnLay);
	mainLay->addLayout(silderLay);
	wid->setLayout(mainLay);

	int value = m_openImg->m_fScale * 1000;
	if (value <= 0)
		value = 0;
	else if (value > 1000)
		value = 1000;
	m_slider->setValue(value);

	m_openImg->setCallBack([this](UPTR_FME f) 
		{
			m_glwidget->renderFrame(std::move(f));
		});
}


void QtOpenImgDlg::initConnect() 
{
	QObject::connect(m_btn, SIGNAL(clicked(bool)), this,SLOT(slotBtnClicked(bool)));
	QObject::connect(m_slider, SIGNAL(valueChanged(int)), this,SLOT(slotValueChanged(int)));
}
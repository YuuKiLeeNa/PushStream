#include "QtAddSourceWidget.h"
#include<QPushButton>
#include<QBoxLayout>
#include<QMenu>
#include<vector>
#include "QtTextDlg.h"
#include"QtResizeDrawFrameBuffer.h"
#include"SourceType/CSourceType.h"
#include"SourceType/GetSourceType.h"
#include "QtAudioSelDlg.h"
#include "AMixWidget.h"
#include "QtAudioFilterDlg.h"
#include "VideoCapture/GLDrawCamera.h"
#include "PushStream/SelCamWidget.h"
#include "PushStream/QtOpenImgDlg.h"


QtAddSourceWidget::QtAddSourceWidget(QtResizeDrawFrameBuffer* drawWidget, QWidget* parentWidget):m_desktop(drawWidget)
{
	setMouseTracking(true);
	initUI();
	initConnect();
}

void QtAddSourceWidget::setAMixWidget(AMixWidget* amixwidget)
{
	m_amixWidget = amixwidget;
	if (m_amixWidget) {
		QtAudioSelDlg dlg_input_audio(false);
		QtAudioSelDlgAdd(&dlg_input_audio);

		QtAudioSelDlg dlg_output_audio(true);
		QtAudioSelDlgAdd(&dlg_output_audio);
	}
}

void QtAddSourceWidget::setDrawWidget(QtResizeDrawFrameBuffer* drawWidget) {
	m_desktop = drawWidget;
}

void QtAddSourceWidget::slotAddBtnClick()
{
	QPoint cursorPos = QCursor::pos();
	m_menu->exec(cursorPos);
}


void QtAddSourceWidget::slotAddText(bool)
{
	/*QPushButton* btn = new QPushButton(QString::fromWCharArray(L"文本"), m_scroll->m_scrollArea->widget());
	btn->setFixedHeight(28);
	btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	btn->setStyleSheet("QPushButton{color:white;}");*/

	//m_scroll->m_scrollArea->widget()->layout()->addWidget(btn);
	//m_scrollLay->addWidget(btn);
	//m_scroll->m_scrollArea->update();
	QtTextDlg dlg;
	if (dlg.exec() == QDialog::Accepted) 
	{
		auto ptr = dlg.getGenTextImage();
		m_desktop->pushDrawImageComponent(ptr);
		createBtnToLay(ptr, QString::fromWCharArray(L"文本"));


		//QPushButton* btn = new QPushButton(QString::fromWCharArray(L"文本"), m_scroll->m_scrollArea->widget());
		//btn->setFixedHeight(28);
		//btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		//btn->setStyleSheet("QPushButton{color:white;}");
		//btn->setMouseTracking(true);
		//btn->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
		////btn->customContextMenuRequested(QPoint pos)

		//m_mapBtnToAddObj[btn] = ptr;
		////btn->setProperty("ptr", ptr);
		//m_scrollLay->addWidget(btn);
		//QObject::connect(btn, SIGNAL(clicked()), this, SLOT(slotScrollBtnClicked()));
		//QObject::connect(btn, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotBtnCustomContextMenuRequested(const QPoint & )));
	}
}

void QtAddSourceWidget::slotScrollBtnClicked()
{
	QPushButton*btn = (QPushButton*)QObject::sender();
	auto iter = m_mapBtnToAddObj.find(btn);
	if (iter == m_mapBtnToAddObj.end())
		return;
	auto t = iter->second->type();

	//ST_FREE_TYPE_TEXT, ST_PIC, ST_AUDIO, ST_MEDIA, ST_CAMERA
	switch (t)
	{
	case CSourceType::ST_FREE_TYPE_TEXT:
	{
		std::shared_ptr<GetSourceType_t<CSourceType::ST_FREE_TYPE_TEXT>>ptr = std::static_pointer_cast<GetSourceType_t<CSourceType::ST_FREE_TYPE_TEXT>>(iter->second);
		QtTextDlg dlg(ptr);
		dlg.exec();
	}
		break;
	case CSourceType::ST_PIC:
	{
		std::shared_ptr<GetSourceType_t<CSourceType::ST_PIC>>ptr = std::static_pointer_cast<GetSourceType_t<CSourceType::ST_PIC>>(iter->second);
		QtOpenImgDlg dlg(ptr);
		dlg.exec();
	}
		break;
	case CSourceType::ST_AUDIO:
		break;
	case CSourceType::ST_MEDIA:
		break;
	case CSourceType::ST_CAMERA:
	{
		std::shared_ptr<GetSourceType_t<CSourceType::ST_CAMERA>>ptr = std::static_pointer_cast<GetSourceType_t<CSourceType::ST_CAMERA>>(iter->second);
		std::shared_ptr<SelCamWidget>dlg = std::make_shared<SelCamWidget>(ptr);
		dlg->selBoxChanged(-1);
		dlg->exec();
		ptr->clearVCaptureFrameCallBack();
		//ptr->setCameraCallGenTex();
	}
		break;
	default:
		break;
	}
	
}

void QtAddSourceWidget::slotBtnCustomContextMenuRequested(const QPoint& pt)
{
	QPushButton* btn = (QPushButton*)QObject::sender();
	auto iter = m_mapBtnToAddObj.find(btn);
	if (iter == m_mapBtnToAddObj.end())
		return;
	auto t = iter->second->type();


	//ST_FREE_TYPE_TEXT, ST_PIC, ST_AUDIO, ST_MEDIA, ST_CAMERA
	QMenu menu;
	QAction* act_del = menu.addAction(QString::fromWCharArray(L"删除"));
	
	if (t != CSourceType::ST_AUDIO)
	{
		QObject::connect(act_del, &QAction::triggered, [iter, this, btn](bool b)
			{
				iter->second->setRemoved(true);
				m_mapBtnToAddObj.erase(btn);
				m_scrollLay->removeWidget(btn);
				delete btn;
			});
	}


	switch (t)
	{
	case CSourceType::ST_FREE_TYPE_TEXT: 
		menu.exec(btn->mapToGlobal(pt));
		break;
	case CSourceType::ST_PIC:
		menu.exec(btn->mapToGlobal(pt));
		break;
	case CSourceType::ST_AUDIO: 
		{
			QObject::connect(act_del, &QAction::triggered, [iter, this, btn](bool b)
			{
				iter->second->setRemoved(true);
				m_scrollLay->removeWidget(btn);
				PSSource ptr = std::static_pointer_cast<GetSourceType_t<CSourceType::ST_AUDIO>>(iter->second);
				m_amixWidget->removeAudioDevice(ptr);
				m_mapBtnToAddObj.erase(btn);
				delete btn;
			});

			QAction* act_filter = menu.addAction(QString::fromWCharArray(L"滤镜"));
			QObject::connect(act_filter, &QAction::triggered, [act_filter, this, btn](bool b)
				{
					auto iter = m_mapBtnToAddObj.find(btn);
					if (iter != m_mapBtnToAddObj.end()) 
					{
						std::shared_ptr<CSource> ptr = std::static_pointer_cast<CSource>(iter->second);
						QtAudioFilterDlg dlg(ptr);
						dlg.exec();
					}
				});
			menu.exec(btn->mapToGlobal(pt));
		}
		break;
	case CSourceType::ST_MEDIA:
		menu.exec(btn->mapToGlobal(pt));
		break;
	case CSourceType::ST_CAMERA:
		menu.exec(btn->mapToGlobal(pt));
		break;
	default:
		break;
	}
}

void QtAddSourceWidget::slotAudioInputActionClicked(bool) 
{
	QtAudioSelDlg dlg(true);
	if (dlg.exec() == QDialog::Accepted) 
		QtAudioSelDlgAdd(&dlg);
}

void QtAddSourceWidget::slotAudioOutputActionClicked(bool) 
{
	QtAudioSelDlg dlg(false);
	if (dlg.exec() == QDialog::Accepted)
		QtAudioSelDlgAdd(&dlg);
}

void QtAddSourceWidget::slotCameraActionClicked(bool)
{
	std::shared_ptr<SelCamWidget>dlg = std::make_shared<SelCamWidget>();
	dlg->selBoxChanged(-1);
	auto ptr = dlg->getCamera();
	ptr->setCameraCallGenTex();
	m_desktop->pushDrawImageComponent(ptr);
	createBtnToLay(ptr, QString::fromWCharArray(L"相机"));

	if (dlg->exec() == QDialog::Accepted)
	{
		//auto ptr = dlg->getCamera();
		/*std::weak_ptr<decltype(ptr)::element_type>wptr(ptr);
		std::function<void(AVFrame* f)>fun = [wptr](AVFrame* f)
			{
				auto ptr = wptr.lock();
				if(ptr)
					ptr->setData(UPTR_FME(f));
			};
		ptr->setCaptureFrameCallBack(fun);*/
		ptr->clearVCaptureFrameCallBack();
		/*ptr->setCameraCallGenTex();
		m_desktop->pushDrawImageComponent(ptr);
		createBtnToLay(ptr, QString::fromWCharArray(L"相机"));*/
	}
}

void QtAddSourceWidget::slotPicActionClicked(bool) 
{
	QtOpenImgDlg dlg;
	auto ptr = dlg.getOpenImg();
	m_desktop->pushDrawImageComponent(ptr);
	createBtnToLay(ptr, QString::fromWCharArray(L"图片"));
	if (dlg.exec() == QDialog::Accepted) 
	{
		
	}
}

void QtAddSourceWidget::QtAudioSelDlgAdd(QtAudioSelDlg*dlg)
{
	auto audioinfo = dlg->getSeledItemInfo();
	if (audioinfo.id.empty())
		return;
	std::string name = dlg->name();
	PSSource src = m_amixWidget->addAudioDevice(name, audioinfo.id, obs_peak_meter_type::SAMPLE_PEAK_METER, dlg->isInputDevice() ? CWasapi<CSource>::SourceType::Input : CWasapi<CSource>::SourceType::DeviceOutput);
	createBtnToLay(src, QString::fromStdString(name));
}


void QtAddSourceWidget::initUI()
{
	QWidget* wid = new QWidget(this);
	wid->setStyleSheet("QWidget{background-color:black;}");
	m_scroll = new QtScrollWidget(wid);
	m_scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_addBtn = new QPushButton(this);
	m_addBtn->setFixedSize(26,26);
	m_addBtn->setStyleSheet("QPushButton{border-image:url(:/PushStream/icon/add_normal.png);border:none}"
		"QPushButton:hover{border-image:url(:/PushStream/icon/add_hover.png);border:1 px solid blue;}"
		"QPushButton:pressed{border-image:url(:/PushStream/icon/add_pressed.png);border:1 px solid blue;}");
	m_removeBtn = new QPushButton(wid);
	m_removeBtn->setFixedSize(26,26);
	m_removeBtn->setStyleSheet("QPushButton{border-image:url(:/PushStream/icon/subtrack_normal.png);border:none}"
		"QPushButton:hover{border-image:url(:/PushStream/icon/subtrack_hover.png);border:1 px solid blue;}"
		"QPushButton:pressed{border-image:url(:/PushStream/icon/subtrack_pressed.png);border:1 px solid blue;}");



	QHBoxLayout* btnLay = new QHBoxLayout;
	btnLay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	btnLay->addWidget(m_addBtn);
	btnLay->addWidget(m_removeBtn);


	QVBoxLayout* widLay = new QVBoxLayout;
	widLay->setContentsMargins(0,0,0,0);
	widLay->setSpacing(0);
	widLay->addWidget(m_scroll);
	widLay->addLayout(btnLay);
	wid->setLayout(widLay);


	QVBoxLayout* mainLay = new QVBoxLayout;
	mainLay->setContentsMargins(0,0,0,0);
	mainLay->setSpacing(0);
	mainLay->addWidget(wid);
	setLayout(mainLay);

	m_menu = new QMenu(this);

	std::vector<std::pair<QString, void(QtAddSourceWidget::*)(bool)>> actionVec{ {QString::fromWCharArray(L"文本"), &QtAddSourceWidget::slotAddText}
		,{QString::fromWCharArray(L"音频输入采集"), &QtAddSourceWidget::slotAudioInputActionClicked}
		,{QString::fromWCharArray(L"音频输出采集"), &QtAddSourceWidget::slotAudioOutputActionClicked}
		, {QString::fromWCharArray(L"相机"), &QtAddSourceWidget::slotCameraActionClicked}
		, {QString::fromWCharArray(L"图像"), &QtAddSourceWidget::slotPicActionClicked}
		,{QString::fromWCharArray(L"媒体文件"), NULL} };

	m_actVec.reserve(actionVec.size());

	for (const auto& ele : actionVec)
	{
		QAction* act = new QAction(ele.first,m_menu);
		if (ele.second)
		{
			QMetaObject::Connection c = QObject::connect(act, &QAction::triggered, this, ele.second);
		}
		/*if (ele.first == actionVec[0].first) 
		{
			QObject::connect(act, SIGNAL(triggered(bool)), this, SLOT(slotAddText(bool)));
		}*/
		m_menu->addAction(act);
		m_actVec.push_back(act);
	}


	

	m_menu->setWindowFlags(m_menu->windowFlags() | Qt::FramelessWindowHint);
	m_menu->setAttribute(Qt::WA_TranslucentBackground);
	m_menu->setMouseTracking(true);
	m_menu->setStyleSheet(R"(QMenu {
    background-color: transparent;
    border: 1px solid rgba(82, 130, 164, 1);
}

QMenu::item {
    font-size: 18px;
    color: rgb(225, 225, 225);
    border: none;
}

QMenu::item:selected  {
    border:1px solid #1C86EE;
	color:#1C86EE;
}

QMenu::item:pressed {
    border:1px solid #00B2EE;
	color:#00B2EE;
}
)");

	QWidget* scroll_wid = new QWidget(m_scroll->m_scrollArea);
	scroll_wid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_scrollLay = new QVBoxLayout;
	m_scrollLay->setContentsMargins(0,0,0,0);
	m_scrollLay->setSpacing(0);
	m_scrollLay->setAlignment(Qt::AlignTop);

	/*QPushButton* btn = new QPushButton("aaa", scroll_wid);
	btn->setStyleSheet("QPushButton{color:white;}");
	m_scrollLay->addWidget(btn);*/

	scroll_wid->setLayout(m_scrollLay);
	m_scroll->setWidget(scroll_wid);

}

void QtAddSourceWidget::initConnect()
{
	QObject::connect(m_addBtn, SIGNAL(clicked()), this, SLOT(slotAddBtnClick()));

}

void QtAddSourceWidget::createBtnToLay(std::shared_ptr<CSourceType> source, const QString& btnName)
{
	QPushButton* btn = new QPushButton(btnName, m_scroll->m_scrollArea->widget());
	btn->setFixedHeight(28);
	btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	btn->setStyleSheet("QPushButton{color:white;}");
	btn->setMouseTracking(true);
	btn->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
	//btn->customContextMenuRequested(QPoint pos)

	m_mapBtnToAddObj[btn] = source;
	//btn->setProperty("ptr", ptr);
	m_scrollLay->addWidget(btn);
	QObject::connect(btn, SIGNAL(clicked()), this, SLOT(slotScrollBtnClicked()));
	QObject::connect(btn, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotBtnCustomContextMenuRequested(const QPoint&)));
}

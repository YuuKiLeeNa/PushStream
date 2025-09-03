#include "QtSettingDlg.h"
#include<QBoxLayout>
#include<QLineEdit>
#include<QComboBox>
#include<QCheckBox>
#include<QSpinBox>
#include<QFileDialog>
#include"Audio/utilfun.h"
#include "encoder/utilEncoder.h"
#include<algorithm>
#include<numeric>
#include "Util/logger.h"


template<typename...ARGS>
struct SAVE_ARGS
{
};

template<typename...ARGS>
struct memberFunNameTraits 
{
};

template<typename Ret,typename CLASS, typename...ARGS>
struct memberFunNameTraits<Ret(CLASS::*)(ARGS...)>
{
	using Ret_type = typename Ret;
	using Class_type = typename CLASS;
	using Args_type = SAVE_ARGS<ARGS...>;
};


QtSettingDlg::QtSettingDlg(const QString& url, const CMediaOption& opt, QWidget* parentWid):QtBaseTitleMoveWidget(parentWid)
,m_recordOpt(opt)
{
	setStyleSheet("QWidget{background-color:gray;}"
	"QPushButton{color:white;}"
	"QLineEdit{color:white;}"
	"QLineEdit:disable{color:white;}");
	setTitleText(QString::fromWCharArray(L"设置"));
	initUI(url);
	initData();
}

QString QtSettingDlg::getPushUrl() 
{
	return m_editAccount->text();
}

void QtSettingDlg::initUI(const QString& url)
{
	setFixedSize(400,600);
	QWidget* wid = getBodyWidget();
	m_scrollArea = new QScrollArea(wid);
	m_scrollArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	m_scrollArea->setFixedWidth(120);
	m_scrollArea->setWidgetResizable(true);

	QWidget* scrollWidget = new QWidget(m_scrollArea);
	m_scrollArea->setWidget(scrollWidget);
	m_stack = new QStackedWidget(wid);
	m_stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QHBoxLayout* widmain = new QHBoxLayout;
	widmain->setContentsMargins(0, 0, 0, 0);
	widmain->addWidget(m_scrollArea);
	widmain->addWidget(m_stack);
	wid->setLayout(widmain);

	QVBoxLayout* scrollWidgetLay = new QVBoxLayout;
	scrollWidgetLay->setContentsMargins(0, 0, 0, 0);
	scrollWidgetLay->setAlignment(Qt::AlignTop);
	scrollWidgetLay->setSpacing(0);
	scrollWidget->setLayout(scrollWidgetLay);
	
	for (int i = 0; i < m_vInitInfo.size(); ++i) 
	{
		getBtn_vInitInfo(i) = new QPushButton(getName_vInitInfo(i), scrollWidget);
		scrollWidgetLay->addWidget(getBtn_vInitInfo(i));
		QObject::connect(getBtn_vInitInfo(i), getSignalFunMemberPointer_vInitInfo(i), this, getSlotFunMemberPointer_vInitInfo(i));
	}

	initWidPushStream(url);
	m_stack->addWidget(m_widPushStream);

	initWidRecord();
	m_stack->addWidget(m_widRecord);

}

void QtSettingDlg::initData()
{
	std::string strSaveDir = m_recordOpt.av_opt_get("saveDir", GetExePath());
	m_editRecordPath->setText(QString::fromUtf8(strSaveDir.c_str()));
	m_editRecordPath->setToolTip(m_editRecordPath->text());
	QObject::connect(m_btnSelPath, &QPushButton::clicked, this, &QtSettingDlg::slotBtnSelPathClicked);


	auto initComboxStreamFun = [this](QComboBox*pCombox
		, std::map<AVCodecID, std::string>(*getCodecIdFun)()
		, const std::string&codecNameInOpt
		, void (QtSettingDlg::*pslot)(int))
		{
			auto codecSets = getCodecIdFun();
			//codecSets.insert({AV_CODEC_ID_NONE, std::string("auto")});
			pCombox->clear();
			for (const auto& ele : codecSets)
				pCombox->addItem(QString::fromStdString(ele.second), QVariant(ele.first));
			
			AVCodecID videoId = m_recordOpt.av_opt_get_codec_id(codecNameInOpt, AV_CODEC_ID_HEVC);
			
			auto iter = std::find_if(codecSets.cbegin(), codecSets.cend(), [videoId](const decltype(codecSets)::value_type&v)
				{
					return v.first == videoId;
				});
			if (iter == codecSets.cend())
			{
				PrintW("cound not find %x in actions", videoId);
				pCombox->setCurrentIndex(0);
			}
			else
				pCombox->setCurrentIndex(std::distance(codecSets.cbegin(), iter));
			(this->*pslot)(pCombox->currentIndex());
			QObject::connect(pCombox, &QComboBox::currentIndexChanged, this, pslot);
		};

	initComboxStreamFun(m_comboxRecordVideo_codecid, &getVideoCodecId, "videoCodec", &QtSettingDlg::slotComboxVideoCodecIdChanged);
	initComboxStreamFun(m_comboxRecordAudio_codecid, &getAudioCodecId, "audioCodec", &QtSettingDlg::slotComboxAudioCodecIdChanged);
	bool isPriorityHardwareAccel = m_recordOpt.av_opt_get_int("priorityHardwareAccel", 1);
	m_checkRecordBoxEnableHardwareAccelator->setChecked(isPriorityHardwareAccel);
	m_comboxRecordVideoMode->addItems(m_strListVideoEncoderMode);

	initPresetProfileTune(m_comBoxRecordPrioritySelVideoCodec->currentIndex());
	QObject::connect(m_comBoxRecordPrioritySelVideoCodec, &QComboBox::currentIndexChanged, this, &QtSettingDlg::initPresetProfileTune);





}

void QtSettingDlg::SlotBtnPushStreamClicked(bool)
{
	m_stack->setCurrentIndex(0);
	//m_stack->setCurrentWidget(m_widPushStream);
}

void QtSettingDlg::SlotBtnRecordClicked(bool) 
{
	m_stack->setCurrentIndex(1);
	//m_stack->setCurrentWidget(m_widRecord);
}

void QtSettingDlg::initWidPushStream(const QString& url)
{
	if (m_widPushStream)
		return;
	m_widPushStream = new QWidget(m_stack);
	QVBoxLayout* layPushStreamWid = new QVBoxLayout;
	layPushStreamWid->setContentsMargins(0, 0, 0, 0);
	layPushStreamWid->setAlignment(Qt::AlignTop);
	//layPushStreamWid->addLayout(fun(QString::fromWCharArray(L"推流url"), m_editAccount));
	layPushStreamWid->addLayout(create_label_control(QString::fromWCharArray(L"推流url"), m_editAccount, m_widPushStream));
	m_editAccount->setText(url);
	//layPushStreamWid->addLayout(fun(QString::fromWCharArray(L"串流密钥"), m_editPassword));
	m_widPushStream->setLayout(layPushStreamWid);




}

void QtSettingDlg::initWidRecord()
{
	if (m_widRecord)
		return;
	m_widRecord = new QWidget(m_stack);
	m_stack->addWidget(m_widRecord);
	
	QHBoxLayout* savePathLay = new QHBoxLayout;
	savePathLay->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	QLabel* labelEditRecordPath = new QLabel(QString::fromWCharArray(L"保存路径"), m_widRecord);
	labelEditRecordPath->setScaledContents(true);
	m_editRecordPath = new QLineEdit(m_widRecord);
	m_editRecordPath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_editRecordPath->setEnabled(false);
	
	m_btnSelPath = new QPushButton(QString::fromWCharArray(L"选择路径"), m_widRecord);
	savePathLay->addWidget(labelEditRecordPath);
	savePathLay->addWidget(m_editRecordPath);
	savePathLay->addWidget(m_btnSelPath);

	m_tabRecord = new QTabWidget(m_widRecord);
	//m_tabRecord->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QVBoxLayout* laywidRecord = new QVBoxLayout(m_widRecord);
	laywidRecord->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	laywidRecord->addLayout(savePathLay);
	laywidRecord->addWidget(m_tabRecord);
	m_widRecord->setLayout(laywidRecord);

	m_tabRecord->addTab(initVideoPage(m_tabRecord), QString::fromWCharArray(L"视频编码"));
	m_tabRecord->addTab(initAudioPage(m_tabRecord), QString::fromWCharArray(L"音频编码"));

}

QWidget* QtSettingDlg::initVideoPage(QWidget* parent)
{
	QWidget* videoEncoder = new QWidget(parent);
	QHBoxLayout* layVideoStram = create_label_control(QString::fromWCharArray(L"视频流"), m_comboxRecordVideo_codecid, videoEncoder);
	m_checkRecordBoxEnableHardwareAccelator = new QCheckBox(QString::fromWCharArray(L"优先硬件加速"), videoEncoder);
	QHBoxLayout* laySelVideoCodec = create_label_control(QString::fromWCharArray(L"优先选择视频编码器"), m_comBoxRecordPrioritySelVideoCodec, videoEncoder);
	QHBoxLayout* layRecordVideoMode = create_label_control(QString::fromWCharArray(L"视频编码模式"), m_comboxRecordVideoMode, videoEncoder);


	m_layMin = create_label_control(QString::fromWCharArray(L"最小比特率(KB)"), m_spinVideoMinBitrate, videoEncoder);
	m_layMax = create_label_control(QString::fromWCharArray(L"最大比特率(KB)"), m_spinVideoMaxBitrate, videoEncoder);
	m_layBuffer = create_label_control(QString::fromWCharArray(L"缓冲区大小(KB)"), m_spinVideoBuffSize, videoEncoder);
	m_layCQP = create_label_control(QString::fromWCharArray(L"CQP"), m_spinCQP, videoEncoder);
	m_layCRF = create_label_control(QString::fromWCharArray(L"CRF"), m_spinCrf, videoEncoder);


	m_layProfile = create_label_control(QString::fromWCharArray(L"profile"), m_comboxProfile, videoEncoder);
	m_layTune = create_label_control(QString::fromWCharArray(L"tune"), m_comboxTune, videoEncoder);
	m_layPreset = create_label_control(QString::fromWCharArray(L"preset"), m_comboxPreset, videoEncoder);

	QVBoxLayout* layVideoEncoder = new QVBoxLayout(videoEncoder);
	layVideoEncoder->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	layVideoEncoder->addLayout(layVideoStram);
	layVideoEncoder->addWidget(m_checkRecordBoxEnableHardwareAccelator);
	layVideoEncoder->addLayout(laySelVideoCodec);
	layVideoEncoder->addLayout(layRecordVideoMode);
	layVideoEncoder->addLayout(m_layMin);
	layVideoEncoder->addLayout(m_layMax);
	layVideoEncoder->addLayout(m_layBuffer);
	layVideoEncoder->addLayout(m_layCQP);
	layVideoEncoder->addLayout(m_layCRF);
	layVideoEncoder->addLayout(m_layProfile);
	layVideoEncoder->addLayout(m_layTune);
	layVideoEncoder->addLayout(m_layPreset);
	videoEncoder->setLayout(layVideoEncoder);
	return videoEncoder;
}

QWidget* QtSettingDlg::initAudioPage(QWidget* parent)
{
	QWidget* widAudioPage = new QWidget(parent);
	QVBoxLayout* layWidAudioPage = new QVBoxLayout(widAudioPage);
	layWidAudioPage->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	layWidAudioPage->addLayout(create_label_control(QString::fromWCharArray(L"音频流"), m_comboxRecordAudio_codecid, widAudioPage));
	layWidAudioPage->addLayout(create_label_control(QString::fromWCharArray(L"音频编码器"), m_comBoxRecordPrioritySelAudioCodec, widAudioPage));
	layWidAudioPage->addLayout(create_label_control(QString::fromWCharArray(L"音频比特率(KB)"), m_spinAudioBitrate, widAudioPage));
	widAudioPage->setLayout(layWidAudioPage);
	return widAudioPage;
}

void QtSettingDlg::slotBtnSelPathClicked()
{
	QFileDialog dialog(NULL);
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setViewMode(QFileDialog::Detail);

	//std::string strSaveDir = m_recordOpt.av_opt_get("saveDir", GetExePath());
	//dialog.setDirectory(QString::fromUtf8(strSaveDir.c_str()));
	dialog.setDirectory(m_editRecordPath->text());
	if (dialog.exec()) {
		QStringList selectedDirs = dialog.selectedFiles();
		// 处理选中的文件
		for (auto& dir : selectedDirs)
			if (!dir.isEmpty())
			{
				m_editRecordPath->setText(dir);
				m_editRecordPath->setToolTip(m_editRecordPath->text());
				break;
			}
	}
}

void QtSettingDlg::slotComboxVideoCodecIdChanged(int index)
{
	comboxCodecIdChanged(index, m_comboxRecordVideo_codecid, m_comBoxRecordPrioritySelVideoCodec, m_vecVideoCodec);
}

void QtSettingDlg::slotComboxAudioCodecIdChanged(int index)
{
	comboxCodecIdChanged(index, m_comboxRecordAudio_codecid, m_comBoxRecordPrioritySelAudioCodec, m_vecAudioCodec);
}

void QtSettingDlg::comboxCodecIdChanged(int index, QComboBox* comboxCodecId, QComboBox* comboxCodec, std::vector<const AVCodec*>& saveCodecs)
{
	comboxCodec->clear();
	if (index < 0)
		return;
	AVCodecID id = comboxCodecId->itemData(index).value<AVCodecID>();
	std::vector<AVCodecID>sets;
	/*if (id == AV_CODEC_ID_NONE)
	{
		int iCount = comboxCodecId->count();
		sets.reserve(iCount - 1);
		for (int i = 1; i < iCount; ++i)
			sets.push_back(comboxCodecId->itemData(i).value<AVCodecID>());
	}
	else*/
	sets.emplace_back(id);

	saveCodecs = getEncoderCodec(sets);
	comboxCodec->addItem("auto");
	for (auto& ele : saveCodecs)
		comboxCodec->addItem(QString::fromLocal8Bit(ele->long_name));



}

void QtSettingDlg::initPresetProfileTune(int codecSelIndex)
{
	m_comboxPreset->clear();
	m_comboxProfile->clear();
	m_comboxTune->clear();

	if (codecSelIndex < 0 || codecSelIndex > m_vecVideoCodec.size())
		return;
	if (codecSelIndex == 0)
	{
		m_comboxPreset->addItem("default");
		m_comboxProfile->addItem("default");
		m_comboxTune->addItem("default");
		return;
	}
	const AVCodec* p = m_vecVideoCodec[codecSelIndex - 1];
	const AVOption* opt = p->priv_class->option;

	do
	{
		if (!(opt->flags & AV_OPT_FLAG_FILTERING_PARAM) && opt->unit != NULL
			&& strcmp(opt->name, opt->unit) != 0)
		{
			QComboBox* pDst = NULL;
			if (strcmp(opt->unit, "preset") == 0)
				pDst = m_comboxPreset;
			else if (strcmp(opt->unit, "profile") == 0)
				pDst = m_comboxProfile;
			else if (strcmp(opt->unit, "tune") == 0)
				pDst = m_comboxTune;
			if (pDst)
				pDst->addItem(opt->name);
		}
	}while(opt=av_opt_next(p->priv_class, opt));

}


//
//void QtSettingDlg::initProfile(int codecSelIndex) 
//{
//
//}
//
//void QtSettingDlg::initTune(int codecSelIndex) 
//{
//	
//}
//

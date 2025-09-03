#include "ButtonSets.h"
#include<qboxlayout.h>

ButtonSets::ButtonSets(QWidget* parentWidget) :QWidget(parentWidget) 
{
	initUI();
	initConnect();
}

QPushButton* ButtonSets::getBtn(int id)
{
	return this->*std::get<1>(initbtn[id]);
}

void ButtonSets::updateBtnText()
{
	for (auto& ele : btnTextState)
		(this->*std::get<2>(ele))->setText((this->*std::get<2>(ele))->isChecked() ? std::get<1>(ele): std::get<0>(ele));
}

void ButtonSets::initUI()
{
	setStyleSheet("QWidget{background-color:black;}"
		"QPushButton{background-color:gray;color:#ffffff;}"
	);
	m_group = new QButtonGroup(this);
	QWidget* wid = new QWidget(this);
	wid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QVBoxLayout* lay1 = new QVBoxLayout;
	lay1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	for (auto& ele : initbtn)
	{
		QPushButton*btn = new QPushButton(std::get<0>(ele), wid);
		btn->setFixedSize(100,22);
		btn->setCheckable(std::get<3>(ele));
		if(std::get<4>(ele))
			m_group->addButton(btn, std::get<2>(ele));
		lay1->addWidget(btn);
		this->*std::get<1>(ele) = btn;
	}
	wid->setLayout(lay1);

	QVBoxLayout* vlay = new QVBoxLayout;
	vlay->setContentsMargins(0,0,0,0);
	vlay->addWidget(wid);

	setLayout(vlay);

	//m_scrollArea->setWidget(wid);
	//m_btnPush->setCheckable(true);
	//m_btnRecord->setCheckable(true);
	
}

void ButtonSets::initConnect() 
{
	QObject::connect(m_group,SIGNAL(idClicked(int)),this, SIGNAL(signalsBtnClicked(int)));
	QObject::connect(m_btnPush, SIGNAL(clicked(bool)), this, SIGNAL(signalsPushBtnClicked(bool)));
	QObject::connect(m_btnRecord, SIGNAL(clicked(bool)), this, SIGNAL(signalsRecordBtnClicked(bool)));
	QObject::connect(m_btnSetting, SIGNAL(clicked(bool)), this, SIGNAL(signalsBtnSettingClicked(bool)));
}


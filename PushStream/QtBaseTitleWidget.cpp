#include "QtBaseTitleWidget.h"
#include<qboxlayout.h>
#include<QMouseEvent>
#define TITLE_HEIGHT 28


QtBaseTitleWidget::QtBaseTitleWidget(QWidget* parent,bool isTitleShow, bool isTransparent) :QDialog(parent)
{
	//setWindowOpacity(0.52);
	setWindowFlags(Qt::FramelessWindowHint |Qt::ToolTip);
	//if (isTransparent)
	//	setAttribute(Qt::WA_TranslucentBackground, true);

	if (isTitleShow)
	{
		m_titleWidget = new QFrame(this);
		m_titleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_titleWidget->setFixedHeight(TITLE_HEIGHT);
		m_titleWidget->setObjectName("m_titleWidget");
		//m_titleWidget->setStyleSheet("QFrame#m_titleWidget{background-color:#28B4FF;border-left:1px solid #347CD5;border-right:1px solid #347CD5;border-top:1px solid #347CD5;}");


		m_iconLabel = new QLabel(m_titleWidget);
		m_iconLabel->setFixedSize(TITLE_HEIGHT, TITLE_HEIGHT);

		m_titleTextLabel = new QLabel(m_titleWidget);
		m_titleTextLabel->setScaledContents(true);
		m_titleTextLabel->setFixedHeight(TITLE_HEIGHT);
		m_titleTextLabel->setStyleSheet("QLabel{font-size:17px;color:black;font:bold;}");

		m_closeButton = new QPushButton(m_titleWidget);
		m_closeButton->setFixedSize(TITLE_HEIGHT, TITLE_HEIGHT);
		m_closeButton->setObjectName("m_closeButton");
		m_closeButton->setStyleSheet("QPushButton#m_closeButton{image:url(:/QtBaseTitleWidget/image/showDialog/QtBaseTitleWidget/close_normal.png);background-color:transparent;border:none;}"
			"QPushButton#m_closeButton:hover{image:url(:/QtBaseTitleWidget/image/showDialog/QtBaseTitleWidget/close_hover.png);}"
			"QPushButton#m_closeButton:pressed{image:url(:/QtBaseTitleWidget/image/showDialog/QtBaseTitleWidget/close_pressed.png);}");

		connect(m_closeButton, SIGNAL(clicked()), this, SLOT(slotCloseBtnClick()));

		QHBoxLayout* titleLayout = new QHBoxLayout;
		titleLayout->setContentsMargins(5, 0, 0, 0);
		titleLayout->setSpacing(2);
		titleLayout->addWidget(m_iconLabel);
		titleLayout->addWidget(m_titleTextLabel);
		titleLayout->addStretch();
		titleLayout->addWidget(m_closeButton);
		m_titleWidget->setLayout(titleLayout);
	}
	m_bodyWidget = new QFrame(this);
	m_bodyWidget->setObjectName("m_bodyWidget");
	//m_bodyWidget->setStyleSheet("QFrame#m_bodyWidget{background-color:#28B4FF;border-left:1px solid #347CD5;border-right:1px solid #347CD5;border-bottom:1px solid #347CD5;}");
	m_bodyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0,0,0,0);
	mainLayout->setSpacing(0);
	if(m_titleWidget)
		mainLayout->addWidget(m_titleWidget);
	mainLayout->addWidget(m_bodyWidget);

	m_bottomWidget = new QFrame(this);
	m_bottomWidget->setObjectName("m_bottomWidget");
	m_bottomWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	m_ok = new QPushButton(QString::fromWCharArray(L"确定"), m_bottomWidget);
	m_cancel = new QPushButton(QString::fromWCharArray(L"取消"), m_bottomWidget);

	QHBoxLayout* botLay = new QHBoxLayout;
	botLay->setAlignment(Qt::AlignVCenter);
	botLay->addStretch();
	botLay->addWidget(m_ok);
	botLay->addWidget(m_cancel);
	botLay->addSpacerItem(new QSpacerItem(15, 1, QSizePolicy::Fixed, QSizePolicy::Preferred));
	m_bottomWidget->setLayout(botLay);

	mainLayout->addWidget(m_bottomWidget);
	setLayout(mainLayout);

	QObject::connect(m_ok, SIGNAL(clicked()), this, SLOT(slotOKClicked()));
	QObject::connect(m_cancel, SIGNAL(clicked()), this, SLOT(slotCancelClicked()));

	//this->setObjectName("QtBaseTitleWidget");
	//setStyleSheet("QtBaseTitleWidget#QtBaseTitleWidget{border:1px solid red;}");
}

void QtBaseTitleWidget::setTitleIcon(const QPixmap& pixmap) 
{
	if(m_iconLabel)
		m_iconLabel->setPixmap(pixmap.scaled(m_iconLabel->width(), m_iconLabel->height()));
}

void QtBaseTitleWidget::setTitleText(const QString& str) 
{
	if(m_titleTextLabel)
	m_titleTextLabel->setText(str);
}

QWidget* QtBaseTitleWidget::getBodyWidget()
{
	return m_bodyWidget;
}

void QtBaseTitleWidget::setYellowBorder() 
{
	if(m_titleWidget)
		m_titleWidget->setStyleSheet("QFrame#m_titleWidget{background-color:#23A9FF;border-left:1px solid #FFF200;border-right:2px solid #FFF200;border-bottom:1px solid #FFF200;border-top:1px solid #FFF200;}");
		//m_titleWidget->setStyleSheet("QFrame#m_titleWidget{background-color:#28C4FF;border-left:1px solid #FFF200;border-right:2px solid #FFF200;border-top:2px solid #FFF200;border-bottom:1px solid #FFF200;}");
	//m_bodyWidget->setStyleSheet("QFrame#m_bodyWidget{background-color:#23A9FF;border-left:1px solid #FFF200;border-right:2px solid #FFF200;border-bottom:1px solid #FFF200;border-top:1px solid #FFF200;}");
	m_bodyWidget->setStyleSheet("QFrame#m_bodyWidget{border-top:2px solid rgba(28, 171, 227,200);border-right:2px solid rgba(28, 171, 227,200);border-bottom:2px solid rgba(28, 171, 227,200);background-color:rgba(17, 48, 89,140);\
		border-left:2px solid rgba(28, 171, 227,200);border-radius:0px;}");
}
void QtBaseTitleWidget::setBlueBorder() 
{
	if (m_titleWidget)
		m_titleWidget->setStyleSheet("QFrame#m_titleWidget{background:none;border-left:none;border-right:none;border-top:none;}");
		//m_titleWidget->setStyleSheet("QFrame#m_titleWidget{background-color:#28C4FF;border-left:1px solid #69FCFF;border-right:2px solid #69FCFF;border-top:2px solid #69FCFF;border-bottom:1px solid #69FCFF;}");
	//m_bodyWidget->setStyleSheet("QFrame#m_bodyWidget{background-color:#23A9FF;border-left:1px solid #69FCFF;border-right:2px solid #69FCFF;border-bottom:1px solid #69FCFF;border-top:1px solid #69FCFF;}");
	m_bodyWidget->setStyleSheet("QFrame#m_bodyWidget{border-top:2px solid rgba(28, 171, 227,200);border-right:2px solid rgba(28, 171, 227,200);border-bottom:2px solid rgba(28, 171, 227,200);background-color:rgba(17, 48, 89,140);\
		border-left:2px solid rgba(28, 171, 227,200);border-radius:0px;}");
}
#ifdef  TOOLS



void QtBaseTitleWidget::mousePressEvent(QMouseEvent* event)
{
	//ֻ�����������ƶ��͸ı��С
	if (event->button() == Qt::LeftButton)
	{
		mouse_press = true;
	}

	//�����ƶ�����
	move_point = event->globalPos() - this->pos();
	//qDebug() << "pos()" << this->pos().x() << " " << this->pos().y();
	//qDebug() << "globalPos()" << event->globalPos().x() << " " << event->globalPos().y();
}

void QtBaseTitleWidget::mouseReleaseEvent(QMouseEvent*)
{
	mouse_press = false;
}

void QtBaseTitleWidget::mouseMoveEvent(QMouseEvent* event)
{
	//�ƶ�����
	if (mouse_press)
	{
		QPoint move_pos = event->globalPos();
		move(move_pos - move_point);
		emit signalEmitPosition(QPointToQString(move_pos - move_point));
	}
}
#endif //  TOOLS
void QtBaseTitleWidget::slotCloseBtnClick()
{
	reject();
}

void QtBaseTitleWidget::slotOKClicked() 
{
	accept();
}

void QtBaseTitleWidget::slotCancelClicked() 
{
	reject();
}
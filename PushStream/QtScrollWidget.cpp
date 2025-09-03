#include "QtScrollWidget.h"
#include<QBoxLayout>

QtScrollWidget::QtScrollWidget(QWidget* pParentWidget) :QWidget(pParentWidget)
, m_scrollArea(new QScrollArea(this))
{
	m_scrollArea->setWidgetResizable(true);
	m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_scrollArea->setStyleSheet(R"(QScrollArea{background-color:transparent;}
 QScrollBar:vertical {
     border: 2px solid grey;
     background: black;
     width: 15px;
     margin: 22px 0 22px 0;
 }
 QScrollBar::handle:vertical {
     background: white;
     min-height: 20px;
 }
 QScrollBar::add-line:vertical {
     border: 2px solid grey;
     background: black;
     height: 20px;
     subcontrol-position: bottom;
     subcontrol-origin: margin;
 }

 QScrollBar::sub-line:vertical {
     border: 2px solid grey;
     background: black;
     height: 20px;
     subcontrol-position: top;
     subcontrol-origin: margin;
 }
 QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {
     border: 2px solid grey;
     width: 3px;
     height: 3px;
     background: white;
 }

 QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
     background: none;
 }
)");
	m_scrollArea->setAlignment(Qt::AlignCenter);
	m_scrollArea->setFrameShape(QFrame::Shape::NoFrame);

	QVBoxLayout* lay1 = new QVBoxLayout;
	lay1->setContentsMargins(0,0,0,0);
	lay1->setSpacing(0);
	lay1->addWidget(m_scrollArea);
	setLayout(lay1);
}

void QtScrollWidget::setWidget(QWidget* wid)
{
	m_scrollArea->setWidget(wid);
}


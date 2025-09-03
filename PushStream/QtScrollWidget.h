#pragma once

#include<qscrollarea.h>
#include<qscrollbar.h>
#include<qscroller.h>

class QtScrollWidget:public QWidget
{
public:
	QtScrollWidget(QWidget* pParentWidget = nullptr);

	void setWidget(QWidget* wid);
	
//protected:
	QScrollArea* m_scrollArea;
};


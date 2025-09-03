#pragma once
#include "QtBaseTitleMoveWidget.h"
//#include "DrawTextImage/GenTextImage.h"
#include "GLDrawText.h"
#include"opencv2/opencv.hpp"
#include <memory>


class QLabel;
class QTextEdit;
class QLineEdit;

class QtTextDlg :public QtBaseTitleMoveWidget
{
	Q_OBJECT
public:
	QtTextDlg(QWidget* parentWidget = NULL) :QtTextDlg(std::make_shared<GLDrawText>(), parentWidget) {};
	QtTextDlg(std::shared_ptr<GLDrawText>ptr, QWidget* parentWidget = NULL);
	std::shared_ptr<GLDrawText> getGenTextImage() { return m_genImage; }
protected:
	void initUI();
	void initConnection();
protected slots:
	void SlotTextChanged();
protected:
	//GenTextImage m_genImage;
	std::shared_ptr<GLDrawText>m_genImage;
	QLabel* m_label;
	QTextEdit* m_edit;
	QLineEdit*m_colorEdit;
};
#pragma once

#include<qwidget.h>
#include<qframe.h>
#include<qlabel.h>
#include<qpushbutton.h>
#include<qpixmap.h>
#include<functional>
#include<sstream>
#include<qdialog.h>


class QtBaseTitleWidget :public QDialog 
{
Q_OBJECT

public:
	QtBaseTitleWidget(QWidget* parent = nullptr, bool isTitleShow = true, bool isTransparent = true);
	void setTitleIcon(const QPixmap&pixmap);
	void setTitleText(const QString& str);
	QWidget* getBodyWidget();

	void setYellowBorder();
	void setBlueBorder();
	
#ifdef TOOLS
signals:
	void signalEmitPosition(QString pos);
protected:
	void mousePressEvent(QMouseEvent* event)override;
	void mouseReleaseEvent(QMouseEvent* event)override;
	void mouseMoveEvent(QMouseEvent* event)override;
	QPoint move_point; //�ƶ��ľ���
	bool mouse_press=false; //����������
#endif

	std::function<QString(QPoint)>QPointToQString = [](const QPoint&pt)->QString
	{
		std::stringstream ss;
		ss << pt.x() << "," << pt.y();
		return QString::fromStdString(ss.str());
	};



protected slots:
	virtual void slotCloseBtnClick();

	virtual void slotOKClicked();
	virtual void slotCancelClicked();
protected:
	QPushButton* m_closeButton = nullptr;
	QLabel* m_iconLabel = nullptr;
	QLabel* m_titleTextLabel = nullptr;

	QWidget* m_titleWidget = nullptr;
	QWidget* m_bodyWidget;
	QWidget* m_bottomWidget;
	QPushButton* m_ok;
	QPushButton* m_cancel;
};
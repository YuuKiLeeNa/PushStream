#pragma once
#include<QWidget>
#include "QtScrollWidget.h"
#include<vector>
#include<map>

class QPushButton;
class QMenu;
class QVBoxLayout;
class QAction;
class QtResizeDrawFrameBuffer;
class CSourceType;
class AMixWidget;
class QtAudioSelDlg;

class QtAddSourceWidget :public QWidget 
{
	Q_OBJECT
public:
	QtAddSourceWidget(QtResizeDrawFrameBuffer*drawWidget,QWidget*parentWidget = NULL);

	void setAMixWidget(AMixWidget* amixwidget);
	void setDrawWidget(QtResizeDrawFrameBuffer* drawWidget);


signals:
	void signalAddText(bool);


public slots:
	void slotAddBtnClick();
	void slotAddText(bool);
	void slotScrollBtnClicked();
	void slotBtnCustomContextMenuRequested(const QPoint& pt);
	void slotAudioInputActionClicked(bool);
	void slotAudioOutputActionClicked(bool);
	void slotCameraActionClicked(bool);
	void slotPicActionClicked(bool);
protected:
	void QtAudioSelDlgAdd(QtAudioSelDlg* dlg);
	void initUI();
	void initConnect();
	void createBtnToLay(std::shared_ptr<CSourceType>source, const QString&btnName);

protected:
	QtScrollWidget* m_scroll;
	QPushButton* m_addBtn;
	QPushButton* m_removeBtn;
	QMenu* m_menu;

	QVBoxLayout* m_scrollLay;
	std::vector<QAction*>m_actVec;
	QtResizeDrawFrameBuffer* m_desktop;
	AMixWidget* m_amixWidget;
	std::map<QPushButton*, std::shared_ptr<CSourceType>>m_mapBtnToAddObj;

};
#pragma once
#include"QtBaseTitleWidget.h"
#include<QPoint>

class QMouseEvent;

class QtBaseTitleMoveWidget:public QtBaseTitleWidget
{
Q_OBJECT

public:
	QtBaseTitleMoveWidget(QWidget* parent = nullptr, bool isTitleShow = true, bool isTransparent = true);
    

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);


    QPoint move_point;
    bool mouse_press;
};

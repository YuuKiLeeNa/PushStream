#include "QtBaseTitleMoveWidget.h"
#include<QMouseEvent>


QtBaseTitleMoveWidget::QtBaseTitleMoveWidget(QWidget* parent, bool isTitleShow, bool isTransparent) :QtBaseTitleWidget(parent, isTitleShow, isTransparent), mouse_press(false)
{
    
}

void QtBaseTitleMoveWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        mouse_press = true;
    }
    move_point = event->globalPos() - this->pos();
}

void QtBaseTitleMoveWidget::mouseReleaseEvent(QMouseEvent*)
{
    mouse_press = false;
}

void QtBaseTitleMoveWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mouse_press)
    {
        QPoint move_pos = event->globalPos();
        move(move_pos - move_point);
    }
}
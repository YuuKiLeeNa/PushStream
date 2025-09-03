#ifndef DRAGRESIZEWGT_H
#define DRAGRESIZEWGT_H

#include <QWidget>
#include <QMouseEvent>
#include <QPoint>

class DragResizeWgt : public QWidget
{
    Q_OBJECT

public:
    explicit DragResizeWgt(QWidget* parent = nullptr);
    ~DragResizeWgt();

public:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

private:
    void checkEdge();

private:
    //Ui::DragResizeWgt* ui;

    QPoint m_startCursor;

    int m_nLeftOff = 0;
    int m_nRightOff = 0;
    int m_nTopOff = 0;
    int m_nBottomOff = 0;

    QPoint dragPosition;
    int    edgeMargin = 4;
    enum {
        nodir,
        top = 0x01,
        bottom = 0x02,
        left = 0x04,
        right = 0x08,
        topLeft = 0x01 | 0x04,
        topRight = 0x01 | 0x08,
        bottomLeft = 0x02 | 0x04,
        bottomRight = 0x02 | 0x08
    } resizeDir; //���ĳߴ�ķ���
};

#endif // DRAGRESIZEWGT_H

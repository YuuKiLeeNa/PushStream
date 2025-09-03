#include "DragResizeWgt.h"
//#include "ui_DragResizeWgt.h"
#include <QDebug>
#include "Windows.h"
#include <QMouseEvent>
#include <QObject>
#include <QWidget>
//#define min(a,b) ((a)<(b)? (a) :(b))
//#define max(a,b) ((a)>(b)? (a) :(b))

DragResizeWgt::DragResizeWgt(QWidget* parent) :
    QWidget(parent)
{
    //ע�⣺һ��Ҫ����һ����С�Ŀ�߰£�Ҫ���ᱻ�ϵ���������Ĭ����С��0��0��
    setMinimumSize(QSize(1, 1));
    //һ��һ��Ҫ����仰�£������ܲ�׽�������������¼��Ĺؼ�������һ����Ҫ�ر�ע�⣬�����������ϻ����������widget���߿ؼ�����Ӧ����UIҲ��Ҫ����setMouseTracking�������
    this->setMouseTracking(true);
    this->setWindowFlags(Qt::FramelessWindowHint);
    edgeMargin = 4;        //���ü���ԵΪ4
    resizeDir = nodir;   //��ʼ����ⷽ��Ϊ��
}

DragResizeWgt::~DragResizeWgt()
{
}

void DragResizeWgt::mousePressEvent(QMouseEvent* event)
{
    event->ignore();
    if (event->button() == Qt::LeftButton)  //ÿ�������������ͼ�¼һ��λ��
    {
        dragPosition = event->globalPos() - frameGeometry().topLeft();  //�����갴��λ����Դ����������λ��
        m_startCursor = event->globalPos();

        m_nLeftOff = frameGeometry().left();
        m_nRightOff = frameGeometry().right();
        m_nTopOff = frameGeometry().top();
        m_nBottomOff = frameGeometry().bottom();
    }
}

void DragResizeWgt::mouseMoveEvent(QMouseEvent* event)
{
    event->ignore();
    if (event->buttons() & Qt::LeftButton)//�������ǰ��µ�
    {
        if (resizeDir == nodir)//���δ�����ڱ�Ե�������д��������϶�����
        {
            move(event->globalPos() - dragPosition);
        }
        else//��ק��Ե��������ק������д�С����
        {
            int ptop, pbottom, pleft, pright;
            ptop = m_nTopOff;
            pbottom = m_nBottomOff;
            pleft = m_nLeftOff;
            pright = m_nRightOff;

            if (resizeDir & top)//��ק�������±仯
            {
                //������ݵ�ǰ���λ������קƫ�������㵱ǰtop��λ��
                ptop = m_nTopOff - (m_startCursor.ry() - event->globalY());
                if (this->height() <= minimumHeight())//���м��˸߶���С�Ĵ���
                {
                    ptop = min(m_nBottomOff - minimumHeight(), ptop);
                }
                else if (this->height() >= maximumHeight())//���м��˸߶����Ĵ���
                {
                    ptop = max(m_nBottomOff - maximumHeight(), ptop);
                }
            }
            else if (resizeDir & bottom)//��ק�ײ����±仯
            {
                //������ݵ�ǰ���λ������קƫ�������㵱ǰbottom��λ��
                pbottom = m_nBottomOff + (event->globalY() - m_startCursor.ry());

                if (this->height() < minimumHeight())//���м��˸߶���С�Ĵ���
                {
                    pbottom = m_nTopOff + minimumHeight();
                }
                else if (this->height() > maximumHeight())//���м��˸߶����Ĵ���
                {
                    pbottom = m_nTopOff + maximumHeight();
                }
            }

            if (resizeDir & left)//��ק������ұ仯
            {
                //������ݵ�ǰ���λ������קƫ�������㵱ǰleft��λ��
                pleft = m_nLeftOff - (m_startCursor.rx() - event->globalX());

                if (this->width() <= minimumWidth())//���м��˿����С�Ĵ���
                {
                    pleft = min(pleft, m_nRightOff - minimumWidth());
                }
                else if (this->width() >= maximumWidth())//���м��˿�����Ĵ���
                {
                    pleft = max(m_nRightOff - maximumWidth(), pleft);
                }
            }
            else if (resizeDir & right)//��ק�Ҳ����ұ仯
            {
                //������ݵ�ǰ���λ������קƫ�������㵱ǰright��λ��
                pright = m_nRightOff + (event->globalX() - m_startCursor.rx());
                if (this->width() < minimumWidth())//���м��˿����С�Ĵ���
                {
                    pright = m_nLeftOff + minimumWidth();
                }
                else if (this->width() > this->maximumWidth())//���м��˿�����Ĵ���
                {
                    pright = m_nLeftOff + this->maximumWidth();
                }
            }
            setGeometry(pleft, ptop, pright - pleft, pbottom - ptop);
        }
    }
    else checkEdge();
}

void DragResizeWgt::mouseReleaseEvent(QMouseEvent* event)
{
    event->ignore();
    if (resizeDir != nodir)//��ԭ�����ʽ
    {
        checkEdge();
    }
}

void DragResizeWgt::checkEdge()
{
    QPoint pos = this->mapFromGlobal(QCursor::pos());//��ʼ��קʱ����ؼ���ʲôλ��

    int diffLeft = pos.rx();
    int diffRight = this->width() - diffLeft;
    int diffTop = pos.ry();
    int diffBottom = this->height() - diffTop;
    QCursor tempCursor;                                    //��õ�ǰ�����ʽ��ע��:ֻ�ܻ�õ�ǰ�����ʽȻ�����������������ʽ
    tempCursor = cursor();                                 //��Ϊ��õĲ������ָ�룬���Բ���������:cursor().setXXXXX

    if (diffTop < edgeMargin)
    {                              //���� ��Ե���� ����ı�ߴ�ķ���
        if (diffLeft < edgeMargin)
        {
            resizeDir = topLeft;
            tempCursor.setShape(Qt::SizeFDiagCursor);
        }
        else if (diffRight < edgeMargin)
        {
            resizeDir = topRight;
            tempCursor.setShape(Qt::SizeBDiagCursor);
        }
        else
        {
            resizeDir = top;
            tempCursor.setShape(Qt::SizeVerCursor);
        }
    }
    else if (diffBottom < edgeMargin)
    {
        if (diffLeft < edgeMargin)
        {
            resizeDir = bottomLeft;
            tempCursor.setShape(Qt::SizeBDiagCursor);
        }
        else if (diffRight < edgeMargin)
        {
            resizeDir = bottomRight;
            tempCursor.setShape(Qt::SizeFDiagCursor);
        }
        else
        {
            resizeDir = bottom;
            tempCursor.setShape(Qt::SizeVerCursor);
        }
    }
    else if (diffLeft < edgeMargin)
    {
        resizeDir = left;
        tempCursor.setShape(Qt::SizeHorCursor);
    }
    else if (diffRight < edgeMargin)
    {
        resizeDir = right;
        tempCursor.setShape(Qt::SizeHorCursor);
    }
    else
    {
        resizeDir = nodir;
        tempCursor.setShape(Qt::ArrowCursor);
    }

    setCursor(tempCursor);
}

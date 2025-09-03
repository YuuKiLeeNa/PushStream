#include "ResizeGLWidget.h"
#include <QMouseEvent>
#ifdef __cplusplus
extern "C" 
{
#endif
#include "libavutil/mathematics.h"
#include "libavutil/frame.h"
#ifdef __cplusplus
}
#endif
#include <QDebug>

#ifndef min
#define min(a,b) ((a)<(b)? (a) :(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)? (a) :(b))
#endif


#define globalY globalPosition().ry
#define globalX globalPosition().rx


ResizeGLWidget::ResizeGLWidget(QWidget* pParentWidget)
    : QtGLVideoWidget(pParentWidget)
{
    this->setMouseTracking(true);
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    this->setWindowFlags(Qt::FramelessWindowHint);
    edgeMargin = 4;
    resizeDir = nodir;
}

ResizeGLWidget::~ResizeGLWidget()
{
}

int64_t ResizeGLWidget::calcWidthOrHeighBasesOneOfTwo(int64_t v, int FrameWidth, int FrameHeight, bool isWidth)
{
    int64_t width = 0, height = 0, x, y, scr_width = 0, scr_height = 0;
    AVRational aspect_ratio;
    {
        std::lock_guard<std::mutex>lock(m_mutex);
        aspect_ratio = m_preFrameRatio;
        x = m_iPreFrameXOffset;
        y = m_iPreFrameYOffset;
    }
    if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
        aspect_ratio = av_make_q(1, 1);
    aspect_ratio = av_mul_q(aspect_ratio, av_make_q(FrameWidth, FrameHeight));

    if (isWidth)
    {
        scr_width = v - x;
        height = av_rescale(scr_width, aspect_ratio.den, aspect_ratio.num) & ~1;
        height += y;
        return height;
    }
    else
    {
        scr_height = v - y;
        width = av_rescale(scr_height, aspect_ratio.num, aspect_ratio.den) & ~1;
        width += x;
        return width;
    }
}

void ResizeGLWidget::calcWidthAndHeight(int w, int h, int* nw, int*nh, int FrameWidth, int FrameHeight, bool iszoomin)
{
    int width = 0, height = 0, x, y, scr_width = 0, scr_height = 0;
    AVRational aspect_ratio;
    {
        std::lock_guard<std::mutex>lock(m_mutex);
        aspect_ratio = m_preFrameRatio;
        x = m_iPreFrameXOffset;
        y = m_iPreFrameYOffset;
    }
    if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
        aspect_ratio = av_make_q(1, 1);
    aspect_ratio = av_mul_q(aspect_ratio, av_make_q(FrameWidth, FrameHeight));

    scr_width = w - x;
    if (scr_width < 0)
    {
        *nw = 0;
        *nh = 0;
        return;
    }
    height = av_rescale(scr_width, aspect_ratio.den, aspect_ratio.num) & ~1;
    height += y;
    if ((iszoomin && height < h) || (!iszoomin && height > h))
    {
        scr_height = h - y;
        if (scr_height < 0)
        {
            *nw = 0;
            *nh = 0;
            return;
        }
        width = av_rescale(scr_height, aspect_ratio.num, aspect_ratio.den) & ~1;
        width += x;

        *nw = width;
        *nh = h;
    }
    else 
    {
        *nw = w;
        *nh = height;
    }
}

double ResizeGLWidget::calcFrameWHRatio() 
{
    return (double)m_iPreFrameWidth/m_iPreFrameHeight;
}

//ResizeGLWidget* ResizeGLWidget::g_wid = nullptr;

void ResizeGLWidget::mousePressEvent(QMouseEvent* event)
{
    //if (g_wid != this)
    //    return QtGLVideoWidget::mouseReleaseEvent(event);
    //event->ignore();
    event->accept();
    if (event->button() == Qt::LeftButton)
    {
        dragPosition = event->globalPosition() - frameGeometry().topLeft();
        m_startCursor = event->globalPosition();

        m_nLeftOff = frameGeometry().left();
        m_nRightOff = frameGeometry().right();
        m_nTopOff = frameGeometry().top();
        m_nBottomOff = frameGeometry().bottom();

        checkEdge();
    }
}

void ResizeGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    //if (g_wid != this)
    //    return QtGLVideoWidget::mouseReleaseEvent(event);
    //event->ignore();
    event->accept();
    if (event->buttons() & Qt::LeftButton)
    {
        int ptop, pbottom, pleft, pright;
        ptop = m_nTopOff;
        pbottom = m_nBottomOff;
        pleft = m_nLeftOff;
        pright = m_nRightOff;
        switch (resizeDir)
        {
        case ResizeGLWidget::nodir:
        {
            QPointF pt = event->globalPosition() - dragPosition;
            move(pt.x(), pt.y());
            break;
        }
        case ResizeGLWidget::top:
        {
            int off = m_startCursor.ry() - event->globalY();
            ptop = m_nTopOff - off;
            //pright = pleft + calcWidthOrHeighBasesOneOfTwo(m_nBottomOff - ptop, false);
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(m_nRightOff - m_nLeftOff, m_nBottomOff - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight,off >= 0);
            pright = pleft + new_w;
            break;
        }
        case ResizeGLWidget::bottom:
        {
            int off = event->globalY() - m_startCursor.ry();
            pbottom = m_nBottomOff + off;
            //pright = pleft + calcWidthOrHeighBasesOneOfTwo(pbottom - ptop, false);
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(m_nRightOff - m_nLeftOff, pbottom - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight,off >= 0);
            pright = pleft + new_w;
            break;
        }
        case ResizeGLWidget::left:
        {
            int off = m_startCursor.rx() - event->globalX();
            pleft = m_nLeftOff - off;
            //pbottom = ptop + calcWidthOrHeighBasesOneOfTwo(pright - pleft, true);
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(pright - pleft, pbottom - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight, off > 0);
            pbottom = ptop + new_h;
            break;
        }
        case ResizeGLWidget::topLeft:
        {
            double ratio = calcFrameWHRatio();
            int xoff = m_startCursor.rx() - event->globalX();
            int yoff = m_startCursor.ry() - event->globalY();
            bool b;
            int tmpLeft = m_nLeftOff - xoff;
            int tmpTop = m_nTopOff - yoff;

            if (((tmpLeft <= pleft && tmpTop <= ptop) && (pright-tmpLeft) / ratio >= pbottom - tmpTop)
                || ((tmpLeft >=pleft && tmpTop >= ptop) && (pright - tmpLeft) / ratio >= pbottom - tmpTop)
                || (tmpLeft <= pleft && tmpTop >= ptop))
            {
                pleft = tmpLeft;
                b = xoff >= 0;
                //ptop = pbottom - calcWidthOrHeighBasesOneOfTwo(pright - pleft, true);
            }
            else 
            {
                ptop = tmpTop;
                b = yoff >= 0;
                //pleft = pright - calcWidthOrHeighBasesOneOfTwo(m_nBottomOff - ptop, false);
            }
            //break;
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(pright - pleft, m_nBottomOff - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight, b);
            ptop = pbottom - new_h;
            pleft = pright - new_w;
            break;
        }
        case ResizeGLWidget::bottomLeft: 
        {
            double ratio = calcFrameWHRatio();
            int xoff = m_startCursor.rx() - event->globalX();
            int yoff = event->globalY() - m_startCursor.ry();
            bool b;
            int tmpLeft = m_nLeftOff - xoff;
            int tmpBottom = m_nBottomOff + yoff;
            if (((tmpLeft <= pleft && pbottom <= tmpBottom) && (pright - tmpLeft) / ratio >= tmpBottom - ptop)
                || ((tmpLeft >= pleft && pbottom >= tmpBottom) && (pright - tmpLeft) / ratio >= tmpBottom - ptop)
                || (tmpLeft <= pleft && tmpBottom <= pbottom))
            {
                pleft = tmpLeft;
                b = xoff >= 0;
                //pbottom = ptop + calcWidthOrHeighBasesOneOfTwo(m_nRightOff - pleft, true);
            }
            else 
            {
                pbottom = tmpBottom;
                b = yoff >= 0;
                //pleft =  pright - calcWidthOrHeighBasesOneOfTwo(pbottom - ptop, false);
            }
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(pright - pleft, pbottom - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight, b);
            pleft = pright - new_w;
            pbottom = ptop + new_h;
            break;
        }
        case ResizeGLWidget::right:
        {
            int off = event->globalX() - m_startCursor.rx();
            pright = m_nRightOff + off;
            //pbottom = ptop + calcWidthOrHeighBasesOneOfTwo(pright - pleft, true);
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(pright - pleft, pbottom - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight, off >= 0);
            pbottom = ptop + new_h;
            break;
        }
        case ResizeGLWidget::topRight:
        {
            double ratio = calcFrameWHRatio();
            int yoff = m_startCursor.ry() - event->globalY();
            int xoff = event->globalX() - m_startCursor.rx();
            bool b;
            int tmpTop = m_nTopOff - yoff;//(m_startCursor.ry() - event->globalY());
            int tmpRight = m_nRightOff + xoff;//(event->globalX() - m_startCursor.rx());

            //qDebug()<<"ratio="<< ratio << " pleft=" << pleft << " ptop=" << ptop << " pright=" << pright << " pbottom=" << pbottom;
            //qDebug() << "tmptop=" << tmpTop << " tmppright=" << tmpRight;
            if (((tmpTop <= ptop && tmpRight >= pright) && (pbottom - tmpTop) * ratio >= tmpRight - pleft)
                || ((tmpTop >= ptop && tmpRight <= pright) && (pbottom - tmpTop) * ratio >= tmpRight - pleft)
                || (tmpTop <= ptop && tmpRight <= pright)) 
            {
                ptop = tmpTop;
                b = yoff >= 0;
                //pright = pleft + calcWidthOrHeighBasesOneOfTwo(pbottom - ptop, false);
                //qDebug() << "calc height:ptop=" << ptop << " pright=" << pright;
            }
            else 
            {
                pright = tmpRight;
                b = xoff >= 0;
                //ptop = pbottom - calcWidthOrHeighBasesOneOfTwo(pright - pleft, true);
                //qDebug() << "calc width:ptop=" << ptop << " pright=" << pright;
            }
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(pright - pleft, pbottom - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight, b);
            pright = pleft + new_w;
            ptop = pbottom - new_h;
            break;
        }
        case ResizeGLWidget::bottomRight:
        {
            double ratio = calcFrameWHRatio();

            int yoff = event->globalY() - m_startCursor.ry();
            int xoff = event->globalX() - m_startCursor.rx();
            bool b;

            int tmpRight = m_nRightOff + xoff;//(event->globalX() - m_startCursor.rx());
            int tmpBottom = m_nBottomOff + yoff;//(event->globalY() - m_startCursor.ry());
            if (((tmpRight >= pright && tmpBottom >= pbottom) && (tmpRight - pleft) / ratio >= tmpBottom - ptop)
                || ((tmpRight <= pright && tmpBottom <= pbottom) && (tmpRight - pleft) / ratio >= tmpBottom - ptop)
                || (tmpRight >= pright && tmpBottom <= pbottom))
            {
                pright = tmpRight;
                b = xoff >= 0;
                //pbottom = ptop + calcWidthOrHeighBasesOneOfTwo(pright - pleft, true);
            }
            else 
            {
                pbottom = tmpBottom;
                b = yoff >= 0;
                //pright = pleft + calcWidthOrHeighBasesOneOfTwo(pbottom - ptop, false);
            }
            int new_w = 0, new_h = 0;
            calcWidthAndHeight(pright - pleft, pbottom - ptop, &new_w, &new_h, m_iPreFrameWidth, m_iPreFrameHeight, b);
            pbottom = ptop + new_h;
            pright = pleft + new_w;
            break;
        }
        default:
            break;
        }
        if(resizeDir != ResizeGLWidget::nodir)
            setGeometry(pleft, ptop, pright - pleft, pbottom - ptop);
    }
    else
        checkEdge(false);
}

void ResizeGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    //if (g_wid != this)
     //   return QtGLVideoWidget::mouseReleaseEvent(event);
    //event->ignore();
    event->accept();
    if (resizeDir != nodir)
    {
        QCursor tempCursor;
        resizeDir = nodir;
        tempCursor.setShape(Qt::ArrowCursor);
        setCursor(tempCursor);
    }
}

//void ResizeGLWidget::setFrameSize(int w, int h) 
//{
//    m_iPreFrameWidth = w;
//    m_iPreFrameHeight = h;
//}

void ResizeGLWidget::resizeGL(int width, int height) 
{
    //QOpenGLWidget::resizeGL(width, height);

    if (resizeDir != nodir && hasFocus())
        return;
    resizeDir = nodir;
    QWidget*wid = dynamic_cast<QWidget*>(parent());
    if (wid == nullptr)
        return QtGLVideoWidget::resizeGL(width, height);
    int pw = wid->width();
    int ph = wid->height();
   /* QRect rect = geometry();
    int newW, newH;
    int oldW = rect.width();
    int oldH = rect.height();*/
    //calcWidthAndHeight()
    //calcWidthAndHeight();
    int neww;
    int newh;
    calcWidthAndHeight(width, height, &neww, &newh, m_iPreFrameWidth, m_iPreFrameHeight, false);
    setGeometry((pw - neww)/2, (ph - newh)/2, neww, newh);
    update();
    //QOpenGLWidget::resizeGL(neww, newh);
}

void ResizeGLWidget::renderFrame(UPTR_FME f)
{
    //if (f && ((f->width != m_iPreFrameWidth || f->height != m_iPreFrameHeight) && (f->width != 0 && f->height != 0))) 
    //    setFrameSize(f->width, f->height);
    //int w = width(); 
    //int h = height();
    //if (std::abs((float)m_iPreFrameWidth / m_iPreFrameHeight - (float)w/h) > 0.1)
    //{
    //    QWidget* wid = dynamic_cast<QWidget*>(parent());
    //    if (wid == nullptr)
    //    {
    //        QtGLVideoWidget::renderFrame(f);
    //        return;
    //    }
    //    int pw = wid->width();
    //    int ph = wid->height();
    //    int neww,newh;
    //    //calcWidthAndHeight(pw, ph, &neww, &newh, f->width, f->height, true);
    //    neww = calcWidthOrHeighBasesOneOfTwo(ph, f->width, f->height, false);
    //    if (neww > pw)
    //    {
    //        neww = pw;
    //        newh = neww* f->height / f->width;
    //    }
    //    else
    //        newh = ph;
    //    setGeometry((pw - neww) / 2, (ph - newh) / 2, neww, newh);
    //}
    setFrame(&*f);
    QtGLVideoWidget::renderFrame(std::move(f));
}

void ResizeGLWidget::setFrame(AVFrame* f)
{
    if (f && ((f->width != m_iPreFrameWidth || f->height != m_iPreFrameHeight) && (f->width != 0 && f->height != 0)))
        setFrameSize(f->width, f->height);
    int w = width();
    int h = height();
    if (std::abs((float)m_iPreFrameWidth / m_iPreFrameHeight - (float)w / h) > 0.1)
    {
        QWidget* wid = dynamic_cast<QWidget*>(parent());
        if (wid == nullptr)
            return;
        int pw = wid->width();
        int ph = wid->height();
        int neww, newh;
        //calcWidthAndHeight(pw, ph, &neww, &newh, f->width, f->height, true);
        neww = calcWidthOrHeighBasesOneOfTwo(ph, f->width, f->height, false);
        if (neww > pw)
        {
            neww = pw;
            newh = neww * f->height / f->width;
        }
        else
            newh = ph;
        setGeometry((pw - neww) / 2, (ph - newh) / 2, neww, newh);
    }
}

void ResizeGLWidget::focusInEvent(QFocusEvent* event) 
{
    QWidget* wid = this;
    while(wid= dynamic_cast<QWidget*>(wid->parent()))
        wid->clearFocus();
}

void ResizeGLWidget::focusOutEvent(QFocusEvent* event) 
{
    resizeDir = nodir;
    QCursor tempCursor;
    tempCursor = cursor();
    tempCursor.setShape(Qt::ArrowCursor);
    setCursor(tempCursor);
}


void ResizeGLWidget::checkEdge(bool changedState)
{
    if (!hasFocus())
    {
        QCursor tempCursor;
        resizeDir = nodir;
        tempCursor.setShape(Qt::ArrowCursor);
        setCursor(tempCursor);
        return;
    }
    QPoint pos = this->mapFromGlobal(QCursor::pos());

    int diffLeft = pos.rx();
    int diffRight = this->width() - diffLeft;
    int diffTop = pos.ry();
    int diffBottom = this->height() - diffTop;
    QCursor tempCursor;                                    
    tempCursor = cursor();

    if (diffTop < edgeMargin)
    {                              
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
    if (!changedState) 
        resizeDir = nodir;
}

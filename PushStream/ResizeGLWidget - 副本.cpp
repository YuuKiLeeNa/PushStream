#include "ResizeGLWidget.h"
#include "CameraCapture/camera.hpp"
#include "PicToYuv.h"
#include <QMouseEvent>
#include<qmessagebox.h>
#define min(a,b) ((a)<(b)? (a) :(b))
#define max(a,b) ((a)>(b)? (a) :(b))

ResizeGLWidget::ResizeGLWidget(QWidget* pParentWidget)
    : QtGLVideoWidget(pParentWidget)
    ,m_con(std::make_shared<PicToYuv>())
{
    setMinimumSize(QSize(100, 100));
    this->setMouseTracking(true);

    this->setWindowFlags(Qt::FramelessWindowHint);
    edgeMargin = 4;        //设置检测边缘为4
    resizeDir = nodir;   //初始化检测方向为无
}

ResizeGLWidget::~ResizeGLWidget()
{
    m_camera.reset();
}

bool ResizeGLWidget::openCamera(const std::wstring& strCamera)
{
    decltype(m_camera) camera = decltype(m_camera)(new Camera, [](Camera* p) {p->Close(), delete p; });
    return openCamera(std::move(camera), strCamera);
}

bool ResizeGLWidget::openCamera(std::unique_ptr<Camera, std::function<void(Camera*)>>&& camera, const std::wstring& strCamera)
{
    m_camera = std::move(camera);
    m_camera->Close();
    m_camera->SetCallBack(std::bind(&ResizeGLWidget::GetFrame, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    if (!m_camera->Open(strCamera))
    {
        QMessageBox::warning(nullptr, "error", QString("open %1 error\n").arg(QString::fromStdWString(strCamera)), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return false;
    }
    return true;
}

void ResizeGLWidget::mousePressEvent(QMouseEvent* event)
{
    event->ignore();
    if (event->button() == Qt::LeftButton)  //每当按下鼠标左键就记录一下位置
    {
        dragPosition = event->globalPos() - frameGeometry().topLeft();  //获得鼠标按键位置相对窗口左上面的位置
        m_startCursor = event->globalPos();

        m_nLeftOff = frameGeometry().left();
        m_nRightOff = frameGeometry().right();
        m_nTopOff = frameGeometry().top();
        m_nBottomOff = frameGeometry().bottom();
    }
}

void ResizeGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    event->ignore();
    if (event->buttons() & Qt::LeftButton)//如果左键是按下的
    {
        if (resizeDir == nodir)//鼠标未放置在边缘处，进行窗口整体拖动处理
        {
            move(event->globalPos() - dragPosition);
        }
        else//拖拽边缘，根据拖拽方向进行大小调整
        {
            int ptop, pbottom, pleft, pright;
            ptop = m_nTopOff;
            pbottom = m_nBottomOff;
            pleft = m_nLeftOff;
            pright = m_nRightOff;

            if (resizeDir & top)//拖拽顶部上下变化
            {
                //计算根据当前鼠标位置与拖拽偏移量计算当前top的位置
                ptop = m_nTopOff - (m_startCursor.ry() - event->globalY());
                if (this->height() <= minimumHeight())//进行极端高度最小的处理
                {
                    ptop = min(m_nBottomOff - minimumHeight(), ptop);
                }
                else if (this->height() >= maximumHeight())//进行极端高度最大的处理
                {
                    ptop = max(m_nBottomOff - maximumHeight(), ptop);
                }
            }
            else if (resizeDir & bottom)//拖拽底部上下变化
            {
                //计算根据当前鼠标位置与拖拽偏移量计算当前bottom的位置
                pbottom = m_nBottomOff + (event->globalY() - m_startCursor.ry());

                if (this->height() < minimumHeight())//进行极端高度最小的处理
                {
                    pbottom = m_nTopOff + minimumHeight();
                }
                else if (this->height() > maximumHeight())//进行极端高度最大的处理
                {
                    pbottom = m_nTopOff + maximumHeight();
                }
            }

            if (resizeDir & left)//拖拽左侧左右变化
            {
                //计算根据当前鼠标位置与拖拽偏移量计算当前left的位置
                pleft = m_nLeftOff - (m_startCursor.rx() - event->globalX());

                if (this->width() <= minimumWidth())//进行极端宽度最小的处理
                {
                    pleft = min(pleft, m_nRightOff - minimumWidth());
                }
                else if (this->width() >= maximumWidth())//进行极端宽度最大的处理
                {
                    pleft = max(m_nRightOff - maximumWidth(), pleft);
                }
            }
            else if (resizeDir & right)//拖拽右侧左右变化
            {
                //计算根据当前鼠标位置与拖拽偏移量计算当前right的位置
                pright = m_nRightOff + (event->globalX() - m_startCursor.rx());
                if (this->width() < minimumWidth())//进行极端宽度最小的处理
                {
                    pright = m_nLeftOff + minimumWidth();
                }
                else if (this->width() > this->maximumWidth())//进行极端宽度最大的处理
                {
                    pright = m_nLeftOff + this->maximumWidth();
                }
            }
            setGeometry(pleft, ptop, pright - pleft, pbottom - ptop);
        }
    }
    else checkEdge();
}

void ResizeGLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    event->ignore();
    if (resizeDir != nodir)//还原鼠标样式
    {
        checkEdge();
    }
}

std::pair<int, int> ResizeGLWidget::setFrameSizeAccordCamera()
{
    int w = m_camera->GetWidth();
    int h = m_camera->GetHeight();
    m_iPreFrameWidth = w;
    m_iPreFrameHeight = h;
    return {w,h};
}

void ResizeGLWidget::checkEdge()
{
    QPoint pos = this->mapFromGlobal(QCursor::pos());//开始拖拽时点击控件的什么位置

    int diffLeft = pos.rx();
    int diffRight = this->width() - diffLeft;
    int diffTop = pos.ry();
    int diffBottom = this->height() - diffTop;
    QCursor tempCursor;                                    //获得当前鼠标样式，注意:只能获得当前鼠标样式然后再重新设置鼠标样式
    tempCursor = cursor();                                 //因为获得的不是鼠标指针，所以不能这样用:cursor().setXXXXX

    if (diffTop < edgeMargin)
    {                              //根据 边缘距离 分类改变尺寸的方向
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


void ResizeGLWidget::GetFrame(long, uint8_t* buff, long len)
{
    /*static auto last_time = system_clock::now();
    static auto last_avg_time = system_clock::now();
    static int32_t dt_avg_v = 0, total_count = 0;
    auto cur_time = system_clock::now();
    auto dt = duration_cast<milliseconds>(cur_time - last_time);

    total_count++;

    if (total_count % 30 == 0) {
        auto cur_avg_time = system_clock::now();
        auto dt_avg = duration_cast<milliseconds>(cur_avg_time - last_avg_time);
        dt_avg_v = (int32_t)dt_avg.count();
        last_avg_time = cur_avg_time;
    }

    last_time = cur_time;
    int32_t dt_v = (int32_t)dt.count();
    printf("frame size: h = %d, w = %d, fps_rt: %0.02f fps: %0.02f (%d)\n",
        camera->GetHeight(), camera->GetWidth(), 1000.0f / dt_v,
        dt_avg_v ? 1000.0f * 30 / dt_avg_v : 0, total_count);

    if (total_count == 100)
    {*/

    int iWidth = m_camera->GetWidth();
    int iHeight = m_camera->GetHeight();
    int iBitCount = m_camera->GetBitDepth();
    if (m_lWidth != iWidth
        || m_lHeight != iHeight
        || m_iBitCount != iBitCount)
    {
        m_lWidth = iWidth;
        m_lHeight = iHeight;
        m_iBitCount = iBitCount;
        //setYUV420pParameters(m_lWidth, m_lHeight);
        m_con->setInfo(iWidth, iHeight, m_iBitCount);
    }
    AVFrame* tmp = av_frame_alloc();
    if ((*m_con)(buff, len, tmp))
        setBackgroundImage(tmp);
}

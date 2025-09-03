#include "testOpenGL.h"
#include "OpenGLFrameBuff/QtResizeDrawFrameBuffer.h"
#include "OpenGLFrameBuff/QtDrawFrameBuffer.h"
#include<QBoxLayout>

#ifdef __cplusplus
extern "C" {
#endif
#include"libavdevice/avdevice.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"

#include "libavutil/imgutils.h"

#ifdef __cplusplus
}
#endif
#include<cassert>
#include<QResizeEvent>
#include"PushStream/imageConvert.h"
#include "define_type.h"

testOpenGL::testOpenGL(QWidget *parent)
    : QMainWindow(parent)
{
    resize(800,600);
    initUI(this);

    //FILE* yuv_file = fopen(R"(F:\1920x1080.yuv)", "rb");
    //FILE* yuv_file = fopen(R"(F:\1920x1080.rgb)", "rb");
    FILE* yuv_file = fopen(R"(F:\first_frame1920x1080.rgba)", "rb");
    
    int pic_width = 1920;
    int pic_height = 1080;
    AVFrame* frame = av_frame_alloc();
    assert(frame);
    frame->width = pic_width;
    frame->height = pic_height;
    frame->format = AV_PIX_FMT_RGBA;
    int ret = av_frame_get_buffer(frame, 1);
    assert(ret == 0);
    ret = av_frame_make_writable(frame);
    assert(ret == 0);
	AVFrame* f = frame;


   /* AVFrame* frame_rgba = av_frame_alloc();
    frame_rgba->width = pic_width;
    frame_rgba->height = pic_height;
    frame_rgba->format = AV_PIX_FMT_RGBA;

    ret = av_frame_get_buffer(frame_rgba, 1);
    assert(ret == 0);
    ret = av_frame_make_writable(frame_rgba);
    assert(ret == 0);
    */

    int bytes_pix_fmt = av_get_bits_per_pixel(av_pix_fmt_desc_get((AVPixelFormat)frame->format))/8;

    for (int h = 0; h < pic_height; ++h)
		if (fread(f->data[0] + f->linesize[0] * h, bytes_pix_fmt, pic_width, yuv_file) != pic_width)
		{
			printf("error\n");
            break;
		}
	//
	//for (int h = 0; h < pic_height/2; ++h)
	//    if (fread(f->data[1] + f->linesize[1] * h, 1, pic_width / 2, yuv_file) != pic_width / 2)
	//	{
	//		printf("error\n");
 //           break;
	//	}

	//for (int h = 0; h < pic_height/2; ++h)
	//	if (fread(f->data[2] + f->linesize[2] * h, 1, pic_width / 2, yuv_file) != pic_width / 2)
	//		{
	//			printf("error\n");
 //               break;
	//		}



   /* imageConvert con;
    bool b= con.init(AV_PIX_FMT_YUV420P, pic_width, pic_height, AV_PIX_FMT_RGBA, pic_width, pic_height);
    assert(b);

    b = con(f->data, f->linesize, frame_rgba->data, frame_rgba->linesize);
    assert(b);*/


    //m_wid->setFrameSize(pic_width, pic_height);
    //m_wid->initializeGL();
    m_wid->renderFrame(UPTR_FME(f));
    fclose(yuv_file);
}

testOpenGL::~testOpenGL()
{

    //if (m_frame)
     //   av_frame_free(&m_frame);
}

void testOpenGL::initUI(QWidget * parentWid)
{
    m_wid = new std::remove_pointer_t<decltype(m_wid)>;//new QtDrawFrameBuffer(parentWid);
    setCentralWidget(m_wid);
   /* QHBoxLayout* lay = new QHBoxLayout;
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);
    lay->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_wid);
    parentWid->setLayout(lay);*/
}

void testOpenGL::resizeEvent(QResizeEvent* event)
{
    QSize size = event->size();
    //m_wid->resizeGL(size.width(), size.height());
    m_wid->resize(size.width(), size.height());
    update();
}

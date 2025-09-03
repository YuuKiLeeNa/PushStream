#include "QtGLVideoWidget.h"
#include <QOpenGLShader>
#include<memory>

#include<algorithm>
#include<numeric>

#ifdef __cplusplus
extern "C" {
#endif
#include"libavdevice/avdevice.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif


#define FRAG_SHADER_YUV420P R"(#version 330 core
uniform sampler2D y_tex;
uniform sampler2D u_tex;
uniform sampler2D v_tex;
in vec2 v_coord;
//layout( location = 0 ) out vec4 fragcolor;
out vec4 fragcolor;

const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);
const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);
const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);
const vec3 offset = vec3(-0.0625, -0.5, -0.5);

void main() {
  float y = texture(y_tex, v_coord).r;
  float u = texture(u_tex, v_coord).r;
  float v = texture(v_tex, v_coord).r;
  vec3 yuv = vec3(y,u,v);
  yuv += offset;
  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);
  fragcolor.r = dot(yuv, R_cf);
  fragcolor.g = dot(yuv, G_cf);
  fragcolor.b = dot(yuv, B_cf);
})"

#define FRAG_SHADER_RGBA R"(#version 330 core
uniform sampler2D rgba_tex;
in vec2 v_coord;
out vec4 fragcolor;
void main() {
  fragcolor = texture(rgba_tex, v_coord);
})"



#define VERT_SHADER R"(#version 330 core
uniform mat4 u_pm;
uniform vec4 draw_pos;

const vec2 verts[4] = vec2[] (
  vec2(-0.5,  0.5),
  vec2(-0.5, -0.5),
  vec2( 0.5,  0.5),
  vec2( 0.5, -0.5)
);

const vec2 texcoords[4] = vec2[] (
  vec2(0.0, 1.0),
  vec2(0.0, 0.0),
  vec2(1.0, 1.0),
  vec2(1.0, 0.0)
);

out vec2 v_coord;

void main() {
   vec2 vert = verts[gl_VertexID];
   vec4 p = vec4((0.5 * draw_pos.z) + draw_pos.x + (vert.x * draw_pos.z),
                 (0.5 * draw_pos.w) + draw_pos.y + (vert.y * draw_pos.w),
                 0, 1);
   gl_Position = u_pm * p;
   v_coord = texcoords[gl_VertexID];
})"

bool operator==(const AVRational & r1, const AVRational & r2)
{
	return memcmp(&r1, &r2, sizeof(AVRational)) == 0;
}

QtGLVideoWidget::QtGLVideoWidget(QWidget* pParentWidget) :QOpenGLWidget(pParentWidget)
{
	QSurfaceFormat defaultFormat = QSurfaceFormat::defaultFormat();
	defaultFormat.setProfile(QSurfaceFormat::CoreProfile);
	defaultFormat.setVersion(3, 3); // Adapt to your system
	QSurfaceFormat::setDefaultFormat(defaultFormat);
	setFormat(defaultFormat);
	m_frameSave = nullptr;
	initConnect();
	setMouseTracking(true);
	setAcceptDrops(true);
	m_frame_tmp.reset(av_frame_alloc());
}

QtGLVideoWidget::~QtGLVideoWidget()
{
	makeCurrent();
	deleteObject();
	doneCurrent();
}

void QtGLVideoWidget::renderFrame(UPTR_FME f)
{
	setFrame(std::move(f));
	update();
}

AVPixelFormat QtGLVideoWidget::getNeededPixelFormat()
{
	return m_pixNeeded;
}


void QtGLVideoWidget::setFrameSize(int width, int height)
{
	m_iPreFrameWidth = width;
	m_iPreFrameHeight = height;
}

bool QtGLVideoWidget::isSupportPixelFormat(AVPixelFormat fmt)
{
	bool b = false;
	switch (fmt)
	{
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_RGB24:
		b = true;
		break;
	default:
		break;
	}
	return b;
}

void QtGLVideoWidget::deleteObject()
{
	m_program.destroyed();
	m_program.removeAllShaders();
	m_vao.destroy();
	deleteTex();
}



void QtGLVideoWidget::setFrame(UPTR_FME f)
{
	if (f == nullptr)
	{
		if (!m_isOpenGLInit)
		{
			std::lock_guard<std::mutex>lock(m_mutex);
			if (m_frameSave)
				m_frameSave.reset();
		}
		return;
	}
	if (!isSupportPixelFormat((AVPixelFormat)f->format)) 
	{
		if (!m_imgConvert.init((AVPixelFormat)f->format, f->width, f->height, AV_PIX_FMT_YUV420P, f->width, f->height))
			return;
		UPTR_FME tmp(av_frame_alloc());
		if (!tmp)
			return;
		if (!m_imgConvert(&*f, &*tmp))
			return;
		f.swap(tmp);
	}
	{
		std::lock_guard<std::mutex>lock(m_mutex);
		if ((AVPixelFormat)f->format != m_pixNeeded)
		{
			m_pixNeeded = (AVPixelFormat)f->format;
			m_isOpenGLInit = false;
		}
		m_frameSave = std::move(f);
		m_bUploadTex = true;
	}
}

QRect QtGLVideoWidget::calculate_display_rect(AVFrame* f)
{
	if (f)
		return calculate_display_rect(0, 0, width(), height(), f->width, f->height, f->sample_aspect_ratio);
	return QRect();
}

QRect QtGLVideoWidget::calculate_display_rect(
	int scr_xleft, int scr_ytop, int scr_width, int scr_height,
	int pic_width, int pic_height, AVRational pic_sar)
{
	/*if (m_iPreFrameXOffset == scr_xleft
		&& m_iPreFrameYOffset == scr_ytop
		&& m_iPreFrameWidth == pic_width
		&& m_iPreFrameHeight == pic_height
		&& m_iPreFrameWinWidth == scr_width
		&& m_iPreFrameWinHeight == scr_height
		&& m_preFrameRatio == pic_sar)
		return;*/


	m_iPreFrameXOffset = scr_xleft;
	m_iPreFrameYOffset = scr_ytop;
	m_iPreFrameWidth = pic_width;
	m_iPreFrameHeight = pic_height;
	m_iPreFrameWinWidth = scr_width;
	m_iPreFrameWinHeight = scr_height;
	m_preFrameRatio = pic_sar;
	AVRational aspect_ratio = pic_sar;
	int64_t width, height, x, y;

	if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
		aspect_ratio = av_make_q(1, 1);
	aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));

	/* XXX: we suppose the screen has a 1.0 pixel ratio */
	height = scr_height;
	width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
	if (width > scr_width) {
		width = scr_width;
		height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
	}
	x = (scr_width - width) / 2;
	y = (scr_height - height) / 2;
	return QRect(scr_xleft + x, scr_ytop + y, FFMAX((int)width, 1), FFMAX((int)height, 1));
}

void QtGLVideoWidget::deleteTex()
{
	auto del_tex = [](QOpenGLTexture*& tex)
		{
			if (tex)
			{
				tex->destroy();
				delete tex;
				tex = NULL;
			}
		};
	del_tex(m_textureY);
	del_tex(m_textureU);
	del_tex(m_textureV);
	del_tex(m_textureRGB);
	del_tex(m_textureRGBA);
}

void QtGLVideoWidget::initConnect()
{
}



void QtGLVideoWidget::initializeGL()
{
	Q_ASSERT(context());
	initializeOpenGLFunctions();
	initShader();
	initializeTextures();
}

void QtGLVideoWidget::paintGL()
{
	if (!m_frameSave)
	{
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}
	bool bUploadTex;
	{
		std::lock_guard<std::mutex>lock(m_mutex);
		if (bUploadTex = m_bUploadTex)
		{
			m_bUploadTex = false;
			av_frame_unref(&*m_frame_tmp);
			if (m_frameSave)
				av_frame_ref(&*m_frame_tmp, &*m_frameSave);
		}
	}
	QRect rect = calculate_display_rect(&*m_frame_tmp);
	if (!m_isOpenGLInit)
	{
		if (!initShader())
			return;
		initializeTextures();

		m_isOpenGLInit = true;
	}

	/*
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_vao.bind();
	QMatrix4x4 m;
	m.ortho(0, width(), height(), 0.0, 0.0, 100.0f);
	m_program.setUniformValue("u_pm", m);
	glUniform4f(u_pos, m_drawRect.left(), m_drawRect.top(), m_drawRect.width(), m_drawRect.height());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, y_tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, u_tex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, v_tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	*/

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_program.bind();
	m_vao.bind();

	QMatrix4x4 m;
	m.ortho(0, width(), height(), 0.0, 0.0, 100.0f);
	m_program.setUniformValue("u_pm", m);
	glUniform4f(u_pos, rect.left(), rect.top(), rect.width(), rect.height());

	switch ((AVPixelFormat)m_frame_tmp->format)
	{
	case AV_PIX_FMT_YUV420P:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureY->textureId());
		if (bUploadTex)
		{
			m_program.setUniformValue("y_tex", 0);
			setYPixels(m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
		}
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_textureU->textureId());
		if (bUploadTex)
		{
			m_program.setUniformValue("u_tex", 1);
			setUPixels(m_frame_tmp->data[1], m_frame_tmp->linesize[1]);
		}
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_textureV->textureId());
		if (bUploadTex)
		{
			m_program.setUniformValue("v_tex", 2);
			setVPixels(m_frame_tmp->data[2], m_frame_tmp->linesize[2]);
		}
		break;
	case AV_PIX_FMT_RGB24:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureRGB->textureId());
		if (bUploadTex)
		{
			m_program.setUniformValue("rgba_tex", 0);
			setRGBPixels(m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
		}
		break;
	case AV_PIX_FMT_RGBA:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureRGBA->textureId());
		if (bUploadTex)
		{
			m_program.setUniformValue("rgba_tex", 0);
			setRGBAPixels(m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
		}
		break;
	default:
		break;
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	m_program.release();
	m_vao.release();
}

void QtGLVideoWidget::resizeGL(int width, int height)
{
	//calculate_display_rect(m_frameSave);
	QOpenGLWidget::resizeGL(width, height);
	if (m_frameSave)
	{
		calculate_display_rect(0, 0, width, height, m_frameSave->width, m_frameSave->height, m_frameSave->sample_aspect_ratio);
		update();
	}
}

void QtGLVideoWidget::setRGBPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(m_textureRGB->textureId(), RGBTexture, pixels, stride);
}

void QtGLVideoWidget::setRGBAPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(m_textureRGBA->textureId(), RGBATexture, pixels, stride);
}

void QtGLVideoWidget::setYPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(m_textureY->textureId(), YTexture, pixels, stride);
}

void QtGLVideoWidget::setUPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(m_textureU->textureId(), UTexture, pixels, stride);
}

void QtGLVideoWidget::setVPixels(uint8_t* pixels, int stride)
{
	bindPixelTexture(m_textureV->textureId(), VTexture, pixels, stride);
}

void QtGLVideoWidget::initializeTextures()
{
	auto init_tex = [this](QOpenGLTexture*& tex, int frameW, int frameH, GLint internalformat, GLenum format)
		{
			if (tex)
				tex->destroy();
			else
				tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
			tex->create();
			//m_textureY->setFormat(QOpenGLTexture::LuminanceFormat);
			//m_textureY->setSize(m_iPreFrameWidth, m_iPreFrameHeight); // Y, U, V 分量
			// 加载YUV数据到纹理
			//m_textureY->allocateStorage(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8);
			glBindTexture(GL_TEXTURE_2D, tex->textureId());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, frameW, frameH, 0, format, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		};

	switch (m_pixNeeded)
	{
	case AV_PIX_FMT_YUV420P:
		init_tex(m_textureY, m_iPreFrameWidth, m_iPreFrameHeight, GL_R8, GL_RED);
		init_tex(m_textureU, m_iPreFrameWidth / 2, m_iPreFrameHeight / 2, GL_R8, GL_RED);
		init_tex(m_textureV, m_iPreFrameWidth / 2, m_iPreFrameHeight / 2, GL_R8, GL_RED);
		break;
	case AV_PIX_FMT_RGB24:
		init_tex(m_textureRGB, m_iPreFrameWidth, m_iPreFrameHeight, GL_RGB, GL_RGB);
		break;
	case AV_PIX_FMT_RGBA:
		init_tex(m_textureRGBA, m_iPreFrameWidth, m_iPreFrameHeight, GL_RGBA, GL_RGBA);
		break;
	default:
		break;
	}
}

void QtGLVideoWidget::bindPixelTexture(GLuint texture, QtGLVideoWidget::TextureType textureType, uint8_t* pixels, int stride)
{
	if (!pixels)
		return;

	int frameW = m_iPreFrameWidth;
	int frameH = m_iPreFrameHeight;

	int width, height;// = textureType == YTexture ? frameW : frameW / 2;
	//int height = textureType == YTexture ? frameH : frameH / 2;

	if (textureType == YTexture
		|| textureType == RGBATexture
		|| textureType == RGBTexture)
	{
		width = frameW;
		height = frameH;
	}
	else
	{
		width = frameW / 2;
		height = frameH / 2;
	}


	//makeCurrent();
	/*glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);*/

	glBindTexture(GL_TEXTURE_2D, texture);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
	/*glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);*/
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pixels);
	switch (textureType) {
	case YTexture:
	case UTexture:
	case VTexture:
		glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
		break;
	case RGBTexture:
		assert(stride % 3 == 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / 3);
		//glPixelStorei(GL_UNPACK_ALIGNMENT, 3);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		break;
	case RGBATexture:
		assert(stride % 4 == 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / 4);
		//glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		break;
	default:
		break;
	}
}

bool QtGLVideoWidget::initShader()
{
	m_program.removeAllShaders();
	m_program.destroyed();
	if (!m_program.create())
	{
		qDebug() << "m_program.create() failed";
	}
	glDisable(GL_DEPTH_TEST);

	m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, VERT_SHADER);

	if (m_pixNeeded == AV_PIX_FMT_YUV420P)
		m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SHADER_YUV420P);
	else if (m_pixNeeded == AV_PIX_FMT_RGB24 || m_pixNeeded == AV_PIX_FMT_RGBA)
		m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SHADER_RGBA);
	bool b = m_program.link();

	if (!b)
	{
		int linkStatus = 0;
		glGetProgramiv(m_program.programId(), GL_LINK_STATUS, &linkStatus);
		if (!linkStatus) {
			char infoLog[512];
			glGetProgramInfoLog(m_program.programId(), 512, NULL, infoLog);
		}
		return b;
	}
	if (m_pixNeeded == AV_PIX_FMT_YUV420P)
	{
		m_program.setUniformValue("y_tex", 0);
		m_program.setUniformValue("u_tex", 1);
		m_program.setUniformValue("v_tex", 2);
	}
	else if (m_pixNeeded == AV_PIX_FMT_RGB24 || m_pixNeeded == AV_PIX_FMT_RGBA)
		m_program.setUniformValue("rgba_tex", 0);
	u_pos = m_program.uniformLocation("draw_pos");
	m_vao.destroy();
	m_vao.create();
	return true;
}

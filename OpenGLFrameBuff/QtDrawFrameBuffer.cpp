#include "QtDrawFrameBuffer.h"

#ifdef __cplusplus
extern "C" {
#endif
#include"libavdevice/avdevice.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif

#include "OpenGLFrameBuff/QtDrawFrameBuffer.h"
#include<QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include<cmath>

#include "Util/logger.h"
#include "Network/Socket.h"
#include "opencv2/opencv.hpp"



#define VERT_SHADER_FRAME_BUFF R"(#version 430 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoords;
uniform mat4 u_pm;
uniform vec4 draw_pos;


out vec2 fragTexCoords;
void main() {
	fragTexCoords = texCoords;
	vec4 p = vec4((0.5 * draw_pos.z) + draw_pos.x + (position.x * 0.5 * draw_pos.z),
                 (0.5 * draw_pos.w) + draw_pos.y + (position.y * 0.5* draw_pos.w),
                 0, 1);

	gl_Position = u_pm*p;
}
)"


#define FRAG_SHADER_FRAME_BUFF R"(#version 430 core
in vec2 fragTexCoords;
out vec4 color;
uniform sampler2D texture1;
void main() {
	color = texture(texture1, fragTexCoords);
}
)"

QtDrawFrameBuffer::QtDrawFrameBuffer(QWidget* parenetWidget):QtGLVideoWidget(parenetWidget)
, m_texFrame(av_frame_alloc())
{
}

QtDrawFrameBuffer::~QtDrawFrameBuffer()
{
	deleteObject();
	av_frame_free(&m_texFrame);
}

void QtDrawFrameBuffer::initializeGL()
{
	//QtGLVideoWidget::initializeGL();
	initializeOpenGLFunctions();
	deleteObject();
	m_fbo = new QOpenGLFramebufferObject(QSize(m_iPreFrameWidth, m_iPreFrameHeight));
	if (m_programFrameBuffer) 
	{
		m_programFrameBuffer->destroyed();
		delete m_programFrameBuffer;
	}
	bool b;
	m_programFrameBuffer = new QOpenGLShaderProgram;
	m_programFrameBuffer->addShaderFromSourceCode(QOpenGLShader::Vertex, VERT_SHADER_FRAME_BUFF);
	m_programFrameBuffer->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SHADER_FRAME_BUFF);
	b = m_programFrameBuffer->link();
	if (!b)
	{
		int linkStatus = 0;
		glGetProgramiv(m_programFrameBuffer->programId(), GL_LINK_STATUS, &linkStatus);
		if (!linkStatus) {
			char infoLog[512];
			glGetProgramInfoLog(m_programFrameBuffer->programId(), 512, NULL, infoLog);
		}
	}

	if (m_vaoFrameBuffer) 
	{
		m_vaoFrameBuffer->destroy();
		delete m_vaoFrameBuffer;
	}
	m_vaoFrameBuffer = new QOpenGLVertexArrayObject;
	m_vaoFrameBuffer->create();

	//glGenFramebuffers(1, &m_framebuffer);
	//// 步骤2: 创建并绑定 color attachment
	//glGenTextures(1, &m_colorBuffer);
	//glBindTexture(GL_TEXTURE_2D, m_colorBuffer);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_iPreFrameWidth, m_iPreFrameHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorBuffer, 0);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glBindTexture(GL_TEXTURE_2D, 0);
	

	//GLfloat vertices[] = {
	//	// Positions // Texture Coords
	//	-1.0f, 1.0f, 0.0f, 1.0f, // Top Left
	//	1.0f, 1.0f, 1.0f, 1.0f, // Top Right
	//	1.0f, -1.0f, 1.0f, 0.0f, // Bottom Right
	//	-1.0f, -1.0f, 0.0f, 0.0f // Bottom Left
	//};


	// 设置顶点数据
	GLfloat vertices[] = {
		// Positions // Texture Coords
		-1.0f, 1.0f, 0.0f, 1.0f, // Top Left
		1.0f, 1.0f, 1.0f, 1.0f, // Top Right
		1.0f, -1.0f, 1.0f, 0.0f, // Bottom Right
		-1.0f, -1.0f, 0.0f, 0.0f // Bottom Left
	};

	GLuint indices[] = {
	3, 2, 1, 0
	};

	m_vaoFrameBuffer->bind();
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_EBO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Positions
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Texture Coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	m_vaoFrameBuffer->release();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	//glGenBuffers(1, &m_downloadPboId);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER, m_downloadPboId);
	//glBufferData(GL_PIXEL_PACK_BUFFER, m_iPreFrameWidth* m_iPreFrameHeight*3, nullptr, GL_STREAM_DRAW);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER,0);

	
	if (!m_texFrame) 
	{
		if (!(m_texFrame = av_frame_alloc())) 
		{
			PrintE("av_frame_alloc error:no memory");
			return;
		}
	}
	else
		av_frame_unref(m_texFrame);

	m_texFrame->width = m_iPreFrameWidth;
	m_texFrame->height = m_iPreFrameHeight;
	m_texFrame->format = AV_PIX_FMT_RGB24;
	m_texFrame->sample_aspect_ratio = { 1,1 };
	int ret;
	if ((ret = av_frame_get_buffer(m_texFrame, 1)) != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return;
	}
	if ((ret = av_frame_make_writable(m_texFrame)) != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_make_writable error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return;
	}

}

void QtDrawFrameBuffer::paintGL()
{
	drawFrameBuffer();

	int iw = width();
	int ih = height();

	QMatrix4x4 m;
	m.ortho(0, iw, 0.0,ih,  0.0, 100.0f);
	m_programFrameBuffer->bind();
	m_programFrameBuffer->setUniformValue("u_pm", m);
	QRect rect = calculate_display_rect(0, 0, iw, ih, m_iPreFrameWidth, m_iPreFrameHeight, { 1,1 });
	int pos = m_programFrameBuffer->uniformLocation("draw_pos");
	glUniform4f(pos, rect.left(), rect.top(), rect.width(), rect.height());
	m_programFrameBuffer->setUniformValue("texture1", 0);

	//m_programFrameBuffer->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);




	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
	m_vaoFrameBuffer->bind();


	glViewport(0, 0, iw, ih);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	m_vaoFrameBuffer->release();
	glBindTexture(GL_TEXTURE_2D, 0);
	m_programFrameBuffer->release();
}

void QtDrawFrameBuffer::resizeGL(int width, int height)
{
	QOpenGLWidget::resizeGL(width, height);
}

void QtDrawFrameBuffer::drawFrameBuffer()
{
	if (!m_frameSave)
	{
		m_fbo->bind();
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_fbo->release();
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
	QRect rect(0, 0, m_frame_tmp->width, m_frame_tmp->height);
	if (!m_isOpenGLInit)
	{
		setFrameSize(m_frame_tmp->width, m_frame_tmp->height);
		initializeGL();
		initializeTextures();
		m_isOpenGLInit = true;
	}
	
	QMatrix4x4 m;
	m.ortho(0, m_frameSave->width, m_frameSave->height,0.0,  0.0, 100.0f);
	m_fbo->bind();
	m_programFrameBuffer->bind();
	m_programFrameBuffer->setUniformValue("u_pm", m);
	int il = rect.left();
	int it = rect.top();
	int itmpw = rect.width();
	int itmph = rect.height();

	m_programFrameBuffer->setUniformValue("draw_pos", rect.left(), rect.top(), rect.width(), rect.height());
	//glUniform4f(u_pos, rect.left(), rect.top(), rect.width(), rect.height());

	
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//m_programFrameBuffer->bind();
	m_vaoFrameBuffer->bind();

	switch ((AVPixelFormat)m_frame_tmp->format)
	{
	case AV_PIX_FMT_RGB24:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureRGB->textureId());
		if (bUploadTex)
		{
			m_programFrameBuffer->setUniformValue("texture1", 0);
			setRGBPixels(m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
		}
		break;
	case AV_PIX_FMT_RGBA:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureRGBA->textureId());
		if (bUploadTex)
		{
			m_programFrameBuffer->setUniformValue("texture1", 0);
			setRGBAPixels(m_frame_tmp->data[0], m_frame_tmp->linesize[0]);
		}
		break;
	default:
		break;
	}

	
	glViewport(0, 0, m_iPreFrameWidth, m_iPreFrameHeight);

	//glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, 0);

	m_programFrameBuffer->release();
	m_vaoFrameBuffer->release();

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{
		//QImage image = QImage(m_fbo->size(), QImage::Format_RGB888);
		//glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());
		//image.save(R"(F:\output0123456.png)");
		//glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, dataSize,GL_MAP_READ_BIT);
		{
			//glPixelStorei(GL_PACK_ALIGNMENT, 1); // 强制1字节对齐
			//glPixelStorei(GL_PACK_ROW_LENGTH, 0); // 使用紧密打包
			glPixelStorei(GL_PACK_ROW_LENGTH, m_fbo->width());
			std::lock_guard<std::mutex>lock(m_mutex);
			
			glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, m_texFrame->data[0]);
			//cv::Mat mat(m_fbo->height(), m_fbo->width(), CV_8UC3);
			//glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, mat.data);
			//cv::flip(mat, mat, 0);
			//cv::imwrite("F:\\output012345678.jpg", mat);
		}
		

		//QImage image = QImage(m_fbo->size(), QImage::Format_RGB888);
		//glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());
		//image.save(R"(F:\output01234567.png)");
	}
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//image = QImage(m_fbo->size(), QImage::Format_RGB888);
	//glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());
	
	m_fbo->release();

	//image.mirrored();
	//if(!image.isNull())
	//	image.save(R"(F:\output0123456.png)");
}

void QtDrawFrameBuffer::deleteObject()
{

	if (m_fbo) 
	{
		delete m_fbo;
		m_fbo = NULL;
	}

	if (m_programFrameBuffer)
	{
		m_programFrameBuffer->removeAllShaders();
		m_programFrameBuffer->destroyed();
		delete m_programFrameBuffer;
		m_programFrameBuffer = NULL;
	}

	if (m_vaoFrameBuffer) 
	{
		m_vaoFrameBuffer->destroy();
		delete m_vaoFrameBuffer;
		m_vaoFrameBuffer = NULL;
	}

	if (m_VBO) 
		glDeleteBuffers(1, &m_VBO);
	if (m_EBO)
		glDeleteBuffers(1, &m_EBO);

}

#include "QtResizeDrawFrameBuffer.h"
#ifdef __cplusplus
extern "C" {
#endif
#include"libavdevice/avdevice.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <cmath>
#include <QPoint>
#include "Util/logger.h"
#include "Network/Socket.h"
#include <QMouseEvent>
#include <functional>
#include <algorithm>
#include <numeric>
#include "SourceType/GetSourceType.h"
#include <qopenglextrafunctions.h>


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
	
	vec4 pos_tmp = u_pm*p;
	gl_Position = pos_tmp;
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


#define VERT_SHADER_DRAW_TEXT R"(#version 430 core
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
	vec4 pos_tmp = u_pm*p;
	gl_Position = pos_tmp;
}
)"

#define FRAG_SHADER_DRAW_TEXT R"(#version 430 core
in vec2 fragTexCoords;
out vec4 color;
uniform sampler2D textureText;
uniform sampler2D textureDesktop;
void main() {
	vec4 color_texDesktop = texture(textureDesktop, fragTexCoords);
	vec4 color_texText = texture(textureText, fragTexCoords);
	//color = color_texText;
	color = mix(color_texDesktop, color_texText, color_texText.a);
	color.a = 1.0f;
}
)"


QtResizeDrawFrameBuffer::QtResizeDrawFrameBuffer(QWidget* parenetWidget):ResizeGLWidget(parenetWidget)
, m_texFrame_RGB(av_frame_alloc())
, m_texFrame_YUV420P(av_frame_alloc())
//, m_roateMat(vmath::rotate(45.0f, 0.0f,1.0f, 0.0f))
{
	setMouseTracking(true);
	//m_vecText.reserve(10);

	//m_offscreenRenderThread;
	
}

QtResizeDrawFrameBuffer::~QtResizeDrawFrameBuffer()
{
	makeCurrent();
	deleteObject();
	doneCurrent();

	//av_frame_free(&m_texFrame);
}

void QtResizeDrawFrameBuffer::renderFrame(UPTR_FME f)
{
	ResizeGLWidget::setFrame(&*f);
	QtGLVideoWidget::setFrame(std::move(f));
	makeCurrent();
	{
		std::lock_guard<std::mutex>lock(m_mutex_fbo);
		drawFrame();
	}
	doneCurrent();
	update();
}

void QtResizeDrawFrameBuffer::initializeGL()
{
	QtGLVideoWidget::initializeGL();
	m_glFuncs = QOpenGLContext::currentContext()->extraFunctions();
	if (!m_glFuncs) {
		qCritical() << "Failed to get extra functions";
		return;
	}
	deleteObject();
	m_fbo = new QOpenGLFramebufferObject(QSize(m_iPreFrameWidth, m_iPreFrameHeight));

	//m_fbo->setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	GLuint fboTex = m_fbo->texture();
	
	glBindTexture(GL_TEXTURE_2D, fboTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//m_fbo->setTextureTarget(GL_TEXTURE_2D);
	//m_fbo->setTextureMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
	

	bool b;
	m_programFrameBuffer = new QOpenGLShaderProgram;
	m_programFrameBuffer->addShaderFromSourceCode(QOpenGLShader::Vertex, VERT_SHADER_FRAME_BUFF);
	m_programFrameBuffer->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SHADER_FRAME_BUFF);
	b = m_programFrameBuffer->link();
	if (!b)
	{
		int linkStatus = 0;
		glGetProgramiv(m_program.programId(), GL_LINK_STATUS, &linkStatus);
		if (!linkStatus) {
			char infoLog[512];
			glGetProgramInfoLog(m_program.programId(), 512, NULL, infoLog);
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
	

	GLfloat vertices[] = {
		// Positions // Texture Coords
		-1.0f, 1.0f, 0.0f, 1.0f, // Top Left
		1.0f, 1.0f, 1.0f, 1.0f, // Top Right
		1.0f, -1.0f, 1.0f, 0.0f, // Bottom Right
		-1.0f, -1.0f, 0.0f, 0.0f // Bottom Left
	};



	 //设置顶点数据
	//GLfloat vertices[] = {
	//	// Positions // Texture Coords
	//	-1.0f, 1.0f, 1.0f, 0.0f, // Top Left
	//	1.0f, 1.0f, 0.0f, 0.0f, // Top Right
	//	1.0f, -1.0f, 0.0f, 1.0f, // Bottom Right
	//	-1.0f, -1.0f, 1.0f, 1.0f // Bottom Left
	//};


	GLuint indices[] = {
	3, 0,1,2
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

	/////////////////////////////////////////////////////////////////////////////

	m_programDrawText = new QOpenGLShaderProgram;
	m_programDrawText->addShaderFromSourceCode(QOpenGLShader::Vertex, VERT_SHADER_DRAW_TEXT);
	m_programDrawText->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAG_SHADER_DRAW_TEXT);
	b = m_programDrawText->link();
	if (!b)
	{
		int linkStatus = 0;
		glGetProgramiv(m_programDrawText->programId(), GL_LINK_STATUS, &linkStatus);
		if (!linkStatus) {
			char infoLog[512];
			glGetProgramInfoLog(m_programDrawText->programId(), 512, NULL, infoLog);
		}
	}

	m_vaoDrawText = new QOpenGLVertexArrayObject;
	m_vaoDrawText->create();
	m_vaoDrawText->bind();
	glGenBuffers(1, &m_VBO_DrawText);
	glGenBuffers(1, &m_EBO_DrawText);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO_DrawText);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO_DrawText);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Positions
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Texture Coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	m_vaoDrawText->release();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	glGenBuffers(2, m_pboIds);


	for (int i = 0; i < 2; i++) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboIds[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_iPreFrameWidth * m_iPreFrameHeight * 3, nullptr, GL_STREAM_READ);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	//m_TextTex2DArray = new QOpenGLTexture(QOpenGLTexture::Target::Target2DArray);


	
	m_offscreenSurface = new QOffscreenSurface();
	m_offscreenSurface->setFormat(context()->format());
	m_offscreenSurface->create();
	
	m_sharedContext = new QOpenGLContext();
	m_sharedContext->setFormat(context()->format());
	m_sharedContext->setShareContext(context());
	m_sharedContext->create();



	if (!m_texFrame_RGB) 
	{
		m_texFrame_RGB.reset(av_frame_alloc());
		if (m_texFrame_RGB)
		{
			PrintE("av_frame_alloc error:no memory");
			return;
		}
	}
	else
		av_frame_unref(m_texFrame_RGB.get());

	if (!m_texFrame_YUV420P) 
	{
		m_texFrame_YUV420P.reset(av_frame_alloc());
		if (m_texFrame_YUV420P)
		{
			PrintE("av_frame_alloc error:no memory");
			return;
		}
	}
	else
		av_frame_unref(m_texFrame_YUV420P.get());


	/*m_texFrame->width = m_iPreFrameWidth;
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
	}*/
}

void QtResizeDrawFrameBuffer::paintGL()
{
	drawFrame();

	m_programFrameBuffer->bind();
	int iw = width();
	int ih = height();

	QMatrix4x4 m;
	m.ortho(0, iw, 0.0, ih, 0.0, 100.0f);

	m_programFrameBuffer->setUniformValue("u_pm", m);
	//int model_mat_index = m_programFrameBuffer->uniformLocation("model_mat");
	//glUniformMatrix4fv(model_mat_index, 1, GL_FALSE, m_roateMat);
	//QRect rect = calculate_display_rect(0, 0, iw, ih, m_iPreFrameWidth, m_iPreFrameHeight, { 1,1 });
	QRect rect(0, 0, iw, ih);
	int pos = m_programFrameBuffer->uniformLocation("draw_pos");
	
	glUniform4f(pos, rect.left(), rect.top(), rect.width(), rect.height());
	glClearColor(0.0f, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_programFrameBuffer->setUniformValue("texture1", 0);

	glActiveTexture(GL_TEXTURE0);

	{
		std::lock_guard<std::mutex>lock(m_mutex_fbo);
		glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
		m_vaoFrameBuffer->bind();
		glViewport(0, 0, iw, ih);
		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, 0);
	}

	m_vaoFrameBuffer->release();
	glBindTexture(GL_TEXTURE_2D, 0);
	m_programFrameBuffer->release();
}

void QtResizeDrawFrameBuffer::pushDrawImageComponent(std::shared_ptr<CalcGLDraw>DrawImageComponent)
{
	//auto ptr = std::make_shared<GLDrawText>(m_iPreFrameWidth, m_iPreFrameHeight, textImage);
	auto iter = std::find(m_vecDrawComponent.begin(), m_vecDrawComponent.end(), DrawImageComponent);
	if (iter == m_vecDrawComponent.end())
	{
		//textImage->m_w = m_iPreFrameWidth;
		//textImage->m_h = m_iPreFrameHeight;
		//textImage->init();
		m_vecDrawComponent.push_back(DrawImageComponent);
	}
	//m_vecText.push_back(textImage);
	//return ptr;
	//m_vecText.back().setPosition(0, 0);
	//m_bInitTextTex2DArray = true;
}
void QtResizeDrawFrameBuffer::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		ResizeGLWidget::mousePressEvent(event);
		return;
	}
	QPoint clickPt = event->pos();
	//float x = clickPt.x();

	int widWidth = width();
	int widHeight = height();


	glm::mat4 viewMat(1.0f);
	glm::mat4 proMat = glm::ortho(0.0f, (float)widWidth, 0.0f,(float)widHeight, 0.0f, 100.0f);

	//m.ortho(0, ele.m_image.cols, ele.m_image.rows, 0, 0.0, 100.0f);

	auto p = getMouseRayGLDrawText(widWidth, widHeight, clickPt.x(), clickPt.y(), viewMat, proMat);

	if (p) 
	{
		m_pCalcGLDraw = p;
		//m_move_point = event->globalPos() - QPoint(m_pGLDrawText->m_x, m_pGLDrawText->m_y);
		m_move_point = clickPt - QPoint((float)m_pCalcGLDraw->m_xCol/m_iPreFrameWidth * width(), (float)m_pCalcGLDraw->m_yRow/ m_iPreFrameHeight *height());
		m_mouse_press = true;
		
	}
	else 
	{
		m_mouse_press = false;
		m_pCalcGLDraw = NULL;
	}

}

void QtResizeDrawFrameBuffer::mouseReleaseEvent(QMouseEvent*)
{
	m_mouse_press = false;
	m_pCalcGLDraw = NULL;
}

void QtResizeDrawFrameBuffer::mouseMoveEvent(QMouseEvent* event)
{
	if (m_mouse_press)
	{
		QPoint move_pos = event->pos();
		//qDebug() << move_pos<<"  x:="<< move_pos.x()<<"  y="<< move_pos.y();
		QPoint pt(move_pos - m_move_point);
		m_pCalcGLDraw->m_xCol = (float)pt.x()/width()*m_iPreFrameWidth;
		m_pCalcGLDraw->m_yRow = (float)pt.y()/height()* m_iPreFrameHeight;
		//m_move_point = event->pos() - QPoint(m_pGLDrawText->m_x, m_pGLDrawText->m_y);
		update();
	}
}

UPTR_FME QtResizeDrawFrameBuffer::getFrameYUV420P()
{
	std::unique_lock<std::mutex>lock(m_mutex_m_texFrame_RGB);
	if (m_bIsConverted)
		return UPTR_FME(av_frame_clone(m_texFrame_YUV420P.get()));
	if (m_convertRGB2yuv420p.setInfo(m_texFrame_RGB->width, m_texFrame_RGB->height, 24))
	{
		if (m_convertRGB2yuv420p(m_texFrame_RGB.get(), m_texFrame_YUV420P.get()))
		{
			m_bIsConverted = true;
			return UPTR_FME(av_frame_clone(m_texFrame_YUV420P.get()));
		}
	}
	return NULL;
}


//void QtResizeDrawFrameBuffer::resizeGL(int width, int height)
//{
//	ResizeGLWidget::resizeGL(width, height);
//}

void QtResizeDrawFrameBuffer::drawFrame()
{
	drawFrameBuffer();
	drawImageComponent();
	writeRGBImage();
}

//void QtResizeDrawFrameBuffer::GenerateTextImageTex2DArray()
//{
//	int isize = m_vecText.size();
//	m_textTexCoordVec.clear();
//	m_textTexCoordVec.reserve(isize);
//	
//	m_TextTex2DArray->setLayers(isize);
//	//m_TextTex2DArray->setDe
//	for (int i = 0; i < isize; ++i) 
//	{
//		cv::Mat image = m_vecText[i].getImage();
//		m_TextTex2DArray->setData(0, 0, 0, image.cols, image.rows, 0, 0, i, QOpenGLTexture::PixelFormat::BGRA, QOpenGLTexture::PixelType::UInt32_RGBA8, image.data);
//		m_textTexCoordVec.emplace_back(m_vecText[i].m_vec);
//	}
//}

void QtResizeDrawFrameBuffer::drawFrameBuffer()
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
	QRect rect(0, 0, m_iPreFrameWidth, m_iPreFrameHeight);
	if (!m_isOpenGLInit)
	{
		initializeGL();
		initializeTextures();
		m_isOpenGLInit = true;
	}

	//glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	m_fbo->bind();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_program.bind();
	m_vao.bind();


	QMatrix4x4 m;
	//m.ortho(0, m_iPreFrameWidth, m_iPreFrameHeight, 0.0, 0.0, 100.0f);
	m.ortho(0, m_iPreFrameWidth, m_iPreFrameHeight, 0.0, 0.0, 100.0f);
	m_program.setUniformValue("u_pm", m);
	//int il = rect.left();
	//int it = rect.top();
	//int itmpw = rect.width();
	//int itmph = rect.height();
	glUniform4f(u_pos, rect.left(), rect.top(), rect.width(), rect.height());

	//switch (m_pixNeeded)
	switch((AVPixelFormat)m_frame_tmp->format)
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

	glViewport(0, 0, m_iPreFrameWidth, m_iPreFrameHeight);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	m_program.release();
	m_vao.release();

	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	//{
		/*QImage image = QImage(m_fbo->size(), QImage::Format_RGB888);
		glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());
		image.save(R"(F:\output012345678.png)");*/
		//std::lock_guard<std::mutex>lock(m_mutex);
		//glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, m_texFrame->data[0]);
	//}
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//image = QImage(m_fbo->size(), QImage::Format_RGB888);
	//glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());
	m_fbo->release();
	//image.mirrored();
	//if(!image.isNull())
	//	image.save(R"(F:\output0123456.png)");
}

//void QtResizeDrawFrameBuffer::DrawComponent(std::shared_ptr<CalcGLDraw> obj)
//{
//	if (obj->isRemoved())
//		return;
//	
//	switch (obj->type()) 
//	{
//	case CSourceType::ST_FREE_TYPE_TEXT:
//		DrawComponent(std::static_pointer_cast<GetSourceType_t<CSourceType::ST_FREE_TYPE_TEXT>>(obj));
//		break;
//	case CSourceType::ST_PIC:
//		break;
//	case CSourceType::ST_MEDIA:
//		break;
//	case CSourceType::ST_CAMERA:
//		DrawComponent(std::static_pointer_cast<GetSourceType_t<CSourceType::ST_CAMERA>>(obj));
//		break;
//	default:
//		break;
//	}
//}

void QtResizeDrawFrameBuffer::drawCalcDraw(std::shared_ptr<CalcGLDraw> calcDraw)
{
	int wImg = calcDraw->m_tex->width();
	int hImg = calcDraw->m_tex->height();
	if (wImg <= 1 || hImg <= 0)
		return;

	m_fbo->bind();
	int locDrawPos = m_programDrawText->uniformLocation("draw_pos");
	m_programDrawText->bind();
	m_vaoDrawText->bind();
	QOpenGLTexture* desktopTex = new QOpenGLTexture(QOpenGLTexture::Target::Target2D);
	QImage qimg(wImg, hImg, QImage::Format::Format_RGB888);
	glReadPixels(calcDraw->m_xCol, m_iPreFrameHeight - calcDraw->m_yRow - hImg, wImg, hImg, GL_RGB, GL_UNSIGNED_BYTE, qimg.bits());
	desktopTex->create();
	desktopTex->setData(qimg);
	QMatrix4x4 m;
	//m.ortho(0, m_iPreFrameWidth, m_iPreFrameHeight, 0.0, 0.0, 100.0f);
	//m.ortho(ele.m_x, ele.m_x+ele.m_w, ele.m_y + ele.m_h, ele.m_y, 0.0, 100.0f);
	m.ortho(0, wImg, 0, hImg, 0.0, 100.0f);
	m_programDrawText->setUniformValue("u_pm", m);
	QRect rect(0, 0, wImg, hImg);

	glUniform4f(locDrawPos, rect.left(), rect.top(), rect.width(), rect.height());

	m_programDrawText->setUniformValue("textureText", 0);
	m_programDrawText->setUniformValue("textureDesktop", 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, calcDraw->m_tex->textureId());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, desktopTex->textureId());

	//glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
	//glViewport(0, 0, m_iPreFrameWidth, m_iPreFrameHeight);
	//glViewport(ele.m_x, m_iPreFrameHeight - ele.m_h - ele.m_y, ele.m_w, ele.m_h);

	glViewport(calcDraw->m_xCol, m_iPreFrameHeight - hImg - calcDraw->m_yRow, wImg, hImg);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE0, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	m_programDrawText->release();
	m_vaoDrawText->release();
	m_fbo->release();
	desktopTex->destroy();
	delete desktopTex;
}

void QtResizeDrawFrameBuffer::drawImageComponent()
{
	if (m_vecDrawComponent.empty())
		return;

	/*if (m_bInitTextTex2DArray)
	{
		GenerateTextImageTex2DArray();
		m_bInitTextTex2DArray = false;
	}*/

	m_fbo->bind();
	int locDrawPos = m_programDrawText->uniformLocation("draw_pos");
	m_programDrawText->bind();
	m_vaoDrawText->bind();

	QOpenGLTexture* desktopTex = new QOpenGLTexture(QOpenGLTexture::Target::Target2D);
	//desktopTex->allocateStorage(QOpenGLTexture::PixelFormat::RGBA, QOpenGLTexture::PixelType::UInt8);

	bool bHasRmoved = false;
	for (auto& ele : m_vecDrawComponent)
	{
		if (ele->isRemoved())
		{
			bHasRmoved = true;
			continue;
		}
		//cv::Mat img = ele->image();
		int w = ele->m_wImg * ele->m_fScale;
		int h = ele->m_hImg * ele->m_fScale;
		if (w <= 1 || h <= 1)
			continue;
		/*if (img.empty())
			continue;*/
		//拷贝desktop texture
		//glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
		//desktopTex->setData(0, 0, 0, ele.m_w, ele.m_h, 0, 0, QOpenGLTexture::PixelFormat::RGBA, QOpenGLTexture::PixelType::UInt8, NULL);
		//desktopTex->bind();
		//glBindTexture(GL_TEXTURE_2D, desktopTex->textureId());
		//glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ele.m_x, m_iPreFrameHeight - ele.m_y - ele.m_h, ele.m_w, ele.m_h);
		//glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ele.m_x, m_iPreFrameHeight - ele.m_y - ele.m_image.rows, ele.m_image.cols, ele.m_image.rows);
		//desktopTex->release();
		QImage qimg(w, h, QImage::Format::Format_RGB888);
		glPixelStorei(GL_PACK_ROW_LENGTH, w);
		glReadPixels(ele->m_xCol, m_iPreFrameHeight - ele->m_yRow - h, w, h, GL_RGB, GL_UNSIGNED_BYTE, qimg.bits());
		if(desktopTex->isCreated())
			desktopTex->destroy();
		desktopTex->create();
		//glBindTexture(GL_TEXTURE_2D, 0);
		desktopTex->setData(qimg);

		QMatrix4x4 m;
		//m.ortho(0, m_iPreFrameWidth, m_iPreFrameHeight, 0.0, 0.0, 100.0f);
		//m.ortho(ele.m_x, ele.m_x+ele.m_w, ele.m_y + ele.m_h, ele.m_y, 0.0, 100.0f);
		m.ortho(0, w, 0, h, 0.0, 100.0f);
		m_programDrawText->setUniformValue("u_pm", m);
		//QRect rect(ele.m_x, ele.m_y, ele.m_image.cols, ele.m_image.rows);
		QRect rect(0, 0, w, h);
		/*
		int il = rect.left();
		int it = rect.top();
		int itmpw = rect.width();
		int itmph = rect.height();
		*/
		glUniform4f(locDrawPos, rect.left(), rect.top(), rect.width(), rect.height());
		//GLuint arrid = m_TextTex2DArray->textureId();

		m_programDrawText->setUniformValue("textureText", 0);
		m_programDrawText->setUniformValue("textureDesktop", 1);

		ele->setData();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ele->m_tex->textureId());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, desktopTex->textureId());

		//glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
		//glViewport(0, 0, m_iPreFrameWidth, m_iPreFrameHeight);
		//glViewport(ele.m_x, m_iPreFrameHeight - ele.m_h - ele.m_y, ele.m_w, ele.m_h);

		glViewport(ele->m_xCol, m_iPreFrameHeight - h - ele->m_yRow, w, h);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);

	}
	m_programDrawText->release();
	m_vaoDrawText->release();

	if (bHasRmoved) 
	{
		m_vecDrawComponent.erase(std::remove_if(m_vecDrawComponent.begin(), m_vecDrawComponent.end(), [](const decltype(m_vecDrawComponent)::value_type&v)
			{
				return v->isRemoved();
			}), m_vecDrawComponent.end());
	}

	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	//{
	//	/*QImage image = QImage(m_fbo->size(), QImage::Format_RGB888);
	//	glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());
	//	image.save(R"(F:\output0123456789.png)");*/
	//	//std::lock_guard<std::mutex>lock(m_mutex);
	//	int wfbo = m_fbo->width();
	//	int hfbo = m_fbo->height();
	//	
	//	std::lock_guard<std::mutex>lock(m_mutex_m_texFrame_RGB);
	//	if (m_texFrame_RGB->width != wfbo || m_texFrame_RGB->height != hfbo)
	//	{
	//		av_frame_unref(m_texFrame_RGB.get());
	//		m_texFrame_RGB->width = wfbo;
	//		m_texFrame_RGB->height = hfbo;
	//		m_texFrame_RGB->format = AV_PIX_FMT_RGB24;
	//		m_texFrame_RGB->sample_aspect_ratio = {1,1};
	//		int ret;
	//		if ((ret = av_frame_get_buffer(m_texFrame_RGB.get(), 1)) != 0)
	//		{
	//			char szErr[AV_ERROR_MAX_STRING_SIZE];
	//			PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	//			return;
	//		}
	//		if ((ret = av_frame_make_writable(m_texFrame_RGB.get())) != 0)
	//		{
	//			char szErr[AV_ERROR_MAX_STRING_SIZE];
	//			PrintE("av_frame_make_writable error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	//			return;
	//		}
	//	}
	//	glPixelStorei(GL_PACK_ROW_LENGTH, wfbo);
	//	glReadPixels(0, 0, wfbo, hfbo, GL_RGB, GL_UNSIGNED_BYTE, m_texFrame_RGB->data[0]);
	//	m_bIsConverted = false;
	//}
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//image = QImage(m_fbo->size(), QImage::Format_RGB888);
	//glReadPixels(0, 0, m_fbo->width(), m_fbo->height(), GL_RGB, GL_UNSIGNED_BYTE, image.bits());
	m_fbo->release();
	desktopTex->destroy();
	delete desktopTex;
}

void QtResizeDrawFrameBuffer::writeRGBImage()
{
	m_fbo->bind();
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
	{
		int wfbo = m_fbo->width();
		int hfbo = m_fbo->height();
		
		cv::Mat mat,dst;// (cv::Size(wfbo, hfbo), CV_8UC3), dst;
		/*
		glPixelStorei(GL_PACK_ROW_LENGTH, wfbo);
		glReadPixels(0, 0, wfbo, hfbo, GL_RGB, GL_UNSIGNED_BYTE, mat.data);
		cv::flip(mat, dst, 0);
		*/


		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboIds[m_pboIndex]);
		// 异步读取 - 立即返回
		glReadPixels(0, 0, m_fbo->width(), m_fbo->height(),
			GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		// 绑定下一个PBO（准备下一帧）
		int nextPboIndex = (m_pboIndex + 1) % 2;
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pboIds[nextPboIndex]);

		// 映射上一个PBO的数据（非阻塞）
		void* mappedData = m_glFuncs->glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_iPreFrameWidth* m_iPreFrameHeight*3,
			GL_MAP_READ_BIT);

		if (mappedData) {
			// 处理数据（注意：这是上一帧的数据）
			//cv::Mat raw(m_fbo->height(), m_fbo->width(), CV_8UC3, mappedData);
			mat = cv::Mat(cv::Size(wfbo, hfbo), CV_8UC3, mappedData);
			cv::Mat corrected;
			cv::flip(mat, dst, 0);  // 垂直翻转
			m_glFuncs->glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		// 更新PBO索引
		m_pboIndex = nextPboIndex;
		if (dst.empty())
			return;

		std::lock_guard<std::mutex>lock(m_mutex_m_texFrame_RGB);
		if (m_texFrame_RGB->width != wfbo || m_texFrame_RGB->height != hfbo)
		{
			av_frame_unref(m_texFrame_RGB.get());
			m_texFrame_RGB->width = wfbo;
			m_texFrame_RGB->height = hfbo;
			m_texFrame_RGB->format = AV_PIX_FMT_RGB24;
			m_texFrame_RGB->sample_aspect_ratio = { 1,1 };
			int ret;
			if ((ret = av_frame_get_buffer(m_texFrame_RGB.get(), 1)) != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return;
			}
			if ((ret = av_frame_make_writable(m_texFrame_RGB.get())) != 0)
			{
				char szErr[AV_ERROR_MAX_STRING_SIZE];
				PrintE("av_frame_make_writable error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
				return;
			}
		}
		memmove(m_texFrame_RGB->data[0], dst.data, wfbo * hfbo * 3);
		m_bIsConverted = false;
	}
	m_fbo->release();
}

void QtResizeDrawFrameBuffer::deleteObject()
{
	//makeCurrent();
	//makeCurrent();
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
	//doneCurrent();

	if (m_programDrawText)
	{
		m_programDrawText->removeAllShaders();
		m_programDrawText->destroyed();
		delete m_programDrawText;
		m_programDrawText = NULL;
	}

	if (m_vaoDrawText)
	{
		m_vaoDrawText->destroy();
		delete m_vaoDrawText;
		m_vaoDrawText = NULL;
	}

	if (m_VBO_DrawText)
		glDeleteBuffers(1, &m_VBO_DrawText);
	if (m_EBO_DrawText)
		glDeleteBuffers(1, &m_EBO_DrawText);

	for (int i = 0; i < sizeof(m_pboIds) / sizeof(std::remove_extent<decltype(m_pboIds)>::type); ++i) 
		if(m_pboIds[i])
			glDeleteBuffers(1, &m_pboIds[i]);

	m_pboIndex = 0;
	/*if (m_TextTex2DArray)
	{
		m_TextTex2DArray->destroy();
		delete m_TextTex2DArray;
		m_TextTex2DArray = NULL;
	}*/
	if (m_offscreenSurface)
	{
		m_offscreenSurface->destroy();
		delete m_offscreenSurface;
		m_offscreenSurface = nullptr;
	}
	if (m_sharedContext) {
		m_sharedContext->destroyed();
		delete m_sharedContext;
		m_sharedContext = nullptr;
	}
}


// 将鼠标坐标转换为世界坐标中的射线
std::shared_ptr<CalcGLDraw> QtResizeDrawFrameBuffer::getMouseRayGLDrawText(int width, int height, double xpos, double ypos, glm::mat4 view, glm::mat4 projection) {
	//int width, height;
	//glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

	// 归一化设备坐标
	float x = (2.0f * xpos) / width - 1.0f;
	float y = 1.0f - (2.0f * ypos) / height;
	float z = 0.0f;//1.0f;


	/////////test////////////////////////////////////
	//right top
	/*glm::vec4 right_top = projection* glm::vec4(width, height, 0.0, 1.0);
	glm::vec4 left_bottom = projection * glm::vec4(0.0, 0.0, 0.0, 1.0);

	auto right_top_src = glm::inverse(projection)* right_top;
	auto left_bottomp_src = glm::inverse(projection) * left_bottom;*/
	//glm::vec4 left_top = projection * glm::vec4(-1.0, 1.0, 0.0, 1.0);
	//glm::vec4 right_bottom = projection * glm::vec4(1.0, -1.0, 0.0, 1.0);

	/////////////////////////////////////
	glm::vec3 ray_nds = glm::vec3(x, y, z);
	glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, 0.0f/*-1.0*/, 1.0);

	glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
	//ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
	//ray_eye = glm::vec4(ray_eye.x, ray_eye.y, 0.0, 0.0);

	glm::vec2 ray_wor = glm::vec2(glm::inverse(view) * ray_eye);

	ray_wor.y = height - ray_wor.y;
	//glm::vec2  ray_wor_xy = glm::vec2(ray_wor);
	//ray_wor_xy = glm::normalize(ray_wor_xy);

	//此刻应该是投影到m_iPreFrameWidth  m_iPreFrameHeight上
	QPointF ptr(((float)ray_wor.x/width)*m_iPreFrameWidth, ((float)ray_wor.y/height) * m_iPreFrameHeight);

	int isize = m_vecDrawComponent.size();


	decltype(m_vecDrawComponent) vecHitComponent;
	vecHitComponent.reserve(isize);
	for (int i = isize - 1; i >=0; --i)
	{
		//cv::Mat img = m_vecText[i]->image();
		QRectF rect(m_vecDrawComponent[i]->m_xCol, m_vecDrawComponent[i]->m_yRow, m_vecDrawComponent[i]->m_wImg* m_vecDrawComponent[i]->m_fScale, m_vecDrawComponent[i]->m_hImg* m_vecDrawComponent[i]->m_fScale);
		if (rect.contains(ptr)) 
		{
			vecHitComponent.push_back(m_vecDrawComponent[i]);
			//return m_vecDrawComponent[i];
		}
	}

	if(vecHitComponent.empty())
		return NULL;


	//按照type类型优先级
	std::shared_ptr<CalcGLDraw>p = vecHitComponent.front();

	for (int i = 1; i < vecHitComponent.size(); ++i)
		if (vecHitComponent[i]->type() < p->type())
			p = vecHitComponent[i];
	return p;
}

void QtResizeDrawFrameBuffer::offscreenRenderThreadFun()
{
	cond


}




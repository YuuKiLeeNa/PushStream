#pragma once
#include "PushStream/QtGLVideoWidget.h"
#include<QtOpenGL/qopenglbuffer.h>

#include<mutex>

class QOpenGLFramebufferObject;

struct AVFrame;


class QtDrawFrameBuffer:public QtGLVideoWidget
{
	//Q_OBJECT
public:
	QtDrawFrameBuffer(QWidget* parenetWidget = NULL);
	virtual ~QtDrawFrameBuffer();

	//using QtGLVideoWidget::setFrameSize;
	//using QtGLVideoWidget::renderFrame;

public:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;

protected:
	//void initializeParam();
	void drawFrameBuffer();
	void deleteObject();

protected:

	//GLuint m_framebuffer = 0;
	//GLuint m_colorBuffer = 0;

	QOpenGLFramebufferObject* m_fbo = NULL;
	QOpenGLShaderProgram *m_programFrameBuffer = NULL;
	QOpenGLVertexArrayObject* m_vaoFrameBuffer = NULL;
	GLuint m_VBO = 0;
	GLuint m_EBO = 0;
	AVFrame* m_texFrame = NULL;


	std::mutex m_mutex;

};

#pragma once
#include "PushStream/ResizeGLWidget.h"
#include<QtOpenGL/qopenglbuffer.h>
#include<mutex>
#include "OpenGLFrameBuff/vmath.h"
#include "DrawTextImage/GenTextImage.h"
#include "OpenGLFrameBuff/GLDrawText.h"
#include<vector>
#include <QOpenGLTexture>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<memory>
#include "PushStream/define_type.h"
#include "PushStream/PicToYuv.h"
#include <QOpenGLExtraFunctions>
#include "define_type.h"
#include<thread>
#include<condition_variable>
#include<memory>
#include<atomic>


template<typename BOOL>
struct SFINE_TRUE_TYPE;

template<>
struct SFINE_TRUE_TYPE<std::true_type>
{
	using value_type = std::true_type;
};

template<typename Base, typename Derived>
using SFINE_IS_BASE_OF_T = typename SFINE_TRUE_TYPE<typename std::is_base_of<Base, Derived>::value_type>::value_type;



class QOpenGLFramebufferObject;


class QtResizeDrawFrameBuffer :public ResizeGLWidget
{
public:
	QtResizeDrawFrameBuffer(QWidget* parenetWidget = NULL);
	virtual ~QtResizeDrawFrameBuffer();
	void renderFrame(UPTR_FME f);

	
public:
	void initializeGL() override;
	void paintGL() override;
	//void resizeGL(int width, int height) override;
	//void pushTextImage(GenTextImage&& textImage) { m_vecText.emplace_back(m_iPreFrameWidth, m_iPreFrameHeight,std::move(textImage)); }
	//std::shared_ptr<GLDrawText> pushTextImage(GenTextImage&& textImage);
	void pushDrawImageComponent(std::shared_ptr<CalcGLDraw>DrawImageComponent);
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent*)override;
	void mouseMoveEvent(QMouseEvent* event)override;
	UPTR_FME getFrameYUV420P();

	void createFramebufferObject();


protected:
	void drawFrame();
protected:
	void drawFrameBuffer();

	//void DrawComponent(std::shared_ptr<CalcGLDraw>obj);
	/*template<typename CSourceTypeDerived, typename = SFINE_IS_BASE_OF_T<CSourceType,CSourceTypeDerived>, typename = SFINE_IS_BASE_OF_T<CalcGLDraw, CSourceTypeDerived>>
	void DrawComponent(std::shared_ptr<CSourceTypeDerived>obj) 
	{
		if (obj->isRemoved())
			return;
		drawCalcDraw(obj);
	}*/

	void drawCalcDraw(std::shared_ptr<CalcGLDraw>calcDraw);

	//逐次绘制图像
	void drawImageComponent();
	void writeRGBImage();
	void deleteObject();

	std::shared_ptr<CalcGLDraw> getMouseRayGLDrawText(int width, int height, double xpos, double ypos, glm::mat4 view, glm::mat4 projection);


protected:
	//void offscreenRenderThreadFun();
	void drawFrameBufferThreadFun();

protected:
	std::vector<std::shared_ptr<CalcGLDraw>>m_vecDrawComponent;

	QOpenGLFramebufferObject* m_fbo = NULL;
	QOpenGLShaderProgram *m_programFrameBuffer = NULL;
	QOpenGLVertexArrayObject* m_vaoFrameBuffer = NULL;
	QOpenGLExtraFunctions* m_glFuncs = NULL;

	GLuint m_VBO = 0;
	GLuint m_EBO = 0;

	GLuint m_pboIds[2] = {0,0};
	int m_pboIndex = 0;

	UPTR_FME m_texFrame_RGB = NULL;
	UPTR_FME m_texFrame_YUV420P = NULL;
	bool m_bIsConverted = false;
	std::mutex m_mutex_m_texFrame_RGB;
	PicToYuv m_convertRGB2yuv420p;
	QOpenGLShaderProgram* m_programDrawText = NULL;
	QOpenGLVertexArrayObject* m_vaoDrawText = NULL;
	GLuint m_VBO_DrawText = 0;
	GLuint m_EBO_DrawText = 0;

	std::mutex m_mutex;

	///移动字体
	QPoint m_move_point;
	bool m_mouse_press = false;
	//待移动的字体对象
	std::shared_ptr<CalcGLDraw>m_pCalcGLDraw = NULL;

	std::mutex m_mutex_fbo;

	//std::shared_ptr<std::thread>m_threadDrawFrameBuffer;
	//std::condition_variable m_cond;
	//std::atomic_bool m_bStopThreadDrawFrameBuffer = false;

	//QOpenGLContext* m_ContextWorkThread = NULL;


};

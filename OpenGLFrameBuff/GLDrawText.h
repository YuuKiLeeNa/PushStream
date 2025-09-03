#pragma once
#include "DrawTextImage/GenTextImage.h"
//#include<QOpenGLShaderProgram>
#include<QOpenGLTexture>
//#include <QOpenGLVertexArrayObject>
//#include<QOpenGLWidget>
#include<QOpenGLFunctions>
#include "OpenGLFrameBuff/vmath.h"
#include "PushStream/qt_define_type.h"
#include "SourceType/CSourceType.h"
#include "CalcGLDraw.h"

class GLDrawText: public GenTextImage, public CalcGLDraw
{
public:
	//帧宽，帧高
	GLDrawText(int frameW, int frameH,const GenTextImage&obj);
	GLDrawText(int frameW, int frameH);
	GLDrawText() :GLDrawText(0, 0) {}
	virtual ~GLDrawText();
	//void deleteObject();
	GLDrawText(GLDrawText&& obj);

	//void setPosition(int x, int y) { m_xCol = x; m_yRow = y; }
	void init();
	void calcGLDrawRect();


	//UPTR_QTex m_tex;
	////顶点坐标
	//vmath::vec4 m_vec;
	////帧宽，帧高
	//int m_wFrame = 0;
	//int m_hFrame = 0;
	////左上角0,0
	//int m_xCol = 0;
	//int m_yRow = 0;
};
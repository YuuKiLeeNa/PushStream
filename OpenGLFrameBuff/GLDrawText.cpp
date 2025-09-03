#include "GLDrawText.h"
#include<QImage>

GLDrawText::GLDrawText(int frameW, int frameH, const GenTextImage& obj):GenTextImage(obj), CalcGLDraw(CSourceType::SourceDefineType::ST_FREE_TYPE_TEXT)
{
	setFrameWH(frameW, frameH);
	init();
}

GLDrawText::GLDrawText(int frameW, int frameH):CalcGLDraw(CSourceType::SourceDefineType::ST_FREE_TYPE_TEXT)
{
	setFrameWH(frameW, frameH);
}

//GLDrawText::GLDrawText(const GenTextImage& obj):GenTextImage(obj), CSourceType(CSourceType::SourceDefineType::ST_FREE_TYPE_TEXT)
//{
//	init();
//}

GLDrawText::~GLDrawText()
{
	//deleteObject();
}

GLDrawText::GLDrawText(GLDrawText&& obj):GenTextImage(std::move((GenTextImage&)obj)), CalcGLDraw(std::move((CalcGLDraw&)obj))
{
	if (this == &obj)
		return;
	//(CSourceType&)(*this) = std::move((CSourceType&)obj);
	//(GenTextImage&)(*this) = std::move((GenTextImage&)obj);
	 m_tex = std::move(obj.m_tex);
	/*QOpenGLVertexArrayObject* m_vao = NULL;
	GLuint m_VBO = 0;
	GLuint m_EBO = 0;*/

	//顶点坐标
	 m_vec = std::move(obj.m_vec);
	 //m_w = obj.m_w;
	 //m_h = obj.m_h;

}

//void GLDrawText::deleteObject()
//{
//	/*if (m_tex)
//	{
//		m_tex->destroy();
//		delete m_tex;
//		m_tex = NULL;
//	}*/
//	/*if(m_vao)
//	{
//		m_vao->destroy();
//		delete m_vao;
//		m_vao = NULL;
//	}
//	if (m_VBO)
//	{
//		glDeleteBuffers(1, &m_VBO);
//		m_VBO = 0;
//	}
//	if (m_EBO) 
//	{
//		glDeleteBuffers(1, &m_EBO);
//		m_EBO = 0;
//	}*/
//}

void GLDrawText::init()
{
	//CalcGLDraw::setData(image());
	cv::Mat mat = image();
	if (!mat.empty()) {
		cv::Mat rgbamat;
		cv::cvtColor(mat, rgbamat, cv::COLOR_BGRA2RGBA);
		CalcGLDraw::setData_callback(rgbamat);
	}
	else 
	{
		CalcGLDraw::setData_callback(mat);
	}
	//deleteObject();
	//m_tex.reset(new QOpenGLTexture(QOpenGLTexture::Target::Target2D));
	//m_tex->create();
	//if (!m_image.empty())
	//{
	//	m_tex->bind();

	////// 设置纹理参数，例如使用线性过滤

	/////上传纹理数据到OpenGL
	//	//m_tex->allocateStorage(QOpenGLTexture::BGRA, QOpenGLTexture::NoPixelType);
	//	//m_tex->setData(0,0,0,m_image.cols, m_image.rows,0,0,0,QOpenGLTexture::BGRA, QOpenGLTexture::NoPixelType, (const void*)m_image.data);
	//	cv::Mat dst;
	//	cv::flip(m_image, dst, 0);
	//	QImage image = QImage(dst.data, m_image.cols, m_image.rows, QImage::Format_ARGB32);
	//	m_tex->setMinificationFilter(QOpenGLTexture::Linear);
	//	m_tex->setMagnificationFilter(QOpenGLTexture::Linear);
	//	m_tex->setData(image);
	//	m_tex->generateMipMaps();

	//	m_tex->release();
	//	calcGLDrawRect();
	//	//setPosition(m_x, m_y);
	//}

	/*m_vao = new QOpenGLVertexArrayObject;
	m_vao->create();*/

	//float top = 1.0f - 2.0f * m_y / m_h;
	//float bottom = 1.0f - 2.0f * (m_y + m_image.rows) / m_h;

	//float left = 2.0f * m_x / m_w - 1.0f;
	//float right = 2.0f * (m_x + m_image.cols) / m_w - 1.0f;

	//GLfloat vertices[] = {
	//	// Positions // Texture Coords
	//	left, top, 0.0f, 1.0f, // Top Left
	//	right, top, 1.0f, 1.0f, // Top Right
	//	right, bottom, 1.0f, 0.0f, // Bottom Right
	//	left, bottom, 0.0f, 0.0f // Bottom Left
	//};

	/*float topTex = 1.0f - (float)m_y / m_h;
	float bottomTex = 1.0f - (m_y + m_image.rows) / m_h;

	float leftTex = (float)m_x / m_w;
	float rightTex = (float)(m_x + m_image.cols) / m_w;*/

	//m_vec = vmath::vec4{ leftTex, rightTex, topTex , bottomTex };
	//m_vec = vmath::vec4{ left, right, top , bottom };

	//GLuint indices[] = {
	//3, 0,1,2
	//};
	//m_vao->bind();
	//glGenBuffers(1, &m_VBO);
	//glGenBuffers(1, &m_EBO);

	//glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//// Positions
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	//glEnableVertexAttribArray(0);
	//// Texture Coords
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	//glEnableVertexAttribArray(1);
	//m_vao->release();
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void GLDrawText::calcGLDrawRect()
{
	//GenTextImage::setPosition(cols, rows);

	/*float topTex = 1.0f - (float)m_y / m_h;
	float bottomTex = 1.0f - (float)(m_y + m_image.rows) / m_h;
	float leftTex = (float)m_x / m_w;
	float rightTex = (float)(m_x + m_image.cols) / m_w;
	m_vec = vmath::vec4{ leftTex, rightTex, topTex , bottomTex };*/

	float top = 1.0f - 2.0f * m_yRow / m_hFrame;
	float bottom = 1.0f - 2.0f * (m_yRow + m_image.rows) / m_hFrame;
	float left = 2.0f * m_xCol / m_wFrame - 1.0f;
	float right = 2.0f * (m_xCol + m_image.cols) / m_yRow - 1.0f;
	m_vec = vmath::vec4{ left, right, top , bottom };

	//float top = 1.0f - 2.0f * m_y / m_h;
	//float bottom = 1.0f - 2.0f * (m_y + m_image.rows) / m_h;
	//float left = 2.0f * m_x / m_w - 1.0f;
	//float right = 2.0f * (m_x + m_image.cols) / m_w - 1.0f;
	//GLfloat vertices[] = {
	//	// Positions // Texture Coords
	//	left, top, 0.0f, 1.0f, // Top Left
	//	right, top, 1.0f, 1.0f, // Top Right
	//	right, bottom, 1.0f, 0.0f, // Bottom Right
	//	left, bottom, 0.0f, 0.0f // Bottom Left
	//};

	/*glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);*/
}

#include "CalcGLDraw.h"

CalcGLDraw::CalcGLDraw(CSourceType::SourceDefineType type): CSourceType(type)
{
	//initializeOpenGLContext();
	//m_tex.reset(new QOpenGLTexture(QOpenGLTexture::Target::Target2D));
	//GLenum err = glGetError();
	//if (err != GL_NO_ERROR) {
	//	printf("glGetError error %i\n", err);
	//}
	//m_tex->create();
	//m_tex->setMinificationFilter(QOpenGLTexture::Linear);
	//m_tex->setMagnificationFilter(QOpenGLTexture::Linear);
	////m_tex->generateMipMaps();




	//if (err != GL_NO_ERROR) {
	//	printf("glGetError error %i\n", err);
	//}
}

CalcGLDraw::CalcGLDraw(CalcGLDraw&& obj)noexcept :CSourceType(std::move((CSourceType&)obj))
, m_tex(std::move(obj.m_tex))
, m_vec(std::move(obj.m_vec))
, m_wFrame(obj.m_wFrame)
, m_hFrame(obj.m_hFrame)
, m_xCol(obj.m_xCol)
, m_yRow(obj.m_yRow)
, m_wImg(obj.m_wImg.load())
, m_hImg(obj.m_hImg.load())
, RGBA_data_(obj.RGBA_data_)
, m_fScale(obj.m_fScale)
, m_bIsTexChanged(obj.m_bIsTexChanged.load())
{
	obj.m_tex.reset();

}

CalcGLDraw& CalcGLDraw::operator=(CalcGLDraw&& obj)noexcept
{
	if (this == &obj)
		return *this;
	*(CSourceType*)this = std::move((CSourceType&)obj);
	m_tex = std::move(obj.m_tex);
	obj.m_tex.reset();
	m_vec = std::move(obj.m_vec);
	m_wFrame = obj.m_wFrame;
	m_hFrame = obj.m_hFrame;
	m_xCol = obj.m_xCol;
	m_yRow = obj.m_yRow;
	m_fScale = obj.m_fScale;
	m_wImg = obj.m_wImg.load();
	m_hImg = obj.m_hImg.load();
	RGBA_data_ = obj.RGBA_data_;
	m_bIsTexChanged = obj.m_bIsTexChanged.load();
	return *this;
}



//void CalcGLDraw::init(cv::Mat imageRGB)
//{
	/*m_tex.reset(new QOpenGLTexture(QOpenGLTexture::Target::Target2D));
	m_tex->create();
	m_tex->setMinificationFilter(QOpenGLTexture::Linear);
	m_tex->setMagnificationFilter(QOpenGLTexture::Linear);*/
	//if (m_tex && !imageRGB.empty())
	//{
	//	assert(imageRGB.type() == CV_8UC4);
	//	cv::Mat dst;
	//	cv::flip(imageRGB, dst, 0);
	//	glBindTexture(GL_TEXTURE_2D, m_tex->textureId());
	///*	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageRGB.cols, imageRGB.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, dst.data);
	//	glBindTexture(GL_TEXTURE_2D, 0);
	//	calcGLDrawRect(imageRGB);
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
//}

void CalcGLDraw::setData(cv::Mat imageRGBA) 
{
	GLenum err = glGetError();

	if (err != GL_NO_ERROR) {
		printf("glGetError error %i\n", err);
	}
	if (!m_tex) 
	{
		m_tex.reset(new QOpenGLTexture(QOpenGLTexture::Target::Target2D));
		m_tex->create();
		m_tex->setMinificationFilter(QOpenGLTexture::Linear);
		m_tex->setMagnificationFilter(QOpenGLTexture::Linear);
	}
	if (m_tex /* && !imageRGBA.empty()*/)
	{
		m_wImg = imageRGBA.cols;
		m_hImg = imageRGBA.rows;
		//assert(imageRGBA.type() == CV_8UC4);
		cv::Mat dst;
		if (!imageRGBA.empty())
		{
			cv::flip(imageRGBA, dst, 0);
		}

		if (err != GL_NO_ERROR) {
			printf("glGetError error %i\n", err);
		}
		glBindTexture(GL_TEXTURE_2D, m_tex->textureId());
		if (imageRGBA.cols > 0)
		{
			glPixelStorei(GL_UNPACK_ROW_LENGTH, imageRGBA.cols);
			err = glGetError();
			if (err != GL_NO_ERROR) {
				printf("glGetError error %i\n", err);
			}
		}
		//glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		/***************************************************************
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		****************************************************************/

		 err = glGetError();
		if (err != GL_NO_ERROR) {
			printf("glGetError error %i\n", err);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageRGBA.cols, imageRGBA.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst.data);
		
		 err = glGetError();
		if (err != GL_NO_ERROR) {
			printf("glGetError error %i\n", err);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		//calcGLDrawRect(imageRGBA);
	}
}

void CalcGLDraw::setData_callback(UPTR_FME f)
{
	if (!f && (AVPixelFormat)f->format != AV_PIX_FMT_RGBA)
		return;
	m_wImg = f->width;
	m_hImg = f->height;
	cv::Mat bgra_mat(cv::Size(f->width, f->height), CV_8UC4, f->data[0]);
	cv::Mat rgba_mat;
	cv::cvtColor(bgra_mat, rgba_mat, cv::COLOR_BGRA2RGBA);
	{
		std::lock_guard<std::mutex>lock(m_mutexRGBA);
		RGBA_data_ = rgba_mat;
	}
	m_bIsTexChanged = true;
}

void CalcGLDraw::setData_callback(cv::Mat imageRGBA)
{
	if (imageRGBA.empty() || imageRGBA.type() != CV_8UC4)
	{
		{
			std::lock_guard<std::mutex>lock(m_mutexRGBA);
			RGBA_data_ = imageRGBA;
		}
		m_bIsTexChanged = true;
		return;
	}
	m_wImg = imageRGBA.cols;
	m_hImg = imageRGBA.rows;
	{
		std::lock_guard<std::mutex>lock(m_mutexRGBA);
		RGBA_data_ = imageRGBA;
	}
	m_bIsTexChanged = true;
}

void CalcGLDraw::setData() 
{
	if (/*m_bIsAsync &&*/ m_bIsTexChanged)
	{
		m_bIsTexChanged = false;
		std::lock_guard<std::mutex>lock(m_mutexRGBA);
		setData(RGBA_data_);
	}
}

void CalcGLDraw::calcGLDrawRect(cv::Mat imageRGBA)
{
	//GenTextImage::setPosition(cols, rows);
	/*float topTex = 1.0f - (float)m_y / m_h;
	float bottomTex = 1.0f - (float)(m_y + m_image.rows) / m_h;
	float leftTex = (float)m_x / m_w;
	float rightTex = (float)(m_x + m_image.cols) / m_w;
	m_vec = vmath::vec4{ leftTex, rightTex, topTex , bottomTex };*/

	float top = 1.0f - 2.0f * m_yRow / m_hFrame;
	float bottom = 1.0f - 2.0f * (m_yRow + imageRGBA.rows * m_fScale) / m_hFrame;
	float left = 2.0f * m_xCol / m_wFrame - 1.0f;
	float right = 2.0f * (m_xCol + imageRGBA.cols * m_fScale) / m_yRow - 1.0f;
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

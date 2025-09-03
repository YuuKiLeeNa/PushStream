#pragma once
#include<QOpenGLFunctions>
#include<QOpenGLWidget>
#include<thread>
#include <QOpenGLVertexArrayObject>
#include <QSurfaceFormat>
#include<atomic>
#include<mutex>
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#ifdef __cplusplus
}
#endif
#include<QOpenGLShaderProgram>
#include<QOpenGLTexture>
#include "define_type.h"
#include "PicToYuv.h"


struct AVFrame;
class QtGLVideoWidget :public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	QtGLVideoWidget(QWidget* pParentWidget = nullptr);
	virtual ~QtGLVideoWidget();
	void renderFrame(UPTR_FME f);
	AVPixelFormat getNeededPixelFormat();
	void setFrameSize(int width, int height);
	inline bool isSupportPixelFormat(AVPixelFormat fmt);
	void deleteObject();
protected:
	void setFrame(UPTR_FME f);
protected:
	QRect calculate_display_rect(AVFrame* f);
	QRect calculate_display_rect(
		int scr_xleft, int scr_ytop, int scr_width, int scr_height,
		int pic_width, int pic_height, AVRational pic_sar);
	void deleteTex();
	void initConnect();

	//QOpenGLTexture* texture = nullptr;
	AVPixelFormat m_pixNeeded = AV_PIX_FMT_YUV420P;

protected:
	bool m_bMousePressed = false;
	QPoint StartPos;
	std::pair<QString, int> m_strSaveFile;
	UPTR_FME m_frameSave;
	UPTR_FME m_frame_tmp;
	bool m_bUploadTex = false;
	int m_iPreFrameXOffset = 0;
	int m_iPreFrameYOffset = 0;
	std::atomic_int m_iPreFrameWidth = 800;
	std::atomic_int m_iPreFrameHeight = 800;
	int m_iPreFrameWinWidth = 0;
	int m_iPreFrameWinHeight = 0;
	AVRational m_preFrameRatio;
	std::mutex m_mutex;
	AVPixelFormat m_frameFormat = AV_PIX_FMT_NONE;
	PicToYuv m_imgConvert;

	void setRGBPixels(uint8_t* pixels, int stride);
	void setRGBAPixels(uint8_t* pixels, int stride);
	void setYPixels(uint8_t* pixels, int stride);
	void setUPixels(uint8_t* pixels, int stride);
	void setVPixels(uint8_t* pixels, int stride);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;
	QRect m_drawRect;
protected:
	enum TextureType {
		YTexture,
		UTexture,
		VTexture,
		RGBTexture,
		RGBATexture
	};

	void initializeTextures();
	void bindPixelTexture(GLuint texture, TextureType textureType, uint8_t* pixels, int stride);
	bool initShader();

	QOpenGLShaderProgram m_program;
	QOpenGLVertexArrayObject m_vao;

	QOpenGLTexture* m_textureRGB = NULL;
	QOpenGLTexture* m_textureRGBA = NULL;
	QOpenGLTexture* m_textureY = NULL;
	QOpenGLTexture* m_textureU = NULL;
	QOpenGLTexture* m_textureV = NULL;

	GLint u_pos{ 0 };
	std::atomic_bool m_isOpenGLInit = false;
};


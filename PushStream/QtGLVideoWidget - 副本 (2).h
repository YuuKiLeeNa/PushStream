#pragma once
#include<QOpenGLFunctions>
#include<QOpenGLWidget>
#include<thread>
#include <QOpenGLVertexArrayObject>
#include <QSurfaceFormat>
#include<atomic>
#include<mutex>
extern "C"
{
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
}
#include<QOpenGLShaderProgram>
#include<QOpenGLTexture>

struct AVFrame;
class QtGLVideoWidget:public QOpenGLWidget,protected QOpenGLFunctions
{
	Q_OBJECT
public:
	QtGLVideoWidget(QWidget*pParentWidget = nullptr);
	virtual ~QtGLVideoWidget();
	void renderFrame(AVFrame* f);
	AVPixelFormat getNeededPixeFormat();
signals:
	void signalsGetFrame();
public slots:
	void slotGetFrame();

protected:
	void calculate_display_rect(AVFrame* f);
	void calculate_display_rect(
		int scr_xleft, int scr_ytop, int scr_width, int scr_height,
		int pic_width, int pic_height, AVRational pic_sar);
	void deleteTex();
	void initConnect();
	struct SwsContext* m_SwsContext = nullptr;
	QVector<QVector3D> vertices;
	QVector<QVector2D> texCoords;
	QOpenGLShaderProgram program;
	QOpenGLTexture* texture = nullptr;
	QMatrix4x4 projection;
	const AVPixelFormat m_pixNeeded = AV_PIX_FMT_YUV420P;

protected:
	bool m_bMousePressed = false;
	QPoint StartPos;
	/*std::thread m_thdGathVideo;
	std::thread m_thdGathAudio;
	std::thread m_thdEnc;

	std::condition_variable m_conVarVideoThd;
	std::condition_variable m_conVarAudioThd;
	std::mutex m_muxVideo;
	bool m_bVideoCodecReady;
	std::mutex m_muxAudio;
	bool m_bAudioCodecReady;
	std::once_flag m_onceFlag;
	COpenStream m_video;
	COpenStream m_audio;
	CEncode m_enc;*/
	std::pair<QString, int> m_strSaveFile;
	AVFrame* m_frameSave;

	int m_iPreFrameXOffset = 0;
	int m_iPreFrameYOffset = 0;
	std::atomic_int m_iPreFrameWidth = 1920;
	std::atomic_int m_iPreFrameHeight = 1080;
	int m_iPreFrameWinWidth = 0;
	int m_iPreFrameWinHeight = 0;
	AVRational m_preFrameRatio;
	int64_t m_i64VideoLength;
	std::mutex m_mutex;
	AVPixelFormat m_frameFormat = AV_PIX_FMT_NONE;
	std::unique_ptr<std::thread, std::function<void(std::thread*&)>> m_thread;
	char m_szCmdLine[2][512];
	char* m_pp[2];

	void setFrameSize(int width, int height);
	void setYPixels(uint8_t* pixels, int stride);
	void setUPixels(uint8_t* pixels, int stride);
	void setVPixels(uint8_t* pixels, int stride);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;
	QRect m_drawRect;
private:
	enum YUVTextureType {
		YTexture,
		UTexture,
		VTexture
	};

	void initializeTextures();
	void bindPixelTexture(GLuint texture, YUVTextureType textureType, uint8_t* pixels, int stride);

	QOpenGLShaderProgram m_program;
	QOpenGLVertexArrayObject m_vao;

	//std::atomic_int m_frameWidth = 1920;
	//std::atomic_int m_frameHeight = 1080;

	GLuint y_tex{ 0 };
	GLuint u_tex{ 0 };
	GLuint v_tex{ 0 };
	GLint u_pos{ 0 };
	std::atomic_bool m_isOpenGLInit = false;
};


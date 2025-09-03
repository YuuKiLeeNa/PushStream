// www.qtav.org

#include "GLVideoWidget.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif


static const QMatrix4x4 yuv2rgb_bt601 =
           QMatrix4x4(
                1.0f,  0.000f,  1.402f, 0.0f,
                1.0f, -0.344f, -0.714f, 0.0f,
                1.0f,  1.772f,  0.000f, 0.0f,
                0.0f,  0.000f,  0.000f, 1.0f)
            *
            QMatrix4x4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, -0.5f,
                0.0f, 0.0f, 1.0f, -0.5f,
                0.0f, 0.0f, 0.0f, 1.0f);

const GLfloat kVertices[] = {
    -1, 1,
    -1, -1,
    1, 1,
    1, -1,
};
const GLfloat kTexCoords[] = {
    0, 0,
    0, 1,
    1, 0,
    1, 1,
};

char const *const* attributes()
{
    //static const char a0[] = {0x61, 0x5f, 0x50, 0x6f, 0x73, 0x0};
    //static const char a1[] = {0x61, 0x5f, 0x54, 0x65, 0x78, 0x0};
    static const char a0[] = "a_Pos";
    static const char a1[] = "a_Tex";//{0x61, 0x5f, 0x54, 0x65, 0x78, 0x0};
    static const char a2[] = {0x00, 0x51, 0x74, 0x41, 0x56, 0x0};
    static const char* A[] = { a0, a1, a2};
    return A;
}

typedef struct {
    QImage::Format qfmt;
    GLint internal_fmt;
    GLenum fmt;
    GLenum type;
    int bpp;
} gl_fmt_entry_t;

#define glsl(x) #x
    static const char kVertexShader[] = glsl(
        attribute vec4 a_Pos;
        attribute vec2 a_Tex;
        uniform mat4 u_MVP_matrix;
        varying vec2 v_TexCoords;
        void main() {
          gl_Position = u_MVP_matrix * a_Pos;
          v_TexCoords = a_Tex;
        });

    static const char kFragmentShader[] = glsl(
            uniform sampler2D u_Texture0;
            uniform sampler2D u_Texture1;
            uniform sampler2D u_Texture2;
            varying mediump vec2 v_TexCoords;
            uniform mat4 u_colorMatrix;
            void main()
            {
                 gl_FragColor = clamp(u_colorMatrix
                                     * vec4(
                                         texture2D(u_Texture0, v_TexCoords).r,
                                         texture2D(u_Texture1, v_TexCoords).r,
                                         texture2D(u_Texture2, v_TexCoords).r,
                                         1)
                                     , 0.0, 1.0);
            });
     static const char kFragmentShaderRGB[] = glsl(
                 uniform sampler2D u_Texture0;
                 varying mediump vec2 v_TexCoords;
                 void main() {
                     vec4 c = texture2D(u_Texture0, v_TexCoords);
                     gl_FragColor = c.rgba;
                 });
#undef glsl

GLVideoWidget::GLVideoWidget(QWidget *parent)
    : QGLWidget(parent)
    , update_res(true)
    , upload_tex(true)
    , m_program(0)
{
//    setAttribute(Qt::WA_OpaquePaintEvent);
  //  setAttribute(Qt::WA_NoSystemBackground);
    //default: swap in qpainter dtor. we should swap before QPainter.endNativePainting()
    memset(tex, 0, 3);
}

void GLVideoWidget::setFrameData(std::shared_ptr<uint8_t>data)
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    upload_tex = true;
    m_data = data;
    plane[0].data = (char*)m_data.get();
    if (plane.size() > 1) {
        plane[1].data = plane[0].data + plane[0].stride*m_height;
        plane[2].data = plane[1].data + plane[1].stride*m_height/2;
    }
    update();
}

void GLVideoWidget::setImage(const QImage &img)
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    upload_tex = true;
    m_image = img;
    plane[0].data = (char*)m_image.constBits();
    update();
}

void GLVideoWidget::bind()
{
    for (int i = 0; i < plane.size(); ++i) {
        bindPlane((i + 1) % plane.size());
    }
    upload_tex = false;
}

void GLVideoWidget::bindPlane(int p)
{
    glActiveTexture(GL_TEXTURE0 + p);
    glBindTexture(GL_TEXTURE_2D, tex[p]);
    if (!upload_tex)
        return;
    // This is necessary for non-power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const Plane &P = plane[p];
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, P.upload_size.width(), P.upload_size.height(), P.fmt, P.type, P.data);
}

void GLVideoWidget::initTextures()
{
    glDeleteTextures(3, tex);
    memset(tex, 0, 3);
    glGenTextures(plane.size(), tex);
    //qDebug("init textures...");
    for (int i = 0; i < plane.size(); ++i) {
        const Plane &P = plane[i];
        //qDebug("tex[%d]: %u", i, tex[i]);
        glBindTexture(GL_TEXTURE_2D, tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // This is necessary for non-power-of-two textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, P.internal_fmt, P.tex_size.width(), P.tex_size.height(), 0/*border, ES not support*/, P.fmt, P.type, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void GLVideoWidget::setYUV420pParameters(int w, int h, int *strides)
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    update_res = true;
    m_data.reset();
    m_image = QImage();
    m_width = w;
    m_height = h;
    plane.resize(3);
    Plane &p = plane[0];
    p.data = 0;
    p.stride = strides && strides[0] ? strides[0] : w;
    p.tex_size.setWidth(p.stride);
    p.upload_size.setWidth(p.stride);
    p.tex_size.setHeight(h);
    p.upload_size.setHeight(h);
    p.internal_fmt = p.fmt = GL_LUMINANCE;
    p.type = GL_UNSIGNED_BYTE;
    p.bpp = 1;
    for (int i = 1; i < plane.size(); ++i) {
        Plane &p = plane[i];
        p.stride = strides && strides[i] ? strides[i] : w/2;
        p.tex_size.setWidth(p.stride);
        p.upload_size.setWidth(p.stride);
        p.tex_size.setHeight(h/2);
        p.upload_size.setHeight(h/2);
        p.internal_fmt = p.fmt = GL_LUMINANCE;
        p.type = GL_UNSIGNED_BYTE;
        p.bpp = 1;
        qDebug() << p.tex_size;
    }
}

void GLVideoWidget::setQImageParameters(QImage::Format fmt, int w, int h, int stride)
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    update_res = true;
    m_data.reset();
    m_image = QImage();
    m_width = w;
    m_height = h;
    plane.resize(1);
    Plane &p = plane[0];
    p.data = 0;
    p.stride = stride ? stride : QImage(w, h, fmt).bytesPerLine();
    static const gl_fmt_entry_t fmts[] = {
        { QImage::Format_RGB888, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 3},
        { QImage::Format_Invalid, 0, 0, 0, 0}
    };
    for (int i = 0; fmts[i].bpp; ++i) {
        if (fmts[i].qfmt == fmt) {
            Plane &p = plane[0];
            p.internal_fmt = fmts[i].internal_fmt;
            p.fmt = fmts[i].fmt;
            p.type = fmts[i].type;
            p.internal_fmt = fmts[i].internal_fmt;
            p.bpp = fmts[i].bpp;

            p.tex_size.setWidth(p.stride/p.bpp);
            p.upload_size.setWidth(p.stride/p.bpp);
            p.tex_size.setHeight(h);
            p.upload_size.setHeight(h);
            return;
        }
    }
    qFatal("Unsupported QImage format %d!", fmt);
}

void GLVideoWidget::paintGL()
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (!plane[0].data)
        return;
    if (update_res || !tex[0]) {
        calculate_display_rect(0, 0, width(), height(), m_width, m_height);
        initializeShader();
        initTextures();
        update_res = false;
        //u_pos = m_program->uniformLocation("draw_pos");
    }
    bind();
    m_program->bind();
    //glUniform4f(0, m_drawRect.left(), m_drawRect.top(), m_drawRect.width(), m_drawRect.height());
    for (int i = 0; i < plane.size(); ++i) {
        m_program->setUniformValue(u_Texture[i], (GLint)i);
    }
    m_program->setUniformValue(u_colorMatrix, yuv2rgb_bt601);
    m_program->setUniformValue(u_MVP_matrix, m_mat);

    // uniform end. attribute begin
    // kVertices ...
    // normalize?
    m_program->setAttributeArray(0, GL_FLOAT, kVertices, 2);
    m_program->setAttributeArray(1, GL_FLOAT, kTexCoords, 2);
    char const *const *attr = attributes();

    /*****************************************
    ******************************************/
     const char a0[] = { 0x61, 0x5f, 0x50, 0x6f, 0x73, 0x0 };
     const char a1[] = { 0x61, 0x5f, 0x54, 0x65, 0x78, 0x0 };
     const char a2[] = { 0x00, 0x51, 0x74, 0x41, 0x56, 0x0 };

     const char* A[] = { a0, a1, a2 };

     const char a3[] = { 0x51, 0x74, 0x41, 0x56, 0x0 };

    for (int i = 0; attr[i][0]; ++i) {
        m_program->enableAttributeArray(i); //TODO: in setActiveShader
    }
    //u_pos = m_program->uniformLocation("a_Pos");
    //glUniform4f(u_pos, m_drawRect.left(), m_drawRect.top(), m_drawRect.width(), m_drawRect.height());
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    for (int i = 0; attr[i][0]; ++i) {
        m_program->disableAttributeArray(i); //TODO: in setActiveShader
    }
    //update();
}

void GLVideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
}

void GLVideoWidget::resizeGL(int w, int h)
{
    calculate_display_rect(0, 0, w, h, m_width, m_height);
    glViewport(0, 0, w, h);
    m_mat.setToIdentity();
    //m_mat.ortho(QRectF(0, 0, w, h));
}

void GLVideoWidget::calculate_display_rect(
    int scr_xleft, int scr_ytop, int scr_width, int scr_height,
    int pic_width, int pic_height, const AVRational& pic_sar)
{
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
    m_drawRect.setX(scr_xleft + x);
    m_drawRect.setY(scr_ytop + y);
    m_drawRect.setWidth(FFMAX((int)width, 1));
    m_drawRect.setHeight(FFMAX((int)height, 1));
}


void GLVideoWidget::initializeShader()
{
    if (m_program) {
        m_program->release();
        delete m_program;
        m_program = 0;
    }
    m_program = new QGLShaderProgram(this);

    m_program->addShaderFromSourceCode(QGLShader::Vertex, kVertexShader);
    QByteArray frag;
    if (plane.size() > 1)
        frag = QByteArray(kFragmentShader);
    else
        frag = QByteArray(kFragmentShaderRGB);
    frag.prepend("#ifdef GL_ES\n"
                 "precision mediump int;\n"
                 "precision mediump float;\n"
                 "#else\n"
                 "#define highp\n"
                 "#define mediump\n"
                 "#define lowp\n"
                 "#endif\n");
    m_program->addShaderFromSourceCode(QGLShader::Fragment, frag);

    char const *const *attr = attributes();
    for (int i = 0; attr[i][0]; ++i) {
        m_program->bindAttributeLocation(attr[i], i);
    }
    if (!m_program->link()) {
        qWarning("QSGMaterialShader: Shader compilation failed:");
        qWarning() << m_program->log();
        qDebug("frag: %s", plane.size() > 1 ? kFragmentShader : kFragmentShaderRGB);
    }

    u_MVP_matrix = m_program->uniformLocation("u_MVP_matrix");
    // fragment shader
    u_colorMatrix = m_program->uniformLocation("u_colorMatrix");
    for (int i = 0; i < plane.size(); ++i) {
        QString tex_var = QString("u_Texture%1").arg(i);
        u_Texture[i] = m_program->uniformLocation(tex_var);
        qDebug("glGetUniformLocation(\"%s\") = %d", tex_var.toUtf8().constData(), u_Texture[i]);
    }
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d", u_MVP_matrix);
    qDebug("glGetUniformLocation(\"u_colorMatrix\") = %d", u_colorMatrix);
}

#pragma once

#include<memory>
#include<QOpenGLTexture>


struct free_QTOpenGLTex 
{
	void operator()(QOpenGLTexture* tex) { if (tex) { tex->destroy(); delete tex; } }
};


typedef std::unique_ptr<QOpenGLTexture, free_QTOpenGLTex>UPTR_QTex;


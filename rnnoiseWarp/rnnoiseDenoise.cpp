#include "rnnoiseDenoise.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "rnnoise.h"
#ifdef __cplusplus
}
#endif
#include<cstdlib>
#include "Audio/utilfun.h"


rnnoiseDenoise::rnnoiseDenoise()
{
	std::string model_file = GetExePath() + "weights_blob.bin";
	m_model = rnnoise_model_from_filename(model_file.c_str());
	m_st = rnnoise_create(m_model);
}

rnnoiseDenoise::~rnnoiseDenoise()
{
	destroy();
}

bool rnnoiseDenoise::create() 
{
	destroy();
	std::string model_file = GetExePath() + "weights_blob.bin";
	m_model = rnnoise_model_from_filename(model_file.c_str());
	m_st = rnnoise_create(m_model);
	return (m_model != NULL && m_st != NULL);
}

void rnnoiseDenoise::destroy() 
{
	if (m_st)
	{
		rnnoise_destroy(m_st);
		m_st = NULL;
	}
	if (m_model)
	{
		rnnoise_model_free(m_model);
		m_model = NULL;
	}
}

rnnoiseDenoise::rnnoiseDenoise(rnnoiseDenoise&& obj) noexcept:m_model(obj.m_model), m_st(obj.m_st)
{
	if (this == &obj)
		return;
	obj.m_model = NULL;
	obj.m_st = NULL;
}

rnnoiseDenoise& rnnoiseDenoise::operator=(rnnoiseDenoise&& obj)noexcept
{
	if (&obj == this)
		return(*this);
	m_model = obj.m_model;
	m_st = obj.m_st;
	obj.m_model = NULL;
	obj.m_st = NULL;
	return (*this);
}

int rnnoiseDenoise::frameSize()
{
	return s_frame_size;
}

bool rnnoiseDenoise::translate(const short* in, short* out)
{
	float* f = (float*)malloc(sizeof(float) * s_frame_size);
	if (!f)
		return false;
	for (int i = 0; i < s_frame_size; ++i)
		f[i] = in[i];
	rnnoise_process_frame(m_st, f, f);
	for (int i = 0; i < s_frame_size; ++i)
		out[i] = f[i];
	return true;
}

rnnoiseDenoise::operator bool()
{
	return m_model != NULL && m_st != NULL;
}

const int rnnoiseDenoise::s_frame_size = rnnoise_get_frame_size();
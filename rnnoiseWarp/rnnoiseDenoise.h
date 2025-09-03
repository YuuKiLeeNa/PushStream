#pragma once

class DenoiseState;
class RNNModel;
class rnnoiseDenoise
{
public:
	rnnoiseDenoise();
	~rnnoiseDenoise();
	bool create();
	void destroy();
	rnnoiseDenoise(rnnoiseDenoise&& obj) noexcept;
	rnnoiseDenoise(const rnnoiseDenoise&) = delete;
	rnnoiseDenoise& operator=(const rnnoiseDenoise&) = delete;
	rnnoiseDenoise& operator=(rnnoiseDenoise&&obj)noexcept;
	int frameSize();
	bool translate(const short*in,short*out);
	operator bool();
	static const int s_frame_size;
protected:
	RNNModel* m_model = 0;
	DenoiseState* m_st = 0;
	
};
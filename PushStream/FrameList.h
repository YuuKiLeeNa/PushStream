#pragma once
extern "C"
{
#include "libavutil/frame.h"
}
#include<memory>
#include<mutex>
#include<list>
#include<condition_variable>

struct FrameList
{
protected:
	typedef struct FREE_FRAME
	{
		void operator()(AVFrame* f)
		{
			av_frame_unref(f);
			av_frame_free(&f);
		}
	}FREE_FRAME;
	typedef std::unique_ptr<AVFrame, FREE_FRAME>PTR;
public:
	void push(AVFrame* f);
	AVFrame* pop();
	AVFrame* wait_pop();
protected:
	std::mutex m_mux;
	std::list<PTR>list_;
	std::condition_variable cond_;
	bool stop_ = false;
};


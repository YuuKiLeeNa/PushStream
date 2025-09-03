#include "FrameList.h"

void FrameList::push(AVFrame* f) 
{
	std::unique_lock<std::mutex>lg(m_mux);
	list_.push_back(decltype(list_)::value_type(f));
	cond_.notify_one();
}

AVFrame* FrameList::pop() 
{
	decltype(list_)::value_type v;
	{
		std::unique_lock<std::mutex>lg(m_mux);
		if (!list_.empty())
		{
			v = std::move(list_.front());
			list_.pop_front();
		}
	}
	return v.release();
}

AVFrame* FrameList::wait_pop() 
{
	decltype(list_)::value_type v;
	{
		std::unique_lock<std::mutex>lg(m_mux);
		cond_.wait(lg, [this]()
			{
				return stop_ || !list_.empty();
			});
		if (!stop_)
		{
			v = std::move(list_.front());
			list_.pop_front();
		}
	}
	return v.release();
}
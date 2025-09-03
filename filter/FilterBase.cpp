#include "FilterBase.h"



std::shared_ptr<AVFilterGraph> FilterBase::CORE_DATA::create_graph() 
{
	return std::shared_ptr<AVFilterGraph>(avfilter_graph_alloc(), [](AVFilterGraph*& f) {avfilter_graph_free(&f); });
}

int FilterBase::addFilter()
{
	if (filter_)
	{
		std::lock_guard<std::mutex>lock(core_data_.mutex_);
		core_data_.filters_.emplace(name_, shared_from_this());
	}
	return 0;
}

int FilterBase::createFilter(AVFilterContext** filter)
{
	if (!core_data_.graph_)
		return AVERROR_INVALIDDATA;
	int ret = avfilter_graph_create_filter(&filter_, avfilter_get_by_name(sname_.c_str()), name_.c_str(), ff().c_str(), nullptr, core_data_.graph_.get());
	if (ret >= 0)
	{
		if (filter)
			*filter = filter_;
		//addFilter();
		specialOp();
	}
	else 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		av_make_error_string(szErr, sizeof(szErr), ret);
	}
	return ret;
}

int FilterBase::send_commend(const std::string& cmd, const std::string& args)
{
	return avfilter_graph_send_command(core_data_.graph_.get(), name_.c_str(), cmd.c_str(), args.c_str(), nullptr, 0, 0);
}

std::atomic_ullong FilterBase::sindex_ = 0;
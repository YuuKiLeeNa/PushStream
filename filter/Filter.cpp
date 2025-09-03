#include "Filter.h"

int Filter::send_commend(const std::string& filterName, const std::string& cmd, const std::string& args)
{
	return avfilter_graph_send_command(data_.graph_.get(), filterName.c_str(), cmd.c_str(), args.c_str(), nullptr, 0, 0);
}
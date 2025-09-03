#pragma once
#include"FilterBase.h"
struct Filter
{
	int send_commend(const std::string& filterName, const std::string& cmd, const std::string& args);
	FilterBase::CORE_DATA data_;
	virtual ~Filter() {}
};


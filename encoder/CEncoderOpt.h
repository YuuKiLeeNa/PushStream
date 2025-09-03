#pragma once
#include "CMediaOption.h"

class CEncoderOpt
{
public:
	void setOption(const CMediaOption&opt);
	CMediaOption getOption();
protected:
	CMediaOption m_opt;
};

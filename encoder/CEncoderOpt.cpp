#include "CEncoderOpt.h"

void CEncoderOpt::setOption(const CMediaOption& opt)
{
	m_opt = opt;
}

CMediaOption CEncoderOpt::getOption()
{
	return m_opt;
}

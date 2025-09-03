#include "imageConvert.h"

imageConvert::~imageConvert() 
{
	reset();
}


imageConvert::imageConvert(imageConvert&& obj) noexcept:m_inFormat(obj.m_inFormat)
, m_inWid(obj.m_inWid)
, m_inHei(obj.m_inHei)
, m_outFormat(obj.m_outFormat)
, m_outWid(obj.m_outWid)
, m_outHei(obj.m_outHei)
,m_con(obj.m_con)
{
	if (this != &obj)
	{
		obj.m_con = nullptr;
		obj.reset();
	}
}

imageConvert& imageConvert::operator=(imageConvert&& obj) noexcept
{
	if (this == &obj)
		return *this;
	m_inFormat = obj.m_inFormat;
	m_inWid = obj.m_inWid;
	m_inHei = obj.m_inHei;
	m_outFormat = obj.m_outFormat;
	m_outWid = obj.m_outWid;
	m_outHei = obj.m_outHei;
	m_con = obj.m_con;
	obj.m_con = nullptr;
	obj.reset();
	return *(this);
}

bool imageConvert::init(AVPixelFormat inFormat, int inWid, int inHei, AVPixelFormat outFormat, int outWid, int outHei)
{
	if (m_inFormat == inFormat
		&& m_inWid == inWid
		&& m_inHei == inHei
		&& m_outFormat == outFormat
		&& m_outWid == outWid
		&& m_outHei == outHei
		&& m_con
		)
		return true;

	reset();
	if(!(m_con = sws_getContext(inWid, inHei, inFormat, outWid, outHei, outFormat, SWS_BICUBIC, nullptr, nullptr,nullptr)))
		return false;
	m_inFormat = inFormat;
	m_inWid = inWid;
	m_inHei = inHei;
	m_outFormat = outFormat;
	m_outWid = outWid;
	m_outHei = outHei;
	return true;
}

void imageConvert::reset() 
{
	if (m_con) 
	{
		sws_freeContext(m_con);
		m_con = nullptr;
	}

	m_inFormat = AV_PIX_FMT_NONE;
	m_inWid = 0;
	m_inHei = 0;
	m_outFormat = AV_PIX_FMT_NONE;
	m_outWid = 0;
	m_outHei = 0;
}

bool imageConvert::operator()(const uint8_t* const srcSlice[]
	,const int srcStride[]
	,uint8_t* const dst[]
	, const int dstStride[])
{
	if (!m_con)
		return false;
	return sws_scale(m_con, srcSlice, srcStride, 0, m_inHei, dst, dstStride) == m_outHei;
}

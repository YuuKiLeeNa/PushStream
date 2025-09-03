#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include "libswscale/swscale.h"

#ifdef __cplusplus
}
#endif


class imageConvert
{
public:
	virtual ~imageConvert();
	imageConvert() = default;
	imageConvert(const imageConvert&) = delete;
	imageConvert(imageConvert&& obj) noexcept;
	imageConvert& operator=(const imageConvert&) = delete;
	imageConvert& operator=(imageConvert&& obj) noexcept;
	bool init(AVPixelFormat inFormat, int inWid, int inHei,AVPixelFormat outFormat,int outWid,int outHei);
	void reset();
	bool operator()(const uint8_t* const srcSlice[]
		, const int srcStride[]
		, uint8_t* const dst[]
		, const int dstStride[]);

	operator bool() { return m_con != nullptr; }
protected:
	AVPixelFormat m_inFormat = AV_PIX_FMT_NONE;
	int m_inWid = 0;
	int m_inHei = 0;
	AVPixelFormat m_outFormat = AV_PIX_FMT_NONE;
	int m_outWid = 0;
	int m_outHei = 0;
	SwsContext* m_con = nullptr;

};


#include "PicToYuv.h"
#ifdef __cplusplus
extern "C" 
{
#endif

#include "libavutil/imgutils.h"

#ifdef __cplusplus
}
#endif
#include "Util/logger.h"
#include "Network/Socket.h"



bool PicToYuv::setInfo(int wid, int hei, int dep)
{
	AVPixelFormat f;
	switch (dep)
	{
	case 8:
		f = AV_PIX_FMT_GRAY8;
		break;
	case 16:
		f = AV_PIX_FMT_RGB555;
		break;
	case 24:
		f = AV_PIX_FMT_RGB24;
		break;
	case 32:
		f = AV_PIX_FMT_RGB32;
		break;
	default:
		reset();
		return false;
	}
	return init(f, wid, hei, AV_PIX_FMT_YUV420P, wid, hei);
}

bool PicToYuv::operator()(const uint8_t* sData, long slen, std::shared_ptr<uint8_t>&data) 
{
	uint8_t* srcArr[4], *dstArr[4];
	int srcLine[4],dstLine[4];

	if (av_image_fill_linesizes(srcLine, m_inFormat, m_inWid) < 0)
		return false;
	if (av_image_fill_arrays(srcArr, srcLine, sData, m_inFormat, m_inWid, m_inHei, 1) < 0)
		return false;
	if (av_image_fill_linesizes(dstLine, m_outFormat, m_outWid) < 0)
		return false;
	int ret = av_image_get_buffer_size(m_outFormat,m_outWid, m_outHei, 1);
	if (ret <= 0)
		return false;
	std::shared_ptr<uint8_t>d;
	d.reset(new uint8_t[ret]);
	if (av_image_fill_arrays(dstArr, dstLine, d.get(), m_outFormat, m_outWid, m_outHei, 1) < 0)
		return false;
	if (imageConvert::operator()(srcArr, srcLine, dstArr, dstLine))
	{
		data = std::move(d);
		return true;
	}
	return false;
}

bool PicToYuv::operator()(const uint8_t* sData, long slen, AVFrame* dst) 
{
	uint8_t* srcArr[4];// , * dstArr[4];
	int srcLine[4];// , dstLine[4];

	if (av_image_fill_linesizes(srcLine, m_inFormat, m_inWid) < 0)
		return false;
	if (av_image_fill_arrays(srcArr, srcLine, sData, m_inFormat, m_inWid, m_inHei, 1) < 0)
		return false;
	//if (av_image_fill_linesizes(dstLine, m_outFormat, m_outWid) < 0)
	//	return false;
	av_frame_unref(dst);

	dst->width = m_outWid;
	dst->height = m_outHei;
	dst->format = m_outFormat;

	int ret;
	if ((ret = av_frame_get_buffer(dst, 1)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_get_buffer:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	if ((ret = av_frame_make_writable(dst)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_make_writable:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}


	//int ret = av_image_get_buffer_size(m_outFormat, m_outWid, m_outHei, 1);
	//if (ret <= 0)
	//	return false;
	//std::shared_ptr<uint8_t>d;
	//d.reset(new uint8_t[ret]);
	//if (av_image_fill_arrays(dstArr, dstLine, d.get(), m_outFormat, m_outWid, m_outHei, 1) < 0)
	//	return false;
	if (imageConvert::operator()(srcArr, srcLine, dst->data, dst->linesize))
	{
		return true;
	}
	av_frame_unref(dst);
	return false;
}

bool PicToYuv::operator()(AVFrame* src, AVFrame* dst) 
{
	av_frame_unref(dst);
	dst->width = m_outWid;
	dst->height = m_outHei;
	dst->format = m_outFormat;
	int ret;
	if ((ret = av_frame_get_buffer(dst, 0)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_get_buffer:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}
	if ((ret = av_frame_make_writable(dst)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_make_writable:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return false;
	}

	if (imageConvert::operator()(src->data, src->linesize, dst->data, dst->linesize))
		return true;
	av_frame_unref(dst);
	return false;
}
#pragma once
#include "imageConvert.h"
#include <memory>
class PicToYuv :public imageConvert
{
public:
	PicToYuv() {};
	bool setInfo(int wid,int hei, int dep);
	bool operator()(const uint8_t* sData, long slen,std::shared_ptr<uint8_t>&data);
	bool operator()(const uint8_t* sData, long slen, AVFrame*dst);
	bool operator()(AVFrame* src, AVFrame* dst);
protected:
};


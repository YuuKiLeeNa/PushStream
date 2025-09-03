#include "H264Encoder.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/codec.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/pixfmt.h"
#ifdef __cplusplus
}
#endif
#include<memory>
#include<functional>
#include<algorithm>
#include<numeric>
#include<assert.h>

H264Encoder::H264Encoder() 
{
}

H264Encoder::~H264Encoder() 
{
	reset();
}

int H264Encoder::init(int w, int h, AVPixelFormat fmt, const std::string& encoderName, AVDictionary* dict)
{
	if (ctx_
		&& fmt == pix_
		&& w == w_
		&& h == h_
		&& encoderName == encoder_name_
		&& dict == dict_)
		return 0;

	std::unique_ptr<AVDictionary, std::function<void(AVDictionary*&)>>freeDict(dict, [](AVDictionary*& d)
		{
			av_dict_free(&d);
		});
	w_ = w;
	h_ = h;
	pix_ = fmt;
	encoder_name_ = encoderName;

	if(dict != dict_)
		av_dict_free(&dict_);
	
	AVDictionary* tmpdict = nullptr;
	av_dict_copy(&tmpdict, dict, 0);
	dict_ = tmpdict;


	const AVCodec* codec = nullptr;
	if (!encoderName.empty())
		codec = avcodec_find_encoder_by_name(encoderName.c_str());
	if (!codec)
		codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
		return AVERROR(EFAULT);

	if(!(ctx_ = avcodec_alloc_context3(codec)))
		return AVERROR(EFAULT);

	ctx_->width = w;
	ctx_->height = h;
	const AVPixelFormat* pix = codec->pix_fmts;
	for (; pix && *pix != AV_PIX_FMT_NONE && *pix != fmt; ++pix) {}
	if(!pix || ( pix && *pix != fmt))
		return AVERROR(EFAULT);;
	ctx_->pix_fmt = fmt;
	if (bitrate_ > 0)
		ctx_->bit_rate = bitrate_;
	ctx_->gop_size = gopsize_;
	ctx_->framerate = { framerate_ ,1};
	ctx_->time_base = { 1, framerate_ };
	ctx_->codec_type = AVMediaType::AVMEDIA_TYPE_VIDEO;
	ctx_->max_b_frames = 0;
	ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ctx_->rc_min_rate = bitrate_;
	ctx_->rc_max_rate = bitrate_;
	ctx_->rc_buffer_size = bitrate_;


	int ret;

	if ((ret = av_opt_set((void*)ctx_->priv_data, "profile", "baseline", 0)) != 0
	||(ret = av_opt_set((void*)ctx_->priv_data, "preset", "ultrafast", 0)) != 0
	||(ret = av_opt_set((void*)ctx_->priv_data, "tune", "zerolatency", 0)) != 0
	||(ret = av_opt_set_int(ctx_->priv_data, "forced-idr", 1, 0)) != 0
	|| (ret = av_opt_set_int(ctx_->priv_data, "avcintra-class", -1, 0)) != 0)
		return ret;

	if ((ret = avcodec_open2(ctx_, codec, &dict)) != 0) 
		return ret;

	return 0;
}

void H264Encoder::setbitrate(int bitrate) 
{
	bitrate_ = bitrate;
}

void H264Encoder::setframerate(int framerate) 
{
	framerate_ = framerate;
}

int H264Encoder::framerate()const
{
	if (ctx_)
		return ctx_->framerate.num / ctx_->framerate.den;
	return 0;
}
int H264Encoder::width()const
{
	if (ctx_)
		return ctx_->width;
	return 0;
}
int H264Encoder::height()const
{
	if (ctx_)
		return ctx_->height;
	return 0;
}

void H264Encoder::setgopsize(int gopsize) 
{
	gopsize_ = gopsize;
}

void H264Encoder::reset() 
{
	bitrate_ = 0;
	bitrate_ = 0;
	framerate_ = 25;
	gopsize_ = 25;

	w_ = 0;
	h_ = 0;
	pix_ = AV_PIX_FMT_NONE;
	encoder_name_ = "";
	av_dict_free(&dict_);


	if (ctx_)
		avcodec_free_context(&ctx_);


}

std::vector<uint8_t> H264Encoder::getExtendData()const
{
	return std::vector<uint8_t>(ctx_->extradata, ctx_->extradata + ctx_->extradata_size);
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>>H264Encoder::SPS_PPS(const std::vector<uint8_t>& data) 
{
	std::vector<uint8_t>sps;
	std::vector<uint8_t>pps;
	uint8_t dst[4] = {0, 0, 0, 1};
	auto iter1 = std::search(data.cbegin(), data.cend(), std::cbegin(dst), std::cend(dst));
	if (iter1 == data.cend())
		return std::pair<std::vector<uint8_t>, std::vector<uint8_t>>();
	iter1 += sizeof(dst);
	auto iter2 = std::search(iter1, data.cend(), std::cbegin(dst), std::cend(dst));
	if(iter2 == data.cend())
		return std::pair<std::vector<uint8_t>, std::vector<uint8_t>>();
	if ((* iter1 & 0x1f) == 0x07)
		sps.assign(iter1, iter2);
	else if((* iter1 & 0x1f) == 0x08)
		pps.assign(iter1, iter2);
	iter2 += 4;
	auto iter3 = std::search(iter2, data.cend(), std::cbegin(dst), std::cend(dst));
	if ((* iter2 & 0x1f) == 0x07)
		sps.assign(iter2, iter3);
	else if (( * iter2 & 0x1f) == 0x08)
		pps.assign(iter2, iter3);
	
	return { std::move(sps),std::move(pps) };
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> H264Encoder::getSpsPps() const
{
	if (ctx_)
		return SPS_PPS(std::vector<uint8_t>(ctx_->extradata, ctx_->extradata + ctx_->extradata_size));
	else
		return std::pair<std::vector<uint8_t>, std::vector<uint8_t>>();
}

std::vector<uint8_t> H264Encoder::SEQ_header(const std::pair<std::vector<uint8_t>, std::vector<uint8_t>>& data) 
{
	if (data.first.empty() || data.second.empty())
		return std::vector<uint8_t>();
	std::vector<uint8_t>sets(2 + 3 + 4 + 1 + 6 + data.first.size() + data.second.size(), 0);
	uint8_t* p = &sets[0];
	/**p++ = 0x17;
	*p++ = 0;
	*p++ = 0x01;
	*p++ = data.first[2];
	*p++ = data.first[3];
	*p++ = data.first[4];
	*p++ = 0xFF;
	*p++ = 0xE1;
	int size = data.first.size();
	*p++ = (size >> 8) & 0xFF;
	*p++ = size & 0xFF;
	memcpy(p, &data.first[0], size);
	p += size;
	size = data.second.size();
	*p++ = (size >> 8) & 0xFF;
	*p++ = size & 0xFF;
	memcpy(p, &data.second[0], size);
	assert(p - &sets[0] == sets.size());*/
	*p++ = 0x17;
	//0:sequence   1:nalu
	*p++ = 0;
	//3 composition time
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;
	//version
	*p++ = 0x01;
	*p++ = data.first[1];
	*p++ = data.first[2];
	*p++ = data.first[3];
	//111111   111	  nalu-1
	*p++ = 0xFF;
	//111     00001   sps个数
	*p++ = 0xE1;
	int size = data.first.size();
	//sps长度
	*p++ = (size >> 8) & 0xFF;
	*p++ = size & 0xFF;
	memcpy(p, &data.first[0], size);
	p += size;
	//pps个数
	*p++ = 0x01;
	//pps长度
	size = data.second.size();
	*p++ = (size >> 8) & 0xFF;
	*p++ = size & 0xFF;
	memcpy(p, &data.second[0], size);
	assert(p - &sets[0] == sets.size());
	return sets;
}

std::vector<uint8_t>H264Encoder::seqheader() 
{
	return SEQ_header(getSpsPps());
}

int H264Encoder::init(AVFrame* f) 
{
	return init(f->width, f->height, (AVPixelFormat)f->format, encoder_name_.c_str(), dict_);
}

bool H264Encoder::IsKeyFrame(const uint8_t* data, uint32_t size)
{
	if (size > 4) {
		//0x67:sps ,0x65:IDR, 0x6: SEI
		if (data[4] == 0x67 || data[4] == 0x65 ||
			data[4] == 0x6 || data[4] == 0x27) {
			return true;
		}
	}
	return false;
}
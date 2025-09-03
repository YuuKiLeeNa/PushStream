#include "AACEncoder.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/codec.h"
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif

#include "Util/logger.h"
#include "Network/Socket.h"



AACEncoder::AACEncoder()
{
}

AACEncoder::~AACEncoder() 
{
	reset();
}

int AACEncoder::init(int sampleRate, uint64_t ch_layout, AVSampleFormat fmt, const std::string& encoderName, AVDictionary* dict)
{
	if (ctx_
		&& sampleRate == sampleRate_
		&& ch_layout == ch_layout_
		&& fmt == fmt_
		&& encoderName == encoder_name_
		&& dict == dict_)
		return 0;
	sampleRate_ = sampleRate;
	ch_layout_ = ch_layout;
	fmt_ = fmt;
	encoder_name_ = encoderName;
	//channels_ = av_get_channel_layout_nb_channels(ch_layout);
	AVChannelLayout chlay;
	int err = av_channel_layout_from_mask(&chlay, ch_layout);
	if (err != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
		return -1;
	}

	if (dict_ != dict)
		av_dict_free(&dict_);
	AVDictionary* tmpdict = nullptr;
	av_dict_copy(&tmpdict, dict, 0);
	dict_ = tmpdict;
	const AVCodec* codec = nullptr;
	if (!encoderName.empty())
		codec = avcodec_find_encoder_by_name(encoderName.c_str());
	if (!codec) 
		codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!codec)
	{
		av_dict_free(&dict);
		return -1;
	}
	const int* pr;
	for (pr = codec->supported_samplerates; pr != NULL && *pr != 0 && *pr != sampleRate; ++pr) {}
	if (pr == NULL || *pr != sampleRate)
		return AVERROR(EFAULT);

	const AVSampleFormat* pf;// = codec->sample_fmts;
	for (pf = codec->sample_fmts; pf != NULL && *pf != AV_SAMPLE_FMT_NONE && *pf != fmt; ++pf) {}
	if (pf == NULL || *pf != fmt)
	{
		av_dict_free(&dict);
		return AVERROR(EFAULT);
	}
	//int ch = av_get_channel_layout_nb_channels(ch_layout);
	if (chlay.nb_channels <= 0)
	{
		av_dict_free(&dict);
		return AVERROR(EFAULT);
	}

	if (!(ctx_ = avcodec_alloc_context3(codec)))
	{
		av_dict_free(&dict);
		return AVERROR(EFAULT);
	}

	if (bitrate_ > 0)
		ctx_->bit_rate = bitrate_;
	//ctx_->channels = channels_;
	ctx_->sample_fmt = fmt;
	//ctx_->channel_layout = ch_layout;
	ctx_->ch_layout = chlay;
	chlay_ = chlay;

	ctx_->sample_rate = sampleRate;
	ctx_->codec_type = AVMediaType::AVMEDIA_TYPE_AUDIO;
	ctx_->time_base = {1, sampleRate };
	if (avcodec_open2(ctx_, codec, &dict) != 0) 
	{
		avcodec_free_context(&ctx_);
		av_dict_free(&dict);
		return -1;
	}
	/*AVDictionaryEntry* t = nullptr;
	while (t = av_dict_get(*dict, nullptr, t, 0)) 
	{
		av_dict_set(dict, t->key, nullptr, 0);
	}*/
	av_dict_free(&dict);
	return 0;
}

void AACEncoder::setbitrate(int bitrate) 
{
	bitrate_ = bitrate;
}

void AACEncoder::reset() 
{
	if (ctx_)
		avcodec_free_context(&ctx_);
	bitrate_ = 0;
	encoder_name_.clear();
	av_dict_free(&dict_);
	sampleRate_ = 0;
	//channels_ = 0;
	av_channel_layout_uninit(&chlay_);
	ch_layout_ = 0;
	fmt_ = AV_SAMPLE_FMT_NONE;
	bitrate_ = 0;

}

std::vector<uint8_t>AACEncoder::SEQ_HEADER(const std::vector<uint8_t>& extradate) 
{
	if (extradate.empty())
		return std::vector<uint8_t>();
	std::vector<uint8_t>sets(extradate.size()+2,0);
	sets[0] = 0x10 | 0x2<<2 | 0x1 << 1 | 0x1;
	memcpy(&sets[2], &extradate[0], extradate.size());
	return sets;
}

std::tuple<int, int, int>AACEncoder::PROFILE_SAMRATE_CH(const std::vector<uint8_t>& extradate) 
{
	return std::make_tuple<int, int, int>(extradate[0]>>3, (int)sample_rate_[(extradate[0] & 0x03) << 1 | extradate[1] >> 7], (extradate[1]>>3) & 0x04);
}

std::vector<uint8_t>AACEncoder::getextradata() 
{
	if (!ctx_)
		return std::vector<uint8_t>();
	return std::vector<uint8_t>(ctx_->extradata, ctx_->extradata+ ctx_->extradata_size);
}

int AACEncoder::channels()const 
{
	if (ctx_)
		return ctx_->ch_layout.nb_channels;
	else return 0;
}

int AACEncoder::framelen()const 
{
	if (ctx_)
		return ctx_->frame_size;
	else return 0;
}

int AACEncoder::samplerate()const 
{
	if (ctx_)
		return ctx_->sample_rate;
	else return 0;
}


int AACEncoder::init(AVFrame* f) 
{
	return init(sampleRate_, ch_layout_, fmt_, encoder_name_.c_str(), dict_);
}

const std::vector<int>AACEncoder::sample_rate_ = { 96000
 ,88200
, 64000
, 48000
, 44100
, 32000
, 24000
, 22050
, 16000
, 12000
, 11025
, 8000
, 7350
, 0
, 0
, 0 };




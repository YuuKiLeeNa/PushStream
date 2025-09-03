#include "utilEncoder.h"


#define make_map_x(codecid) {codecid, #codecid}

std::map<AVCodecID, std::string> global_codec_id
{
	make_map_x(AV_CODEC_ID_H264)
	, make_map_x(AV_CODEC_ID_HEVC)
	, make_map_x(AV_CODEC_ID_AAC)
	, make_map_x(AV_CODEC_ID_OPUS)
};

std::map<AVCodecID, std::string>getCodecId()
{
	return global_codec_id;
}

std::map<AVCodecID, std::string> getCodecTypeId(AVMediaType type)
{
	auto vec = getCodecId();
	decltype(vec) vtmp;
	for (auto& ele : vec)
		if (avcodec_get_type(ele.first) == type)
			vtmp.emplace(ele);
	return vtmp;
}

std::map<AVCodecID, std::string>getVideoCodecId()
{
	return getCodecTypeId(AVMediaType::AVMEDIA_TYPE_VIDEO);
}

std::map<AVCodecID, std::string>getAudioCodecId()
{
	return getCodecTypeId(AVMediaType::AVMEDIA_TYPE_AUDIO);
}

std::vector<const AVCodec*> getEncoderCodec(const std::vector<AVCodecID>& id)
{
	std::vector<const AVCodec*>sets;

	void* opaque = NULL;
	const AVCodec* pCodec;
	while (pCodec = av_codec_iterate(&opaque))
	{
		if (!av_codec_is_encoder(pCodec))
			continue;
		auto iter = std::find_if(id.cbegin(), id.cend(), [pCodec](std::remove_reference_t<decltype(id)>::const_reference&ele)
			{
				return ele == pCodec->id;
			});

		if (iter == id.cend())
			continue;

		if (check_soft_encoder(pCodec))
			sets.push_back(pCodec);
		else if (has_hw_config(pCodec))
			sets.push_back(pCodec);
	}

	return sets;
}


bool has_hw_config(const AVCodec* codec) {
	for (int i = 0;; i++) {
		const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
		if (!config) break;
		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX 
			&& check_hw_device_type(config->device_type))
			return true;
	}
	return false;
}


bool check_hw_device_type(AVHWDeviceType type) {
	AVHWDeviceType current = AV_HWDEVICE_TYPE_NONE;
	while ((current = av_hwdevice_iterate_types(current)) != AV_HWDEVICE_TYPE_NONE) {
		if (current == type) return true;
	}
	return false;
}


bool check_soft_encoder(const AVCodec* codec)
{
	const AVCodecHWConfig* config = avcodec_get_hw_config(codec, 0);
	return config == NULL;
}

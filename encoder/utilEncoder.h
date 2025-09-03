#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/codec_id.h"
#include "libavdevice/avdevice.h"
#include "libavcodec/codec.h"
#include "libavutil/hwcontext.h"
#ifdef __cplusplus
}
#endif
#include<vector>
#include<map>
#include<string>


std::map<AVCodecID, std::string>getCodecId();
std::map<AVCodecID, std::string>getCodecTypeId(AVMediaType type);

std::map<AVCodecID, std::string>getVideoCodecId();
std::map<AVCodecID, std::string>getAudioCodecId();

std::vector<const AVCodec*>getEncoderCodec(const std::vector<AVCodecID>&id);
bool has_hw_config(const AVCodec* codec);
bool check_hw_device_type(AVHWDeviceType type);
bool check_soft_encoder(const AVCodec*codec);



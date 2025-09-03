#pragma once
#include<map>
#include<string>
#ifdef __cplusplus
extern "C"{
#endif

#include "libavutil/rational.h"
#include "libavutil/pixfmt.h"
#include "libavutil/samplefmt.h"
#include "libavcodec/codec_id.h"

#ifdef __cplusplus
}
#endif
#include<sstream>


struct CMediaVariant : public std::string {
    template<typename T>
    inline CMediaVariant(const T& t) :
        std::string(std::to_string(t)) {
    }

    template<size_t N>
    CMediaVariant(const char(&s)[N]) :
        std::string(s, N) {
    }

    CMediaVariant(const char* cstr) :
        std::string(cstr) {
    }

    CMediaVariant(const std::string& other = std::string()) :
        std::string(other) {
    }

    template <typename T>
    inline operator T() const {
        return as<T>();
    }

    template<typename T>
    bool operator==(const T& t) const {
        return 0 == this->compare(CMediaVariant(t));
    }

    bool operator==(const char* t) const {
        return this->compare(t) == 0;
    }

    template <typename T>
    inline typename std::enable_if<!std::is_class<T>::value, T>::type as() const {
        return as_default<T>();
    }

private:
    template <typename T>
    T as_default() const {
        T t;
        std::stringstream ss;
        return ss << *this && ss >> t ? t : T();
    }
};

template<>
inline CMediaVariant::CMediaVariant(const AVRational& t)
{
    std::stringstream ss;
    ss << t.num << " " << t.den;
    *(std::string*)this = ss.str();
}

template <>
inline CMediaVariant::operator AVRational() const {
    //return as<T>();
    AVRational ral;
    std::stringstream ss(*this);
    ss >> ral.num >> ral.den;
    return ral;
}


template <>
inline bool CMediaVariant::as<bool>() const;

template <>
inline uint8_t CMediaVariant::as<uint8_t>() const;

template <>
inline AVPixelFormat CMediaVariant::as<AVPixelFormat>() const
{
    int fmt;
    std::stringstream ss(*this);
    ss >> fmt;
    return (AVPixelFormat)fmt;
}

template <>
inline AVSampleFormat CMediaVariant::as<AVSampleFormat>() const
{
    int fmt;
    std::stringstream ss(*this);
    ss >> fmt;
    return (AVSampleFormat)fmt;
}

template <>
inline AVCodecID CMediaVariant::as<AVCodecID>() const
{
    int id;
    std::stringstream ss(*this);
    ss >> id;
    return (AVCodecID)id;
}


/*
   option["b:a"] = 128000;
   option["ar"] = 48000;
   option["channel_layout"] = AV_CH_LAYOUT_STEREO;
   option["sample_fmt"] = AV_SAMPLE_FMT_FLTP;
   option["codecid:a"] = (AVCodecID)AV_CODEC_ID_AAC;
   option["b:vMin"] = 2560 * 1024;
   option["b:vMax"] = 5120 * 1024;
   option["aspect"] = AVRational{ 1,1 };
   option["codecid:v"] = (AVCodecID)AV_CODEC_ID_H265;
   option["s"] = AVRational{ (int)ScreenCapture_->GetWidth() ,(int)ScreenCapture_->GetHeight() };
   option["v:time_base"] = AVRational{ 1,framerate_ };
   option["r"] = AVRational{ framerate_,1 };
   option["pix_fmt"] = AV_PIX_FMT_YUV420P;
*/

class CMediaOption:protected std::map<std::string, CMediaVariant>//,public OPERATOR_
{
public:
	void av_opt_set(const std::string&name, const std::string&val);
	void av_opt_set_int(const std::string&name, int64_t val);
	void av_opt_set_double(const std::string&name, double val);
	void av_opt_set_q(const std::string&name, AVRational val);
	//void av_opt_set_bin(const std::string&name, const uint8_t* val, int size);
	void av_opt_set_pixel_fmt(const std::string&name, enum AVPixelFormat fmt);
	void av_opt_set_sample_fmt(const std::string&name, enum AVSampleFormat fmt);
    void av_opt_set_codec_id(const std::string& name, enum AVCodecID id);
    int av_opt_remove(const std::string& name);
	
	std::string av_opt_get(const std::string&name, const std::string& default_val);
	int64_t av_opt_get_int(const std::string&name, int64_t default_val);
	double av_opt_get_double(const std::string&name, double default_val);
	AVRational av_opt_get_q(const std::string&name, AVRational default_val);
	//void av_opt_get_bin(const std::string&name, const uint8_t* val, int size);
	AVPixelFormat av_opt_get_pixel_fmt(const std::string&name, enum AVPixelFormat default_fmt);
	AVSampleFormat av_opt_get_sample_fmt(const std::string&name, enum AVSampleFormat default_fmt);
    AVCodecID av_opt_get_codec_id(const std::string& name, enum AVCodecID default_id);
	using std::map<std::string, CMediaVariant>::operator[];
    using std::map<std::string, CMediaVariant>::clear;
};


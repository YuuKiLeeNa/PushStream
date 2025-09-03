#include "CMediaOption.h"

 void CMediaOption::av_opt_set(const std::string& name, const std::string& val) 
{
	(*this)[name] = val;
}

 void CMediaOption::av_opt_set_int(const std::string& name, int64_t val) {
	(*this)[name] = val;
}
 void CMediaOption::av_opt_set_double(const std::string& name, double val) {
	(*this)[name] = val;
}
 void CMediaOption::av_opt_set_q(const std::string& name, AVRational val) {
	(*this)[name] = val;
}
//void CMediaOption::av_opt_set_bin(const std::string& name, const uint8_t* val, int size) {
//
//}
 void CMediaOption::av_opt_set_pixel_fmt(const std::string& name, enum AVPixelFormat fmt) {
	(*this)[name] = fmt;
}
 void CMediaOption::av_opt_set_sample_fmt(const std::string& name, enum AVSampleFormat fmt) {
	(*this)[name] = fmt;
}

int CMediaOption::av_opt_remove(const std::string& name)
 {
	return (int)erase(name);
 }

void CMediaOption::av_opt_set_codec_id(const std::string& name, AVCodecID id)
{
	(*this)[name] = id;
}

 std::string CMediaOption::av_opt_get(const std::string& name, const std::string& default_val)
{
	auto iter = this->find(name);
	if (iter != end())
		return iter->second;
	else
		return default_val;
}
 int64_t CMediaOption::av_opt_get_int(const std::string& name, int64_t default_val)
{
	auto iter = this->find(name);
	if (iter != end())
		return iter->second;
	else
		return default_val;
}
 double CMediaOption::av_opt_get_double(const std::string& name, double default_val)
{
	auto iter = this->find(name);
	if (iter != end())
		return iter->second;
	else
		return default_val;
}
 AVRational CMediaOption::av_opt_get_q(const std::string& name, AVRational default_val)
{
	auto iter = this->find(name);
	if (iter != end())
		return iter->second;
	else
		return default_val;
}
//void av_opt_get_bin(const std::string&name, const uint8_t* val, int size);
 AVPixelFormat CMediaOption::av_opt_get_pixel_fmt(const std::string& name, enum AVPixelFormat default_fmt)
{
	auto iter = this->find(name);
	if (iter != end())
		return iter->second;
	else
		return default_fmt;
}

 AVSampleFormat CMediaOption::av_opt_get_sample_fmt(const std::string& name, enum AVSampleFormat default_fmt)
{
	auto iter = this->find(name);
	if (iter != end())
		return iter->second;
	else
		return default_fmt;
}

AVCodecID CMediaOption::av_opt_get_codec_id(const std::string& name, AVCodecID default_id)
{
	auto iter = this->find(name);
	if (iter != end())
		return iter->second;
	else
		return default_id;
}

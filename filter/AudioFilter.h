#pragma once
#include "FilterBase.h"
#include"Filter.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/channel_layout.h"
#include "libavutil/bprint.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavfilter/avfilter.h"
#include "libavutil/rational.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#ifdef __cplusplus
}
#endif
#include<mutex>
#include<map>
#include<sstream>
#include<vector>
#include<functional>
#include<algorithm>
#include<iomanip>

struct AudioFilter : Filter
{
	enum FILTER_TYPE :int { abuffer = 0,volume,amix,aformat,abuffersink };
	struct abufferFilter :FilterBase
	{
		abufferFilter(const AVRational &time_base, int sample_rate, AVSampleFormat sample_fmt, const AVRational& timebase, uint64_t channel_layout, CORE_DATA&core_data) :INHERIT(abuffer)
			, time_base_(timebase)
			, sample_rate_(sample_rate)
			, sample_fmt_(sample_fmt)
			, channel_layout_(channel_layout) {}
		AVRational time_base_;
		int sample_rate_;
		AVSampleFormat sample_fmt_;
		uint64_t channel_layout_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << RAT(time_base) << ":" << INT(sample_rate) << ":" << SAM(sample_fmt) << ":" << I64(channel_layout);
			return ss.str();
		}
	};
	struct volumeFilter :FilterBase 
	{
		static constexpr struct { AVSampleFormat f; int p; }XX[AV_SAMPLE_FMT_NB + 2] =
		{
			{AV_SAMPLE_FMT_NONE,-1}
			,{AV_SAMPLE_FMT_U8,0}          ///< unsigned 8 bits
				,{AV_SAMPLE_FMT_S16,0}           ///< signed 16 bits
				,{AV_SAMPLE_FMT_S32,0}           ///< signed 32 bits
				,{AV_SAMPLE_FMT_FLT,1}           ///< float
				,{AV_SAMPLE_FMT_DBL, 2}          ///< double
				,{AV_SAMPLE_FMT_U8P,0}           ///< unsigned 8 bits, planar
				,{AV_SAMPLE_FMT_S16P,0}          ///< signed 16 bits, planar
				,{AV_SAMPLE_FMT_S32P,0}          ///< signed 32 bits, planar
				,{AV_SAMPLE_FMT_FLTP,1}          ///< float, planar
				,{AV_SAMPLE_FMT_DBLP,2}          ///< double, planar
				,{AV_SAMPLE_FMT_S64, 0}          ///< signed 64 bits
				,{AV_SAMPLE_FMT_S64P,0}          ///< signed 64 bits, planar
				,{AV_SAMPLE_FMT_NB,-1}
		};
		volumeFilter(float vol, AVSampleFormat fmt, CORE_DATA& core_data) :INHERIT(volume)
			, volume_(vol)
			, precision_(volumeFilter::XX[fmt].p){}
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(volume) << "dB:" << INT(precision);
			return ss.str();
		}
		float volume_;
		int precision_;
	};
	struct amixFilter :FilterBase 
	{
		enum DURATION:int {
			longest = 0
			,shortest = 1
			,first = 2
		};
		amixFilter(int inputs, DURATION duration, float dropout_transition,const std::vector<float>&weights,CORE_DATA& core_data):INHERIT(amix) 
		, inputs_(inputs)
		, duration_(duration)
		, dropout_transition_(dropout_transition)
		, weights_(weights) {}
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(inputs) << ":" << INT(duration)<<":"<<INT(dropout_transition)<< makeString(weights_);
			return ss.str();
		}
		std::string makeString(const std::vector<float>& sets)
		{
			std::stringstream ss;
			std::for_each(sets.cbegin(), sets.cend(), [&ss](std::remove_reference<decltype(sets)>::type::const_reference v) {ss << v << " "; });
			std::string str = ss.str();
			if (!str.empty())
				str = str.substr(0, str.length() - 1);
			return str;
		}
		int inputs_;
		DURATION duration_;
		float dropout_transition_; 
		std::vector<float>weights_;
	};
	struct aformatFilter :FilterBase
	{
		aformatFilter(AVSampleFormat f, int r, uint64_t cl, CORE_DATA& core_data) :INHERIT(aformat)
		,f_(f)
		,r_(r)
		,cl_(cl){}
		std::string ff() override
		{
			std::stringstream ss;
			ss << SAM(f) << ":" << INT(r) << ":" << I64(cl);
			return ss.str();
		}
		AVSampleFormat f_;
		int r_;
		uint64_t cl_;
	};
	struct abuffersinkFilter :FilterBase
	{
		abuffersinkFilter(int sample_rates,AVSampleFormat sample_fmts,uint64_t channel_layouts, CORE_DATA& core_data) :INHERIT(abuffersink)
			, sample_rates_(sample_rates)
			, sample_fmts_(sample_fmts)
			, channel_layouts_(channel_layouts) {}
		int sample_rates_;
		AVSampleFormat sample_fmts_;
		uint64_t channel_layouts_;
		std::string ff() override
		{
			return "";
			//std::stringstream ss;
			//ss <<INT(sample_rates) << ":" << SAM(sample_fmts) << ":" << I64(channel_layouts);
			//return ss.str();
		}
		virtual int specialOp()override 
		{

			if (av_opt_set(filter_->priv, "sample_rates", std::to_string(sample_rates_).c_str(), 0)
				|| av_opt_set(filter_->priv, "sample_fmts", av_get_sample_fmt_name(sample_fmts_), 0)
				|| av_opt_set(filter_->priv, "channel_layouts", (std::stringstream()<<"0x"<< std::hex<< channel_layouts_).str().c_str(), 0))
				return -1;
			return 0;
		}
	};
};

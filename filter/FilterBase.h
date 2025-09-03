#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/pixfmt.h"
#include "libavutil/pixdesc.h"
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
#include<atomic>

struct FilterBase:std::enable_shared_from_this<FilterBase>
{
	struct CORE_DATA
	{
		CORE_DATA() :graph_(avfilter_graph_alloc(), [](AVFilterGraph*& f) {avfilter_graph_free(&f); }) {}
		static std::shared_ptr<AVFilterGraph> create_graph();
		std::mutex mutex_;
		std::map<std::string, std::shared_ptr<FilterBase>>filters_;
		std::shared_ptr<AVFilterGraph>graph_;
	};
#define A_(x)  x##_
#define INT(x) #x##"="<<A_(x)
#define I64(x) #x##"=0x"<<std::hex<<A_(x)<<std::dec
#define RAT(x) #x##"="<<A_(x).num<<"/"<<A_(x).den
#define PIX(x) #x##"="<<av_get_pix_fmt_name(A_(x))
#define SAM(x) #x##"="<<av_get_sample_fmt_name(A_(x))
#define SQU(x) #x##"='"<<A_(x)<<'\''
	///////////////////////////////////////////////
#define INHERIT(x)	FilterBase(#x, #x +std::to_string(++sindex_), FILTER_TYPE::x, core_data)
#define INHERIT_INDEX(x,index)	FilterBase(#x, #x +std::to_string(index), FILTER_TYPE::x, core_data)
////////////////////////////////////////////////////////////
//#define mutex_ core_data_.mutex_
//#define filters_ core_data_.filters_
//#define graph_ core_data_.graph_

	FilterBase(const std::string& sname, const std::string& name, int type, CORE_DATA& core_data) :sname_(sname)
		, name_(name)
		, type_(type)
		, core_data_(core_data) {};
	virtual ~FilterBase() {};
	virtual std::string ff() = 0;
	virtual int specialOp() { return 0; };
	int addFilter();
	int createFilter(AVFilterContext** filter = nullptr);
	int send_commend(const std::string& cmd, const std::string& args);
	
	const std::string sname_;
	const std::string name_;
	int type_;
	AVFilterContext* filter_ = nullptr;
	static std::atomic_ullong sindex_;
	CORE_DATA& core_data_;
};


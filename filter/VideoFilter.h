#pragma once
#include "FilterBase.h"
#include "Filter.h"
extern "C"
{
#include "libavutil/pixfmt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/rational.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
}
#include<vector>
#include<sstream>
#include<atomic>
#include<memory>

struct VideoFilter: Filter
{
	enum FILTER_TYPE :int {buffer=0, movie, rotate, scale, scale_cuda, drawtext, overlay, crop, vflip, hflip, split, format,buffersink};
	struct bufferFilter :FilterBase
	{
		bufferFilter(int w, int h, const AVRational& sar, const AVRational& timebase, const AVRational& framerate, AVPixelFormat fmt,CORE_DATA& core_data) :INHERIT(buffer)
			, width_(w)
			, height_(h)
			, sar_(sar)
			, time_base_(timebase)
			, frame_rate_(framerate)
			, pix_fmt_(fmt) {}
		bufferFilter(int w, int h, const AVRational& sar, const AVRational& timebase, const AVRational& framerate, AVPixelFormat fmt, int index, CORE_DATA& core_data) :INHERIT_INDEX(buffer, index)
			, width_(w)
			, height_(h)
			, sar_(sar)
			, time_base_(timebase)
			, frame_rate_(framerate)
			, pix_fmt_(fmt) {}
		int width_;
		int height_;
		AVRational sar_;
		AVRational time_base_;
		AVRational frame_rate_;
		AVPixelFormat pix_fmt_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(width)<<":"<< INT(height)<<":"<< PIX(pix_fmt) << ":" << RAT(sar)<<":"<< RAT(time_base)<<":"<< RAT(frame_rate);
			return ss.str();
		}
	};
	struct movieFilter :FilterBase
	{
		movieFilter(const std::string& filename, int loop, int discontinuity, CORE_DATA& core_data) :INHERIT(movie)
			, filename_(filename)
			, loop_(loop)
			, discontinuity_(discontinuity){}
		movieFilter(const std::string& filename, int loop, int discontinuity, int index,CORE_DATA& core_data) :INHERIT_INDEX(movie,index)
			, filename_(filename)
			, loop_(loop)
			, discontinuity_(discontinuity) {}
		std::string filename_;
		int loop_;
		int discontinuity_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << SQU(filename)<<":"<<INT(loop)<<":"<<INT(discontinuity);
			return ss.str();
		}
	};
	struct rotateFilter :FilterBase
	{
		rotateFilter(const std::string &angle, const std::string& ow, const std::string& oh, const std::string& fillcolor, int bilinear, CORE_DATA& core_data) :INHERIT(rotate)
		, angle_(angle)
		, ow_(ow)
		, oh_(oh)
		, fillcolor_(fillcolor)
		, bilinear_(bilinear){}
		rotateFilter(const std::string& angle, const std::string& ow, const std::string& oh, const std::string& fillcolor, int bilinear, int index,CORE_DATA& core_data) :INHERIT_INDEX(rotate, index)
			, angle_(angle)
			, ow_(ow)
			, oh_(oh)
			, fillcolor_(fillcolor)
			, bilinear_(bilinear) {}
		std::string angle_;
		std::string ow_;
		std::string oh_;
		std::string fillcolor_;
		int bilinear_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(angle) << ":" << INT(ow) << ":" << INT(oh)<<":"<< INT(fillcolor);
			return ss.str();
		}
	};
	struct scaleFilter :FilterBase
	{
		scaleFilter(int w, int h,CORE_DATA&core_data) :INHERIT(scale)
		,w_(w)
		,h_(h) {}
		scaleFilter(int w, int h, int index,CORE_DATA& core_data) :INHERIT_INDEX(scale,index)
			, w_(w)
			, h_(h) {}
		int w_;
		int h_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(w) << ":" << INT(h);
			return ss.str();
		}
	}; 
	struct scale_cudaFilter :FilterBase 
	{
		scale_cudaFilter(int w, int h, int interp_algo, AVPixelFormat format,CORE_DATA& core_data) :INHERIT(scale_cuda)
			, w_(w)
			, h_(h) 
			, interp_algo_(interp_algo)
			, format_(format)
			{}
		scale_cudaFilter(int w, int h, int interp_algo, AVPixelFormat format, int index,CORE_DATA& core_data) :INHERIT_INDEX(scale_cuda, index)
			, w_(w)
			, h_(h)
			, interp_algo_(interp_algo)
			, format_(format)
		{}
		int w_;
		int h_;
		int interp_algo_;
		AVPixelFormat format_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(w) << ":" << INT(h) << ":"<<INT(interp_algo) << ":" << PIX(format);
			return ss.str();
		}


	};
	struct drawtextFilter :FilterBase
	{
		drawtextFilter(const std::string&fontfile
			, const std::string& text
			, const std::string& fontcolor
			, const std::string& fontsize
			, const std::string& x
			, const std::string& y
			, int box
			, const std::string& boxcolor
			,int boxborderw
			,int line_spacing
			,CORE_DATA& core_data) :INHERIT(drawtext)
			,fontfile_(fontfile)
			,text_(text)
			,fontcolor_(fontcolor)
			,fontsize_(fontsize)
			,x_(x)
			,y_(y)
			,box_(box)
			,boxcolor_(boxcolor)
			,boxborderw_(boxborderw)
			,line_spacing_(line_spacing){}
		drawtextFilter(const std::string& fontfile
			, const std::string& text
			, const std::string& fontcolor
			, const std::string& fontsize
			, const std::string& x
			, const std::string& y
			, int box
			, const std::string& boxcolor
			, int boxborderw
			, int line_spacing
			, int index
			, CORE_DATA& core_data) :INHERIT_INDEX(drawtext,index)
			, fontfile_(fontfile)
			, text_(text)
			, fontcolor_(fontcolor)
			, fontsize_(fontsize)
			, x_(x)
			, y_(y)
			, box_(box)
			, boxcolor_(boxcolor)
			, boxborderw_(boxborderw)
			, line_spacing_(line_spacing) {}
		//drawtext=fontfile='C\\:/Windows/Fonts/arial.ttf'
		//:text='my kotori chan':fontcolor=0xff000070:
		//fontsize=88:x=200:y=200:box=1:
		//boxcolor=red@0.2:boxborderw=10:line_spacing=10
		std::string fontfile_;
		std::string text_;
		std::string fontcolor_;
		std::string fontsize_;
		std::string x_;
		std::string y_;
		int box_;
		std::string boxcolor_;
		int boxborderw_;
		int line_spacing_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << SQU(fontfile) << ":" << SQU(text) << ":" << INT(fontcolor) << ":" << INT(x) << ":" << INT(y) << ":" << INT(box) << ":" << INT(boxcolor) << ":" << INT(boxborderw) << ":" << INT(line_spacing);
			return ss.str();
		}
	};
	struct overlayFilter :FilterBase
	{
		overlayFilter(const std::string&x, const std::string&y,CORE_DATA& core_data) :INHERIT(overlay)
		,x_(x)
		,y_(y){}
		overlayFilter(const std::string& x, const std::string& y, int index,CORE_DATA& core_data) :INHERIT_INDEX(overlay, index)
			, x_(x)
			, y_(y) {}
		std::string x_;
		std::string y_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(x) << ":" << INT(y);
			return ss.str();
		}
	}; 
	struct cropFilter :FilterBase
	{
		cropFilter(const std::string&w, const std::string&h, const std::string&x, const std::string&y, CORE_DATA& core_data) :INHERIT(crop)
		,w_(w)
		,h_(h)
		,x_(x)
		,y_(y) {}
		cropFilter(const std::string& w, const std::string& h, const std::string& x, const std::string& y, int index,CORE_DATA& core_data) :INHERIT_INDEX(crop,index)
			, w_(w)
			, h_(h)
			, x_(x)
			, y_(y) {}
		std::string w_;
		std::string h_;
		std::string x_;
		std::string y_;
		std::string ff() override
		{
			std::stringstream ss;
			ss << INT(w)<<":" << INT(h) << ":" << INT(x) << ":" << INT(y);
			return ss.str();
		}
	};
	struct vflipFilter :FilterBase
	{
		vflipFilter(CORE_DATA& core_data) :INHERIT(vflip) {}
		vflipFilter(int index,CORE_DATA& core_data) :INHERIT_INDEX(vflip,index) {}
		std::string ff() override{return ""; }
	};
	struct hflipFilter :FilterBase
	{
		hflipFilter(CORE_DATA& core_data) :INHERIT(hflip) {}
		hflipFilter(int index, CORE_DATA& core_data) :INHERIT_INDEX(hflip,index) {}
		std::string ff() override{return "";}
	};
	struct splitFilter :FilterBase
	{
		splitFilter(int outputs, CORE_DATA& core_data) :INHERIT(split), outputs_(outputs) {}
		splitFilter(int outputs, int index,CORE_DATA& core_data) :INHERIT_INDEX(split,index), outputs_(outputs) {}
		int outputs_;
		std::string ff() override { std::stringstream ss; ss << INT(outputs); return ss.str(); }
	};
	struct formatFilter :FilterBase
	{
		formatFilter(AVPixelFormat pix_fmts, CORE_DATA& core_data) :INHERIT(format)
			, pix_fmts_(pix_fmts) {}
		formatFilter(AVPixelFormat pix_fmts, int index,CORE_DATA& core_data) :INHERIT_INDEX(format,index)
			, pix_fmts_(pix_fmts) {}
		AVPixelFormat pix_fmts_;
		std::string ff() override { std::stringstream ss; ss << PIX(pix_fmts); return ss.str(); }
	};
	struct buffersinkFilter :FilterBase
	{
		buffersinkFilter(AVPixelFormat pix_fmts,CORE_DATA& core_data) :INHERIT(buffersink)
		, pix_fmts_(pix_fmts) {}
		buffersinkFilter(AVPixelFormat pix_fmts, int index,CORE_DATA& core_data) :INHERIT_INDEX(buffersink,index)
			, pix_fmts_(pix_fmts) {}
		AVPixelFormat pix_fmts_;
		std::string ff() override { return "";/* << PIX(pix_fmts);*/ }
		virtual int specialOp() override 
		{
			return av_opt_set(filter_->priv, "pix_fmts", av_get_pix_fmt_name(pix_fmts_), 0);
		}
	};
};


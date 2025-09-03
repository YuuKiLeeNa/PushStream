#include "AudioMixer.h"
#include<functional>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/channel_layout.h"
#include "libavutil/buffer.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/frame.h"
#include "libavutil/bprint.h"
#include "libavutil/opt.h"
#include "libavutil/audio_fifo.h"

#ifdef __cplusplus
}
#endif
#include<sstream>
#include<iterator>
#include<numeric>
#include<algorithm>
#include<iomanip>
#include "CAudioFrameCast.h"
#include "Util/logger.h"
#include "Network/Socket.h"
//#include "volume-control.hpp"
#include "define_type.h"


AudioMixer::AudioMixer() :m_out{0}
{
}

//AudioMixer::AudioMixer(const std::vector<AudioMixer::AudioInfo>& in, const AudioMixer::AudioInfo& out)
//{
//	setData(in, out);
//}

//AudioMixer::AudioMixer(const AudioMixer::AudioInfo& in1, const AudioMixer::AudioInfo& in2, const AudioMixer::AudioInfo& out) :AudioMixer({in1,in2}, out)
//{
//}

int AudioMixer::addInput(int sample_rate, uint64_t channel_layout, AVSampleFormat fmt, float weights, float volume)
{
	std::lock_guard<std::mutex>lock(data_.mutex_);
	AudioInfo audioinfo;
	audioinfo.channel_layout = channel_layout;
	audioinfo.fmt = fmt;
	audioinfo.sampleRate = sample_rate;
	audioinfo.timebase = {1,sample_rate };
	audioinfo.volume = volume;
	audioinfo.weights = weights;
	decltype(m_inSets)::const_iterator iter;
	do
	{
		audioinfo.nameIndex = m_abuffer++;
		iter = std::find_if(m_inSets.cbegin(), m_inSets.cend(), [&audioinfo](decltype(m_inSets)::const_reference v) {return v.nameIndex == audioinfo.nameIndex; });
	} while (iter != m_inSets.cend());
	m_inSets.push_back(audioinfo);
	return audioinfo.nameIndex;
}

//int AudioMixer::addInput(const AudioMixer::AudioInfo& inputs) 
//{
//	m_inSets.push_back({ nullptr,inputs });
//	return ;
//}

void AudioMixer::setOutput(int sample_rate, uint64_t channel_layout, AVSampleFormat fmt)
{
	std::lock_guard<std::mutex>lock(data_.mutex_);
	m_out.channel_layout = channel_layout;
	m_out.fmt = fmt;
	m_out.sampleRate = sample_rate;
	m_out.timebase = { 1,sample_rate };
}

//void AudioMixer::setOutput(const AudioMixer::AudioInfo& output) 
//{
//	m_out.info = output;
//	m_out.ctx = nullptr;
//}

AudioMixer::~AudioMixer()
{
	reset();
}

void AudioMixer::reset()
{
	std::lock_guard<std::mutex>lock(data_.mutex_);
	//m_volumeCtx.clear();
	//m_amixCtx = nullptr;
	m_inSets.clear();
	m_abuffer = 0;
	ctx_.clear();
	//m_volume = 0;
	//m_aformat = 0;
	//m_amix = 0;
	//m_abuffersink = 0;
	memset(&m_out, 0, sizeof(m_out));
	m_out.fmt = AV_SAMPLE_FMT_NONE;
	data_.graph_.reset();
	if (fifo_)
	{
		av_audio_fifo_free(fifo_);
		fifo_ = nullptr;
	}
	fifofmt_ = AV_SAMPLE_FMT_NONE;
	fifochannels_ = 0;
	fifosamplerate_ = 0;
	//m_out.info.fmt = AV_SAMPLE_FMT_NONE;
	//if(data_.graph_)
	//	avfilter_graph_free(&data_.graph_));
}

int AudioMixer::init(AudioMixer::MixerMode mode)
{
	std::lock_guard<std::mutex>lock(data_.mutex_);
	m_mode = mode;
	int ret;
	if (!(data_.graph_ = FilterBase::CORE_DATA::create_graph()))
	{
		PrintE("avfilter_graph_alloc return nullptr");
		return -1;
	}
	std::vector<std::unique_ptr<AVFilterContext, std::function<void(AVFilterContext*&)>>>sets;
	decltype(sets)::value_type::deleter_type freeCtx= [](AVFilterContext* &f)
	{
		avfilter_free(f);
		f = nullptr;
	};

#define MAKE_UNIQUE(x) decltype(sets)::value_type(x, freeCtx)
#define PUSH_V(x) sets.push_back(MAKE_UNIQUE(x))

	if ((ret = initabuffersink(&sink_)) < 0)
		return ret;
	PUSH_V(sink_);
	int size = m_inSets.size();
	AVFilterContext* amixCtx = nullptr;
	std::vector<float>weights;
	weights.reserve(size);
	std::for_each(m_inSets.cbegin(), m_inSets.cend(), [&weights](decltype(m_inSets)::const_reference v) {weights.push_back(v.weights); });
	if ((ret = initamix(&amixCtx, size, m_mode, 3, weights)) < 0)
		return ret;
	PUSH_V(amixCtx);
	//m_amixCtx = amixCtx;
	if ((ret = avfilter_link(amixCtx, 0, sink_, 0)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_link error:%s", av_make_error_string(szErr,sizeof(szErr), ret));
		return ret;
	}
	sets.reserve(size * 2 + 1 + 1);

	AVChannelLayout out_chlay;
	int err = av_channel_layout_from_mask(&out_chlay, m_out.channel_layout);
	if (err != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
		return -1;
	}

	for (int i = 0;i < size; ++i)
	{
		AVFilterContext* abufferCtx = nullptr;
		if ((ret = initabuffer(&abufferCtx, m_inSets[i].timebase, m_inSets[i].sampleRate, m_inSets[i].channel_layout, m_inSets[i].fmt, m_inSets[i].nameIndex)) < 0)
			return ret;
		PUSH_V(abufferCtx);

		AVChannelLayout chlay;
		int err = av_channel_layout_from_mask(&chlay, m_inSets[i].channel_layout);
		if (err != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
			return -1;
		}

		AVFilterContext* aresampleCtx = nullptr;
		if ((ret = initaresample(&aresampleCtx
			, chlay.u.mask//av_get_channel_layout_nb_channels(m_inSets[i].channel_layout)
			, out_chlay.u.mask//av_get_channel_layout_nb_channels(m_out.channel_layout)
			, m_inSets[i].sampleRate, m_out.sampleRate, m_inSets[i].fmt, m_out.fmt, m_inSets[i].channel_layout, m_out.channel_layout, m_inSets[i].nameIndex)) < 0)
			return ret;
		PUSH_V(aresampleCtx);

		//AVFilterContext* aformatCtx = nullptr;
		//if ((ret = initaformat(&aformatCtx, m_out.sampleRate, m_out.channel_layout, m_out.fmt, m_inSets[i].nameIndex)) < 0)
		//	return ret;
		//PUSH_V(aformatCtx);
		AVFilterContext* volumeCtx = nullptr;
		if ((ret = initvolume(&volumeCtx, getPrecision(m_inSets[i].fmt), m_inSets[i].volume, m_inSets[i].nameIndex)) != 0)
			return ret;
		PUSH_V(volumeCtx);
		//m_volumeCtx.push_back(volumeCtx);
		if ((ret = avfilter_link(abufferCtx, 0, aresampleCtx, 0)) != 0
			|| (ret = avfilter_link(aresampleCtx, 0, volumeCtx, 0)) != 0
			|| (ret = avfilter_link(volumeCtx, 0, amixCtx, i)) != 0) 
		/*if ((ret = avfilter_link(abufferCtx, 0, volumeCtx, 0)) != 0
			|| (ret = avfilter_link(volumeCtx, 0, amixCtx, i)) != 0)*/
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("avfilter_link error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
			return ret;
		}
		ctx_[m_inSets[i].nameIndex] = abufferCtx;
	}
	if ((ret = avfilter_graph_config(data_.graph_.get(), nullptr)) < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_config error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	if ((ret = initFifo()) < 0) 
		return ret;
	std::for_each(sets.begin(), sets.end(), [](decltype(sets)::reference v) {v.release(); });
	return 0;
}

//int AudioMixer::init(const std::vector<AudioMixer::AudioInfo>& in, const AudioMixer::AudioInfo& out,AudioMixer::MixerMode mode)
//{
//	m_mode = mode;
//	reset();
//	setData(in,out);
//	return init(mode);
//}

//int AudioMixer::init(const AudioMixer::AudioInfo& in1, const AudioMixer::AudioInfo& in2, const AudioMixer::AudioInfo& out, AudioMixer::MixerMode mode)
//{
//	m_mode = mode;
//	reset();
//	setData({in1,in2}, out);
//	return init(mode);
//}

//int AudioMixer::addFrame(int indexOfFilter, AVFrame* pushFrame, bool isKeepRef)
//{
//	int ret;
//	int flags = AV_BUFFERSRC_FLAG_PUSH;
//	if (isKeepRef)
//		flags |= AV_BUFFERSRC_FLAG_KEEP_REF;
//	{
//		std::lock_guard<std::mutex>lock(mutex_);
//		auto iter = ctx_.find(indexOfFilter);
//		if (iter == ctx_.end())
//		{
//			if (flags & AV_BUFFERSRC_FLAG_KEEP_REF)
//				av_frame_unref(pushFrame);
//			return AVERROR(ERANGE);
//		}
//		ret = av_buffersrc_add_frame_flags(iter->second, pushFrame, flags);
//	}
//	if (ret < 0)
//	{
//		makeErrStr(ret, "av_buffersrc_add_frame_flags error:");
//		return ret;
//	}
//	return 0;
//}
//

int AudioMixer::getFrame(AVFrame* f, int nb_samples)
{
	int ret = 0;
	{
		std::lock_guard<std::mutex>lock(data_.mutex_);
		if (sink_)
		{
			AVFrame* tmpf = av_frame_alloc();
			if (!tmpf) 
			{
				PrintE("av_frame_alloc error:no memory");
				return AVERROR(ENOMEM);
			}
			UPTR_FME free_tmpf(tmpf);
			ret = av_buffersink_get_frame_flags(sink_, tmpf, 0 /*AV_BUFFERSINK_FLAG_NO_REQUEST*/);
			if (ret < 0) 
			{
				if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_buffersink_get_frame_flags error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}
			}
			else
			{
				int fifosize = av_audio_fifo_size(fifo_);
				assert(fifosize >= 0);
				if ((ret = av_audio_fifo_realloc(fifo_, fifosize + tmpf->nb_samples + 1)) != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_audio_fifo_realloc error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}

				if ((ret = av_audio_fifo_write(fifo_, (void**)tmpf->data, tmpf->nb_samples)) != tmpf->nb_samples)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_audio_fifo_write error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}
			}
			
			assert(f->data[0] == NULL);

			int fifosize = av_audio_fifo_size(fifo_);
			if (fifosize >= nb_samples || (fifosize > 0 && ret == AVERROR_EOF))
			{
				f->sample_rate = av_buffersink_get_sample_rate(sink_);
				int err = av_buffersink_get_ch_layout(sink_, &f->ch_layout);
				if (err != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_buffersink_get_ch_layout error:%s", av_make_error_string(szErr, AV_ERROR_MAX_STRING_SIZE, err));
					return err;
				}

				f->format = av_buffersink_get_format(sink_);

				if (ret == AVERROR_EOF && fifosize < nb_samples)
					nb_samples = fifosize;
				f->nb_samples = nb_samples;
				f->time_base = { 1, f->sample_rate };

				if ((ret = av_frame_get_buffer(f, 0)) < 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_frame_get_buffer:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}

				if ((ret = av_frame_make_writable(f)) != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_frame_make_writable:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}

				if ((ret = av_audio_fifo_read(fifo_, (void**)f->data, f->nb_samples)) != f->nb_samples)
				{
					av_frame_unref(f);
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_audio_fifo_read error :%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}

				if ((ret = av_audio_fifo_realloc(fifo_, fifosize - f->nb_samples + 1)) != 0)
				{
					av_frame_unref(f);
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_audio_fifo_realloc error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}
				return 0;
			}
			else if (ret == AVERROR_EOF)
				return AVERROR_EOF;
			else
				return AVERROR(EAGAIN);
		}
		else 
		{
			return AVERROR_INVALIDDATA;;
		}
	}
}

int AudioMixer::setVolumeDB(int indexOfFilter, float volume) 
{
	std::lock_guard<std::mutex>lock(data_.mutex_);
	if (!data_.graph_ || m_inSets.empty())
		return -1;

	int ret = avfilter_graph_send_command(data_.graph_.get(), ("volume" + std::to_string(indexOfFilter)).c_str(), "volume", (std::to_string(volume)+"dB").c_str(), nullptr, 0, 0);
	if (ret < 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_send_command error:ret=%s", av_make_error_string(szErr, sizeof(szErr), ret));
	}
	/*else
	{
		if ((ret = av_opt_set(m_volumeCtx[indexOfFilter]->priv, "volume", std::to_string(volume).c_str(), 0)) < 0) 
			makeErrStr(ret, "av_opt_set for volume error:");
	}*/
	return ret;
}

//int AudioMixer::getVolume(int indexOfFilter, float *volume) 
//{
//	uint8_t* v = nullptr;
//	std::lock_guard<std::mutex>lock(mutex_);
//	int ret = av_opt_get(m_volumeCtx[indexOfFilter]->priv, "volume",0, &v);
//	if (ret < 0)
//		makeErrStr(ret, "av_opt_set for volume error:");
//	else
//	{
//		*volume = std::stof(std::string((const char*)v));
//		av_free(v);
//	}
//	return ret;
//}

int AudioMixer::setWeights(const std::vector<float>&weights)
{
	std::lock_guard<std::mutex>lock(data_.mutex_);
	if (weights.size() != m_inSets.size())
	{
		PrintE("size doesn't match needing");
		return -1;
	}
	std::string str = makeString(weights);

	int ret = avfilter_graph_send_command(data_.graph_.get(), "amix1", "weights", str.c_str(), nullptr, 0, 0);
	if (ret < 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_send_command for amix1 error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	}
	/*else 
	{
		if ((ret = av_opt_set(m_amixCtx->priv, "weights", str.c_str(), 0)) < 0) 
			makeErrStr(ret, "av_opt_set error:");
	}*/
	return ret;
}

int AudioMixer::getInputAudioStream()
{
	std::lock_guard<std::mutex>lock(data_.mutex_);
	return m_inSets.size();
}

//int AudioMixer::getWeights(std::vector<float>* weights) 
//{
//	std::lock_guard<std::mutex>lock(mutex_);
//	uint8_t* v = nullptr;
//	int ret = av_opt_get(m_amixCtx->priv, "weights", 0, &v);
//	if (ret < 0)
//		makeErrStr(ret, "av_opt_set for weights error:");
//	else
//	{
//		weights->clear();
//		weights->reserve(m_inSets.size());
//		std::istringstream ss((const char*)v);
//		av_free(v);
//		std::copy(std::istream_iterator<float>(ss), std::istream_iterator<float>(), std::back_inserter(*weights));
//	}
//	return ret;
//}
//void AudioMixer::freeFilter(AudioMixer::AudioFilter*audio_filter)
//{
//	avfilter_free(audio_filter->ctx);
//	memset(&audio_filter->info, 0, sizeof(audio_filter->info));
//	audio_filter->info.fmt = AV_SAMPLE_FMT_NONE;
//}

//void AudioMixer::setData(const std::vector<AudioMixer::AudioInfo>& in, const AudioMixer::AudioInfo& out) 
//{
//	reset();
//	std::lock_guard<std::mutex>lock(mutex_);
//	const int inSize = in.size();
//	m_inSets.resize(inSize);
//	for (int i = 0; i < inSize; ++i)
//		m_inSets.push_back({nullptr, in[i] });
//	m_out.info = out;
//	m_out.ctx = nullptr;
//}

int AudioMixer::initabuffer(AVFilterContext** ctx, const AVRational& timebase, int rample_rate, uint64_t channel_layout, AVSampleFormat fmt,int nameindex)
{
	int ret;
	*ctx = nullptr;
	const AVFilter*filter = avfilter_get_by_name("abuffer");
	if (!filter)
	{
		PrintE("avfilter_get_by_name error for abuffer");
		return -1;
	}
	AVBPrint buffer;
	av_bprint_init(&buffer, 128, AV_BPRINT_SIZE_UNLIMITED);

	av_bprintf(&buffer, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%llx", timebase.num, timebase.den, rample_rate, av_get_sample_fmt_name(fmt), channel_layout);
	
	ret = avfilter_graph_create_filter(ctx, filter, ("abuffer" + std::to_string(nameindex)).c_str(), buffer.str, nullptr, data_.graph_.get());
	if ((ret = av_bprint_finalize(&buffer, nullptr)) != 0)
		return ret;
	if (ret < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_create_filter error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	return 0;
}

int AudioMixer::initaresample(AVFilterContext** ctx, uint64_t inchlay, uint64_t outchlay, int insamplerate, int outsamplerate, AVSampleFormat infmt, AVSampleFormat outfmt, uint64_t in_ch_layout, uint64_t out_ch_layout, int nameindex)
{
	int ret;
	*ctx = nullptr;
	const AVFilter* filter = avfilter_get_by_name("aresample");
	if (!filter)
		return -1;
	std::stringstream ss;


	AVChannelLayout inLayout, outLayout;

	if ((ret = av_channel_layout_from_mask(&inLayout, in_ch_layout)) != 0
		|| (ret=av_channel_layout_from_mask(&outLayout, out_ch_layout)) != 0) 
	{
		PrintE("av_channel_layout_from_mask error");
		return -1;
	}

	char szInLayout[AV_ERROR_MAX_STRING_SIZE], szOutLayout[AV_ERROR_MAX_STRING_SIZE];
	if ((ret = av_channel_layout_describe(&inLayout, szInLayout, sizeof(szInLayout))) < 0
		|| (ret = av_channel_layout_describe(&outLayout, szOutLayout, sizeof(szOutLayout))) < 0)
	{
		PrintE("av_channel_layout_describe error");
		return -1;
	}


	ss << "in_sample_rate=" << insamplerate << ":out_sample_rate=" << outsamplerate << ":in_sample_fmt=" << av_get_sample_fmt_name(infmt) << ":out_sample_fmt=" << av_get_sample_fmt_name(outfmt) << ":in_chlayout=" << szInLayout << ":out_chlayout="  << szOutLayout;
	ret = avfilter_graph_create_filter(ctx, filter, ("aresample" + std::to_string(nameindex)).c_str(), ss.str().c_str(), nullptr, data_.graph_.get());
	if (ret)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_create_filter error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	return 0;
}

int AudioMixer::initaformat(AVFilterContext** ctx, int sample_rate, uint64_t channel_layout, AVSampleFormat fmt, int nameindex)
{
	int ret;
	*ctx = nullptr;
	const AVFilter* filter = avfilter_get_by_name("aformat");
	if (!filter)
		return -1;
	std::stringstream ss;
	ss << "f=" << av_get_sample_fmt_name(fmt) << ":r=" << sample_rate << ":cl=0x" << std::hex << channel_layout;
	ret = avfilter_graph_create_filter(ctx, filter, ("aformat" + std::to_string(nameindex)).c_str(), ss.str().c_str(), nullptr, data_.graph_.get());
	if (ret) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_create_filter error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	return 0;
}

int AudioMixer::initvolume(AVFilterContext** ctx, int precision, float volume, int nameindex)
{
	const AVFilter* filter = avfilter_get_by_name("volume");
	if (!filter)
	{
		PrintE("avfilter_get_by_name error for volume");
		return -1;
	}
	std::stringstream ss;
	ss << "volume=" << volume << "dB:precision=" << precision;
	int ret;
	if ((ret = avfilter_graph_create_filter(ctx, filter, ("volume" + std::to_string(nameindex)).c_str(), ss.str().c_str(), nullptr, data_.graph_.get())) < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_create_filter error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	return 0;
}

int AudioMixer::initamix(AVFilterContext** ctx, int inputs, AudioMixer::MixerMode mode, int dropout_transition, const std::vector<float>& weights) 
{
	if (inputs != weights.size())
	{
		PrintE("variable inputs doesn't match weights size");
		return -1;
	}
	std::stringstream ss;
	ss << "inputs=" << inputs << ":duration=" << (int)mode << ":dropout_transition=" << dropout_transition << ":weights=";
	std::for_each(weights.cbegin(), weights.cend(), [&ss](std::remove_reference<decltype(weights)>::type::const_reference& v) {ss << v<< " ";});
	ss.seekp(-1, std::ios_base::_Seekdir::_Seekend)<<'\0';
	const AVFilter* filter = avfilter_get_by_name("amix");
	if (!filter)
	{
		PrintE("avfilter_get_by_name for amix error");
		return -1;
	}
	int ret = avfilter_graph_create_filter(ctx, filter, "amix0", ss.str().c_str(), nullptr, data_.graph_.get());
	if (ret < 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_create_filter error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	return 0;
}

int AudioMixer::initabuffersink(AVFilterContext** ctx) 
{
	const AVFilter* filter = avfilter_get_by_name("abuffersink");
	if (!filter) 
	{
		PrintE("avfilter_get_by_name return nullptr for abuffersink");
		return -1;
	}
	int ret;
	if ((ret = avfilter_graph_create_filter(ctx, filter, "abuffersink0", "", nullptr, data_.graph_.get())) < 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("avfilter_graph_create_filter error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	AVSampleFormat fmt[] = { m_out.fmt, AV_SAMPLE_FMT_NONE};
	if ((ret = av_opt_set_bin((*ctx)->priv, "sample_fmts", (uint8_t*)fmt, sizeof(*fmt), 0)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_opt_set_bin error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	int sample_rate[] = { m_out.sampleRate, 0};
	if ((ret = av_opt_set_bin((*ctx)->priv, "sample_rates", (uint8_t*)sample_rate, sizeof(*sample_rate), 0)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_opt_set_bin error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	
	/*AVChannelLayout chlay;
	ret = av_channel_layout_from_mask(&chlay, m_out.channel_layout);
	if (ret != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	char szbuf[AV_ERROR_MAX_STRING_SIZE];
	ret = av_channel_layout_describe(&chlay, szbuf, sizeof(szbuf));
	if (ret <= 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_describe error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	char* szOpt[2] = { szbuf , NULL};

	if ((ret = av_opt_set_from_string((*ctx)->priv, "ch_layouts", szOpt, "|", ":")) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_opt_set error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}*/


	//uint64_t ch_layouts[] = {m_out.channel_layout, 0};
	//AVChannelLayout chlay[2];

	///*chlay[1].nb_channels = 0;
	//chlay[1].order = AV_CHANNEL_ORDER_UNSPEC;
	//chlay[1].u.mask = 0;
	//chlay[1].opaque = NULL;*/
	//memset(chlay, 0, sizeof(chlay));

	//chlay[1].order = AV_CHANNEL_ORDER_NATIVE;

	//ret = av_channel_layout_from_mask(&chlay[0], m_out.channel_layout);
	//if (ret != 0) 
	//{
	//	char szErr[AV_ERROR_MAX_STRING_SIZE];
	//	PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	//	return ret;
	//}

	/*char szbuf[AV_ERROR_MAX_STRING_SIZE];
	ret = av_channel_layout_describe(&chlay, szbuf, sizeof(szbuf));
	if (ret <= 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_describe error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}*/
	//char* szPtr[2] = { szbuf , NULL};
	//if ((ret = av_opt_set_chlayout((*ctx)->priv, "ch_layouts", chlay, 0)) < 0) 
	//{
	//	char szErr[AV_ERROR_MAX_STRING_SIZE];
	//	PrintE("av_opt_set_chlayout error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
	//	return ret;
	//}
	//if ((ret = av_opt_set_from_string((*ctx)->priv,"ch_layouts", szPtr, ret, 0)) != 0)
	//{
	//	//makeErrStr(ret, "av_opt_set_bin error:");
	//	PrintE("av_opt_set_from_string error ");
	//	return ret;
	//}
	/*
	int channels[] = {av_get_channel_layout_nb_channels(m_out.info.channel_layout), 0};
	if ((ret = av_opt_set_bin((*ctx)->priv, "channel_counts", (uint8_t*)channels, sizeof(*channels), 0)) != 0)
	{
		makeErrStr(ret, "av_opt_set_bin error:");
		return ret;
	}
	if ((ret = av_opt_set_int((*ctx)->priv, "all_channel_counts", 1, 0)) != 0)
	{
		makeErrStr(ret, "av_opt_set_int error:");
		return ret;
	}
	*/
	return 0;
}


std::string AudioMixer::makeString(const std::vector<float>& sets) 
{
	std::stringstream ss;
	std::for_each(sets.cbegin(), sets.cend(), [&ss](std::remove_reference<decltype(sets)>::type::const_reference v) {ss << v << " "; });
	std::string str = ss.str();
	if (!str.empty()) 
		str = str.substr(0, str.length() - 1);
	return str;
}

int AudioMixer::getPrecision(AVSampleFormat fmt)
{
	return XX[(int)fmt + 1].p;
}

int AudioMixer::initFifo() 
{
	//int channels = av_get_channel_layout_nb_channels(m_out.channel_layout);
	AVChannelLayout out_chlay;
	int err = av_channel_layout_from_mask(&out_chlay, m_out.channel_layout);
	if (err != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
		return -1;
	}

	//int channels = out_chlay.nb_channels;

	if (m_out.fmt == AV_SAMPLE_FMT_NONE
		|| av_channel_layout_check(&out_chlay) != 1//channels <= 0
		|| m_out.sampleRate <= 0)
		return false;

	if (fifofmt_ == m_out.fmt
		&& fifochannels_ == out_chlay.nb_channels
		&& fifosamplerate_ == m_out.sampleRate
		&& fifo_)
		return true;

	UPTR_AFIFO fifo(av_audio_fifo_alloc(m_out.fmt, out_chlay.nb_channels, 1));
	if (!fifo) 
	{
		PrintE("av_audio_fifo_alloc error:no memory");
		return false;
	}
	if (fifo_)
	{
		/*int fifosize = av_audio_fifo_size(fifo_);
		if (fifosize > 0)
		{
			CAudioFrameCast cast;
			if (cast.set(fifochannels_, fifofmt_, fifosamplerate_, out_chlay.nb_channels, m_out.fmt, m_out.sampleRate))
			{
				UPTR_FME free_f(av_frame_alloc());
				if (!free_f)
					return -1;
				free_f->ch_layout = out_chlay;
				free_f->format = m_out.fmt;
				free_f->sample_rate = m_out.sampleRate;
				free_f->nb_samples = fifosize;
				int ret;
				if ((ret = av_frame_get_buffer(free_f.get(), 0)) != 0)
					return -1;
				if ((ret = av_frame_make_writable(free_f.get())) != 0)
				{
					char szErr[AV_ERROR_MAX_STRING_SIZE];
					PrintE("av_frame_make_writable:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
					return ret;
				}
				if (av_audio_fifo_read(fifo_, (void**)free_f->data, free_f->nb_samples) != free_f->nb_samples)
					return -1;
				if (!cast.translate(free_f.get()))
					return -1;
				if (!(free_f = cast.getFrame(0)))
					return -1;
				if (av_audio_fifo_realloc(fifo.get(), free_f->nb_samples + 1) != 0)
					return -1;
				if (av_audio_fifo_write(fifo.get(), (void**)free_f->data, free_f->nb_samples) != free_f->nb_samples)
					return -1;
			}
			else 
				return -1;
		}*/
		av_audio_fifo_free(fifo_);
	}
	fifo_ = fifo.release();
	fifofmt_= m_out.fmt;
	fifochannels_ = out_chlay.nb_channels;
	fifosamplerate_ = m_out.sampleRate;
	return 0;
}
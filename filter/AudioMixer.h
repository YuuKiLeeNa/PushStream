#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavfilter/avfilter.h"
#include "libavutil/buffer.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#ifdef __cplusplus
}
#endif
#include "MixerBase.h"
#include<vector>
#include<string>
#include<mutex>
#include<atomic>
#include<map>
#include"AudioFilter.h"
#include<deque>



struct AVAudioFifo;
class AudioMixer:public MixerBase<AudioMixer>/*, public ffMakeErrStr*/, public AudioFilter
{
public:
	typedef struct AudioInfo
	{
		float weights;
		float volume;
		uint64_t channel_layout;
		AVRational timebase;
		int sampleRate;
		AVSampleFormat fmt;
		int nameIndex;
	}AudioInfo;
	enum MixerMode:int {longest=0,shortest=1,first=2};
	AudioMixer();
	//AudioMixer(const std::vector<AudioMixer::AudioInfo>&in, const AudioMixer::AudioInfo&out);
	//AudioMixer(const AudioMixer::AudioInfo& in1, const AudioMixer::AudioInfo& in2, const AudioMixer::AudioInfo& out);
	int addInput(int sample_rate,uint64_t channel_layout, AVSampleFormat fmt, float weights, float volume);
	//int addInput(const AudioMixer::AudioInfo& inputs);
	void setOutput(int sample_rate, uint64_t channel_layout, AVSampleFormat fmt);
	//void setOutput(const AudioMixer::AudioInfo& output);
	virtual ~AudioMixer();
	void reset();
	int init(AudioMixer::MixerMode mode);
	//int init(const std::vector<AudioMixer::AudioInfo>& in, const AudioMixer::AudioInfo& out, AudioMixer::MixerMode mode);
	//int init(const AudioMixer::AudioInfo& in1, const AudioMixer::AudioInfo& in2, const AudioMixer::AudioInfo& out, AudioMixer::MixerMode mode);
	//int addFrame(int indexOfFilter, AVFrame*pushFrame,bool isKeepRef = false);
	//int getFrame(AVFrame*f);
	//FRAME_ADD
	//warning f must call av_frame_unref before , or it will leak memory
	int getFrame(AVFrame* f, int nb_samples = 0);
	//int getFrame(AVFrame* f, int nb_samples);
	int setVolumeDB(int indexOfFilter, float volume);
	//int getVolume(int indexOfFilter, float *volume);
	int setWeights(const std::vector<float>& weights);
	//int getWeights(std::vector<float>* weights);
	int getInputAudioStream();
protected:
	int initabuffer(AVFilterContext**ctx, const AVRational&timebase, int rample_rate, uint64_t channel_layout,AVSampleFormat fmt, int nameindex);
	int initaresample(AVFilterContext** ctx, uint64_t inchlay, uint64_t outchlay, int insamplerate, int outsamplerate, AVSampleFormat infmt, AVSampleFormat outfmt, uint64_t in_ch_layout, uint64_t out_ch_layout, int nameindex);
	int initaformat(AVFilterContext**ctx,int sample_rate, uint64_t channel_layout, AVSampleFormat fmt, int nameindex);
	int initvolume(AVFilterContext** ctx, int precision, float volume, int nameindex);
	int initamix(AVFilterContext** ctx,int inputs, AudioMixer::MixerMode mode, int dropout_transition, const std::vector<float>&weights);
	int initabuffersink(AVFilterContext** ctx);
	std::string makeString(const std::vector<float>& sets);
	int getPrecision(AVSampleFormat fmt);
	int initFifo();
protected:
	AVAudioFifo* fifo_ = nullptr;
	AVSampleFormat fifofmt_ = AV_SAMPLE_FMT_NONE;
	int fifochannels_ = 0;
	int fifosamplerate_ = 0;

	std::vector<AudioInfo>m_inSets;
	AudioInfo m_out;
	enum MixerMode m_mode;
	int m_abuffer = 0;
	//std::deque<std::pair<long double, int>>m_deq;
	const struct { AVSampleFormat f; int p; }XX[AV_SAMPLE_FMT_NB+2] =
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
};


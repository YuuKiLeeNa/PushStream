#pragma once
#include "CRingBuffer.h"
#include "obs.h"
#include "CAudioFrameCast.h"
#include<mutex>
#include "audio-io.h"
#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/avutil.h"
#include "libavutil/samplefmt.h"
#include "libavutil/channel_layout.h"
#ifdef __cplusplus
}
#endif

#include "Util/logger.h"
#include "Network/Socket.h"
#include "platform.h"
#include "obs-internal.h"
#include "util_uint64.h"
#include "CWasapi.h"
#include<atomic>
#include<string>
#include "AudioPeakMagnitudeCalc.h"
#include<functional>


#define TS_SMOOTHING_THRESHOLD 70000000ULL
#define AUDIO_OUTPUT_FRAMES 1024
#define MAX_BUF_SIZE (1000 * AUDIO_OUTPUT_FRAMES * sizeof(float))

typedef std::function<void(float(*)[MAX_AUDIO_CHANNELS], float(*)[MAX_AUDIO_CHANNELS])>AUDIO_DATA_CALC_CALL_BACK;


//template<int CHANNELS>
class CSource:public CWasapi<CSource/*<CHANNELS>*/>, public CRingBuffer/*<CHANNELS>*/, public AudioPeakMagnitudeCalc
{
public:
	//using base_type = BASE;
	//int s_channels = 0;
	CSource(const std::string& name, obs_peak_meter_type peak_calc_type, AUDIO_DATA_CALC_CALL_BACK audio_call_back, typename CWasapi<CSource>::SourceType type) :source_name(name)
		, CWasapi<CSource>(type)
		, AudioPeakMagnitudeCalc(peak_calc_type)
		, m_audio_data_call_back(audio_call_back)
	{
		//dataLengthSizeNotice = util_mul_div64(TS_SMOOTHING_THRESHOLD*1.5, m_out_sample_rate, 1000000000ULL);
		memset(&m_output_audio, 0, sizeof(m_output_audio)); 
		m_output_audio.format = AUDIO_FORMAT_UNKNOWN;
		m_output_audio.speakers = SPEAKERS_UNKNOWN;
		CRingBuffer::init();
		CRingBuffer::SIZE = 2;
		dataLengthNoticeSignal = CreateEvent(nullptr, true, false, nullptr);
		if (!dataLengthNoticeSignal.Valid())
			PrintE("Could not create dataLengthNoticeSignal signal");
	}
	~CSource() 
	{
		CWasapi<CSource>::Stop();
		if (m_output_audio.data[0])
			av_freep(&m_output_audio.data[0]);
	}

	inline WinHandle& getDataLengthNoticeSignal() { return dataLengthNoticeSignal; }

	inline void source_output_audio(obs_source_audio*in_audio);
	inline bool process_audio(const struct obs_source_audio* audio, struct obs_source_audio* audio_out);

	void setOutAudioInfo(uint64_t channel_layout, int sample_rate, AVSampleFormat format) 
	{
		m_out_channel_layout = channel_layout;
		m_out_sample_rate = sample_rate;
		m_out_format = format;
	}

	static inline uint64_t uint64_diff(uint64_t ts1, uint64_t ts2)
	{
		return (ts1 < ts2) ? (ts2 - ts1) : (ts1 - ts2);
	}

	void reset_audio_timing(uint64_t timestamp, uint64_t os_time)
	{
		timing_set = true;
		timing_adjust = os_time - timestamp;
	}

	inline void handle_ts_jump(uint64_t expected, uint64_t ts,
		uint64_t diff, uint64_t os_time);
	inline void reset_audio_data(uint64_t os_time);

	static inline uint64_t conv_frames_to_time(const size_t sample_rate,
		const size_t frames);

	inline int get_out_channels()
	{
		AVChannelLayout chlay;
		int err = av_channel_layout_from_mask(&chlay, m_out_channel_layout);
		if (err != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
			return -1;
		}
		return chlay.nb_channels;
	}


	int get_out_samplerate() { return m_out_sample_rate; }
	AVSampleFormat get_out_format() { return m_out_format; }
	uint64_t get_out_lay() { return m_out_channel_layout; }

	AVChannelLayout get_out_chlay()
	{
		AVChannelLayout chlay;
		memset(&chlay, 0, sizeof(chlay));
		int err = av_channel_layout_from_mask(&chlay, m_out_channel_layout);
		if (err != 0) 
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
		}
		return chlay;
	}

	static inline size_t get_buf_placement(uint32_t sample_rate, uint64_t offset);

	inline uint64_t getSamples(uint8_t*arr[MAX_AV_PLANES], int Samples, size_t* leftSize);
	uint64_t getAudioTs() { std::lock_guard<std::mutex>lock(audio_buf_mutex); return audio_ts; }
	void setAudioTs(uint64_t t) { std::lock_guard<std::mutex>lock(audio_buf_mutex); audio_ts = t; }
	int getDataLengthSizeNotice() { return dataLengthSizeNotice;}
	void setSourceName(const std::string& name) { source_name = name; }
	std::string getSourceName() {return source_name;};
	inline void push_front_zero_then_set_audio_ts(size_t bytes,uint64_t t);
	inline void push_front_data_bytes_then_set_audio_ts(uint8_t**dd, size_t bytes, uint64_t t);

	void setAudioDataCalcCallBack(AUDIO_DATA_CALC_CALL_BACK cb) { m_audio_data_call_back = cb; }
protected:
	inline bool reset_resampler(const struct obs_source_audio* audio);
	inline void source_output_audio_data(const struct audio_data* data);
	inline void source_output_audio_push_back(const struct audio_data* in);
	inline void source_output_audio_place(const struct audio_data* in);
	inline void signal_audio_data(const struct audio_data* in);
protected:
	CAudioFrameCast m_cast;
	uint64_t m_out_channel_layout = AV_CH_LAYOUT_STEREO;
	int m_out_sample_rate = 48000;
	AVSampleFormat m_out_format = AV_SAMPLE_FMT_FLTP;

	uint64_t m_in_channel_layout = 0;
	int m_in_sample_rate = 0;
	AVSampleFormat m_in_format = AV_SAMPLE_FMT_NONE;

	std::mutex audio_mutex;
	std::mutex audio_buf_mutex;
	volatile bool timing_set = false;
	volatile uint64_t timing_adjust;
	uint64_t resample_offset = 0;
	uint64_t last_audio_ts = 0;
	uint64_t next_audio_ts_min = 0;
	uint64_t next_audio_sys_ts_min = 0;
	uint64_t last_frame_ts = 0;
	uint64_t last_sys_timestamp = 0;
	uint64_t audio_ts = 0;

	int64_t sync_offset = 0;
	int64_t last_sync_offset = 0;
	size_t last_audio_input_buf_size = 0;

	/*alignas(16) */struct obs_source_audio m_output_audio;
	
	WinHandle dataLengthNoticeSignal;
	int dataLengthSizeNotice = 4096 * 2;
	std::string source_name;
	size_t buff_last_size = 0;
	AUDIO_DATA_CALC_CALL_BACK m_audio_data_call_back;
};




void CSource::source_output_audio(obs_source_audio* in_audio)
{
	//struct obs_source_audio output;
	/*if (!obs_source_valid(source, "obs_source_output_audio"))
		return;
	if (destroying(source))
		return;
	if (!obs_ptr_valid(audio_in, "obs_source_output_audio"))
		return;*/

		/* sets unused data pointers to NULL automatically because apparently
		 * some filter plugins aren't checking the actual channel count, and
		 * instead are checking to see whether the pointer is non-zero. */
	struct obs_source_audio audio = *in_audio;
	size_t channels = get_audio_planes(audio.format, audio.speakers);
	for (size_t i = channels; i < MAX_AV_PLANES; i++)
		audio.data[i] = NULL;


	if (!process_audio(&audio, &m_output_audio))
		return;
	//pthread_mutex_lock(&source->filter_mutex);
	//output = filter_async_audio(source, &source->audio_data);

	//if (output) {
	struct audio_data data;
	for (int i = 0; i < MAX_AV_PLANES; i++)
		data.data[i] = m_output_audio.data[i];
	data.frames = m_output_audio.frames;
	data.timestamp = m_output_audio.timestamp;

	{
		std::lock_guard<std::mutex>lock(audio_mutex);
		source_output_audio_data(&data);
	}
	//}

	//pthread_mutex_unlock(&source->filter_mutex);

}


bool CSource::process_audio(const struct obs_source_audio* audio, struct obs_source_audio* audio_out)
{
	uint32_t frames = audio->frames;
	//bool mono_output;
	bool bNeedCast = false;

	if (m_in_sample_rate != audio->samples_per_sec ||
		m_in_format != audio->format ||
		m_in_channel_layout != audio->speakers)
	{
		m_in_sample_rate = audio->samples_per_sec;
		m_in_format = (AVSampleFormat)audio->format;
		m_in_channel_layout = audio->speakers;
		if (m_in_sample_rate != m_out_sample_rate ||
			m_in_format != m_out_format ||
			m_in_channel_layout != m_out_channel_layout
			)
		{

			reset_resampler(audio);
			bNeedCast = true;
		}
	}

	if (bNeedCast && !m_cast)
		return false;

	if (m_cast) {
		//uint8_t* output[MAX_AV_PLANES];
		//memset(output, 0, sizeof(output));
		uint32_t out_frames = 0;

		m_cast.audio_resampler_resample(audio_out->data, &audio_out->frames, &resample_offset, audio->data, audio->frames);

		audio_out->timestamp = audio->timestamp;
		//memcpy(audio->);
		//copy_audio_data(source, (const uint8_t* const*)output, frames,
		//	audio->timestamp);
	}
	else {
		//copy_audio_data(source, audio->data, audio->frames,audio->timestamp);
		resample_offset = 0;
		AVChannelLayout chlay;
		int err = av_channel_layout_from_mask(&chlay, m_out_channel_layout);
		if (err != 0)
		{
			char szErr[AV_ERROR_MAX_STRING_SIZE];
			PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, AV_ERROR_MAX_STRING_SIZE, err));
			return false;
		}
		memcpy(audio_out, audio, sizeof(*audio_out));

		av_samples_alloc(audio_out->data, NULL, chlay.nb_channels, audio->frames, m_out_format, 1);

		int bytes = av_get_bytes_per_sample(m_out_format);

		for (int ch = 0; ch < chlay.nb_channels; ++ch)
			memcpy(audio_out->data[ch], audio->data[ch], bytes * audio->frames);
		audio_out->timestamp = audio->timestamp;
	}

	//mono_output = audio_output_get_channels(obs->audio.audio) == 1;

	/*if (!mono_output && source->sample_info.speakers == SPEAKERS_STEREO &&
		(source->balance > 0.51f || source->balance < 0.49f)) {
		process_audio_balancing(source, frames, source->balance,
			OBS_BALANCE_TYPE_SINE_LAW);
	}*/

	//if (!mono_output && (source->flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0)
	//	downmix_to_mono_planar(source, frames);
	return true;
}


void CSource::handle_ts_jump(uint64_t expected, uint64_t ts,
	uint64_t diff, uint64_t os_time)
{
	PrintD("Timestamp for %s(device:%s) jumped by '%" PRIu64 "', "
		"expected value %" PRIu64 ", input value %" PRIu64,
		source_name.c_str(), CSource::device_name.c_str(), diff, expected, ts);

	//pthread_mutex_lock(&source->audio_buf_mutex);
	std::lock_guard<std::mutex>lock(audio_buf_mutex);
	reset_audio_timing(ts, os_time);
	reset_audio_data(os_time);
	//pthread_mutex_unlock(&source->audio_buf_mutex);
}


void CSource::reset_audio_data(uint64_t os_time)
{
	//for (size_t i = 0; i < MAX_AUDIO_CHANNELS; i++) {
		//if (audio_input_buf[i].size)
	uint8_t* pDATA[MAX_AUDIO_CHANNELS];
	memset(pDATA, 0, sizeof(pDATA));
	CRingBuffer::pop_front_all((void**)pDATA);
	//}

	last_audio_input_buf_size = 0;
	audio_ts = os_time;
	next_audio_sys_ts_min = os_time;
}


uint64_t CSource::conv_frames_to_time(const size_t sample_rate,
	const size_t frames)
{
	if (!sample_rate)
		return 0;

	return util_mul_div64(frames, 1000000000ULL, sample_rate);
}

//template<int CHANNELS>
//int CSource<CHANNELS>::get_out_channels()
//{
//	AVChannelLayout chlay;
//	int err = av_channel_layout_from_mask(&chlay, m_out_channel_layout);
//	if (err != 0)
//	{
//		char szErr[AV_ERROR_MAX_STRING_SIZE];
//		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), err));
//		return -1;
//	}
//	return chlay.nb_channels;
//}


size_t CSource::get_buf_placement(uint32_t sample_rate, uint64_t offset)
{
	//uint32_t sample_rate = audio_output_get_sample_rate(audio);
	return (size_t)util_mul_div64(offset, sample_rate, 1000000000ULL);
}


uint64_t CSource::getSamples(uint8_t* arr[MAX_AV_PLANES], int Samples, size_t*leftSize)
{
	int calcsize = Samples*av_get_bytes_per_sample(m_out_format);
	uint64_t during_time = util_mul_div64(Samples, 1000000000ULL, m_out_sample_rate);

	//std::lock_guard<std::mutex>lock(CRingBuffer<CHANNELS>::m_ring_buff_mutex);
	std::lock_guard<std::mutex>lock(audio_buf_mutex);
	if (CRingBuffer::size < calcsize || audio_ts == 0)
	{
		if (leftSize)
			*leftSize = CRingBuffer::size;
		return 0;
	}
	CRingBuffer::pop_front((void**)arr, calcsize);
	if (leftSize)
		*leftSize = CRingBuffer::size;
	//返回先前值
	uint64_t prious_vale = audio_ts;
	audio_ts += during_time;
	return prious_vale;//std::atomic_fetch_add(&audio_ts, during_time);
}


void CSource::push_front_zero_then_set_audio_ts(size_t bytes, uint64_t t) 
{
	std::lock_guard<std::mutex>lock(audio_buf_mutex);
	CRingBuffer::push_front_zero(bytes);
	audio_ts = t;
}


void CSource::push_front_data_bytes_then_set_audio_ts(uint8_t** dd, size_t bytes, uint64_t t)
{
	std::lock_guard<std::mutex>lock(audio_buf_mutex);
	if (audio_ts == 0)
		return;
	CRingBuffer::push_front((const void**)dd, bytes);
	audio_ts = t;
}


bool CSource::reset_resampler(const obs_source_audio* audio)
{

	bool b = m_cast.set((uint64_t)audio->speakers, (AVSampleFormat)audio->format, audio->samples_per_sec, (uint64_t)m_out_channel_layout, m_out_format, m_out_sample_rate);
	return b;
}


void CSource::source_output_audio_data(const struct audio_data* data)
{
	size_t sample_rate = m_out_sample_rate;//audio_output_get_sample_rate(obs->audio.audio);
	struct audio_data in = *data;
	uint64_t diff;
	uint64_t os_time = os_gettime_ns();
	int64_t sync_offset_tmp;
	bool using_direct_ts = false;
	bool push_back = false;

	/* detects 'directly' set timestamps as long as they're within
	 * a certain threshold */
	if (uint64_diff(in.timestamp, os_time) < MAX_TS_VAR) {
		timing_adjust = 0;
		timing_set = true;
		using_direct_ts = true;
	}

	if (!timing_set) {
		reset_audio_timing(in.timestamp, os_time);
	}
	else if (next_audio_ts_min != 0) {
		diff = uint64_diff(next_audio_ts_min, in.timestamp);

		/* smooth audio if within threshold */
		if (diff > MAX_TS_VAR && !using_direct_ts)
			handle_ts_jump(next_audio_ts_min, in.timestamp, diff, os_time);
		else if (diff < TS_SMOOTHING_THRESHOLD) {
			//if (source->async_unbuffered && source->async_decoupled)
			//	source->timing_adjust = os_time - in.timestamp;
			in.timestamp = next_audio_ts_min;
		}
		else {
			PrintD("Audio timestamp for %s(device:%s) exceeded TS_SMOOTHING_THRESHOLD, diff=%" PRIu64
				" ns, expected %" PRIu64 ", input %" PRIu64,
				source_name.c_str(), CSource::device_name.c_str(), diff, next_audio_ts_min, in.timestamp);
		}
	}

	last_audio_ts = in.timestamp;
	next_audio_ts_min = in.timestamp + conv_frames_to_time(sample_rate, in.frames);

	in.timestamp += timing_adjust;

	{
		std::lock_guard<std::mutex>lock(audio_buf_mutex);

		if (next_audio_sys_ts_min == in.timestamp) {
			push_back = true;

		}
		else if (next_audio_sys_ts_min) {
			diff = uint64_diff(next_audio_sys_ts_min, in.timestamp);

			if (diff < TS_SMOOTHING_THRESHOLD) {
				push_back = true;

			}
			else if (diff > MAX_TS_VAR) {
				/* This typically only happens if used with async video when
				 * audio/video start transitioning in to a timestamp jump.
				 * Audio will typically have a timestamp jump, and then video
				 * will have a timestamp jump.  If that case is encountered,
				 * just clear the audio data in that small window and force a
				 * resync.  This handles all cases rather than just looping. */
				reset_audio_timing(data->timestamp, os_time);
				in.timestamp = data->timestamp + timing_adjust;
			}
		}

		sync_offset_tmp = sync_offset;
		in.timestamp += sync_offset_tmp;
		in.timestamp -= resample_offset;

		next_audio_sys_ts_min =
			next_audio_ts_min + timing_adjust;

		if (last_sync_offset != sync_offset_tmp) {
			if (last_sync_offset)
				push_back = false;
			last_sync_offset = sync_offset;
		}

		//if (source->monitoring_type != OBS_MONITORING_TYPE_MONITOR_ONLY) {
		if (push_back && audio_ts)
			source_output_audio_push_back(&in);
		else
			source_output_audio_place(&in);
		//}
	}
	//pthread_mutex_unlock(&source->audio_buf_mutex);

	signal_audio_data(data);
	//source_signal_audio_data(source, data, source_muted(source, os_time));
}


void CSource::source_output_audio_push_back(const struct audio_data* in)
{
	//audio_t* audio = obs->audio.audio;
	int channels = get_out_channels();//audio_output_get_channels(audio);
	if (channels < 0)
		return;
	size_t size_in = in->frames * sizeof(float);

	{
		//std::lock_guard<std::mutex>lock(CRingBuffer<CHANNELS>::m_ring_buff_mutex);
		/* do not allow the circular buffers to become too big */
		if ((CRingBuffer::size + size_in) > MAX_BUF_SIZE)
			return;

		//for (size_t i = 0; i < channels; i++)
		CRingBuffer::push_back((const void**)in->data, size_in);
		if (dataLengthSizeNotice <= CRingBuffer::size && audio_ts != 0)
		{
			//printf("%s size %llu\n", CWasapi<CSource<CHANNELS>>::device_name.c_str(), CRingBuffer<CHANNELS>::size);
			DWORD h = WaitForSingleObject(dataLengthNoticeSignal, 0);
			switch (h)
			{
			case WAIT_OBJECT_0:
				break;
			default:
				//printf("SetEvent %s\n", CWasapi<CSource<CHANNELS>>::device_name.c_str());
				SetEvent(dataLengthNoticeSignal);
				break;
			}
		}
		/*else 
		{
			ResetEvent(dataLengthNoticeSignal);
		}*/
	}
	/* reset audio input buffer size to ensure that audio doesn't get
	 * perpetually cut */
	last_audio_input_buf_size = 0;
}


void CSource::source_output_audio_place(const struct audio_data* in)
{
	//audio_t* audio = obs->audio.audio;
	size_t buf_placement;
	size_t channels = get_out_channels(); //audio_output_get_channels(audio);
	size_t size_in = in->frames * sizeof(float);

	if (!audio_ts || in->timestamp < audio_ts)
		reset_audio_data(in->timestamp);

	buf_placement = get_buf_placement(m_out_sample_rate, in->timestamp - audio_ts) * sizeof(float);

#if DEBUG_AUDIO == 1
	blog(LOG_DEBUG,
		"frames: %lu, size: %lu, placement: %lu, base_ts: %llu, ts: %llu",
		(unsigned long)in->frames,
		(unsigned long)source->audio_input_buf[0].size,
		(unsigned long)buf_placement, source->audio_ts, in->timestamp);
#endif

	/* do not allow the circular buffers to become too big */
	if ((buf_placement + size_in) > MAX_BUF_SIZE)
		return;

	//for (size_t i = 0; i < channels; i++) {
	{
		//std::lock_guard<std::mutex>lock(CRingBuffer<CHANNELS>::m_ring_buff_mutex);
		CRingBuffer::place(buf_placement, (const void**)in->data, size_in);
		CRingBuffer::pop_back(NULL, CRingBuffer::size - (buf_placement + size_in));
		if (dataLengthSizeNotice <= CRingBuffer::size && audio_ts != 0)
		{
			//printf("%s size %llu\n", CWasapi<CSource<CHANNELS>>::device_name.c_str(), CRingBuffer<CHANNELS>::size);
			SetEvent(dataLengthNoticeSignal);
		}
		/*else 
		{
			ResetEvent(dataLengthNoticeSignal);
		}*/
	}
	//}

	last_audio_input_buf_size = 0;
}


void CSource::signal_audio_data(const struct audio_data* in) 
{
	int ch = get_audio_channels((speaker_layout)m_out_channel_layout);
	calcMagnitudePeak(in, ch);
	if(m_audio_data_call_back)
		m_audio_data_call_back(&m_magnitude, &m_peak);
}


typedef std::shared_ptr<CSource> PSSource;
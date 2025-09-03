#pragma once
#include "audio-io.h"
#include "obs-audio-controls.h"
#include <xmmintrin.h>

class AudioPeakMagnitudeCalc 
{
public:

	AudioPeakMagnitudeCalc(obs_peak_meter_type type = SAMPLE_PEAK_METER) :m_peak_meter_type(type) {}

	void calcMagnitudePeak(const struct audio_data* data, int nr_channels);
	void calcMagnitude(const struct audio_data* data, int nr_channels);
	void calcPeak(const struct audio_data* data, int nr_channels);
	void set_peak_meter_type(obs_peak_meter_type type) { m_peak_meter_type = type; }

protected:
	static float get_true_peak(__m128 previous_samples, const float* samples,
		size_t nr_samples);
	static float get_sample_peak(__m128 previous_samples, const float* samples,
		size_t nr_samples);


	void volmeter_process_peak_last_samples(int channel_nr, float* samples,
		size_t nr_samples);

	obs_peak_meter_type m_peak_meter_type;
	float m_prev_samples[MAX_AUDIO_CHANNELS][4] = {};
	float m_magnitude[MAX_AUDIO_CHANNELS] = {};
	float m_peak[MAX_AUDIO_CHANNELS] = {};
};
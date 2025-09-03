#include "AudioPeakMagnitudeCalc.h"



/* msb(h, g, f, e) lsb(d, c, b, a)   -->  msb(h, h, g, f) lsb(e, d, c, b)
 */
#define SHIFT_RIGHT_2PS(msb, lsb)                                          \
	{                                                                  \
		__m128 tmp =                                               \
			_mm_shuffle_ps(lsb, msb, _MM_SHUFFLE(0, 0, 3, 3)); \
		lsb = _mm_shuffle_ps(lsb, tmp, _MM_SHUFFLE(2, 1, 2, 1));   \
		msb = _mm_shuffle_ps(msb, msb, _MM_SHUFFLE(3, 3, 2, 1));   \
	}

 /* x(d, c, b, a) --> (|d|, |c|, |b|, |a|)
  */
#define abs_ps(v) _mm_andnot_ps(_mm_set1_ps(-0.f), v)

  /* Take cross product of a vector with a matrix resulting in vector.
   */
#define VECTOR_MATRIX_CROSS_PS(out, v, m0, m1, m2, m3)    \
	{                                                 \
		out = _mm_mul_ps(v, m0);                  \
		__m128 mul1 = _mm_mul_ps(v, m1);          \
		__m128 mul2 = _mm_mul_ps(v, m2);          \
		__m128 mul3 = _mm_mul_ps(v, m3);          \
                                                          \
		_MM_TRANSPOSE4_PS(out, mul1, mul2, mul3); \
                                                          \
		out = _mm_add_ps(out, mul1);              \
		out = _mm_add_ps(out, mul2);              \
		out = _mm_add_ps(out, mul3);              \
	}

   /* x4(d, c, b, a)  -->  max(a, b, c, d)
	*/
#define hmax_ps(r, x4)                     \
	do {                               \
		float x4_mem[4];           \
		_mm_storeu_ps(x4_mem, x4); \
		r = x4_mem[0];             \
		r = fmaxf(r, x4_mem[1]);   \
		r = fmaxf(r, x4_mem[2]);   \
		r = fmaxf(r, x4_mem[3]);   \
	} while (false)



void AudioPeakMagnitudeCalc::calcMagnitudePeak(const audio_data * data, int nr_channels)
{
	calcMagnitude(data,nr_channels);
	calcPeak(data,nr_channels);
}

void AudioPeakMagnitudeCalc::calcMagnitude(const struct audio_data* data, int nr_channels)
{
	size_t nr_samples = data->frames;

	int channel_nr = 0;
	for (int plane_nr = 0; channel_nr < nr_channels; plane_nr++) {
		float* samples = (float*)data->data[plane_nr];
		if (!samples) {
			continue;
		}

		float sum = 0.0;
		for (size_t i = 0; i < nr_samples; i++) {
			float sample = samples[i];
			sum += sample * sample;
		}
		m_magnitude[channel_nr] = sqrtf(sum / nr_samples);

		channel_nr++;
	}
}

void AudioPeakMagnitudeCalc::calcPeak(const struct audio_data* data, int nr_channels)
{
	int nr_samples = data->frames;
	int channel_nr = 0;
	for (int plane_nr = 0; channel_nr < nr_channels; plane_nr++) {
		float* samples = (float*)data->data[plane_nr];
		if (!samples) {
			continue;
		}
		/*if (((uintptr_t)samples & 0xf) > 0) {
			printf("Audio plane %i is not aligned %p skipping "
				"peak volume measurement.\n",
				plane_nr, samples);
			m_peak[channel_nr] = 1.0;
			channel_nr++;
			continue;
		}*/

		/* volmeter->prev_samples may not be aligned to 16 bytes;
		 * use unaligned load. */
		__m128 previous_samples =
			_mm_loadu_ps(m_prev_samples[channel_nr]);

		float peak;
		switch (m_peak_meter_type) {
		case TRUE_PEAK_METER:
			peak = get_true_peak(previous_samples, samples,
				nr_samples);
			break;

		case SAMPLE_PEAK_METER:
		default:
			peak = get_sample_peak(previous_samples, samples,
				nr_samples);
			break;
		}

		volmeter_process_peak_last_samples(channel_nr,samples, nr_samples);

		m_peak[channel_nr] = peak;

		channel_nr++;
	}

	/* Clear the peak of the channels that have not been handled. */
	for (; channel_nr < MAX_AUDIO_CHANNELS; channel_nr++) {
		m_peak[channel_nr] = 0.0;
	}
}




float AudioPeakMagnitudeCalc::get_true_peak(__m128 previous_samples, const float* samples,
	size_t nr_samples)
{
	/* These are normalized-sinc parameters for interpolating over sample
	 * points which are located at x-coords: -1.5, -0.5, +0.5, +1.5.
	 * And oversample points at x-coords: -0.3, -0.1, 0.1, 0.3. */
	const __m128 m3 =
		_mm_set_ps(-0.155915f, 0.935489f, 0.233872f, -0.103943f);
	const __m128 m1 =
		_mm_set_ps(-0.216236f, 0.756827f, 0.504551f, -0.189207f);
	const __m128 p1 =
		_mm_set_ps(-0.189207f, 0.504551f, 0.756827f, -0.216236f);
	const __m128 p3 =
		_mm_set_ps(-0.103943f, 0.233872f, 0.935489f, -0.155915f);

	__m128 work = previous_samples;
	__m128 peak = previous_samples;
	for (size_t i = 0; (i + 3) < nr_samples; i += 4) {
		__m128 new_work = _mm_load_ps(&samples[i]);
		__m128 intrp_samples;

		/* Include the actual sample values in the peak. */
		__m128 abs_new_work = abs_ps(new_work);
		peak = _mm_max_ps(peak, abs_new_work);

		/* Shift in the next point. */
		SHIFT_RIGHT_2PS(new_work, work);
		VECTOR_MATRIX_CROSS_PS(intrp_samples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrp_samples));

		SHIFT_RIGHT_2PS(new_work, work);
		VECTOR_MATRIX_CROSS_PS(intrp_samples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrp_samples));

		SHIFT_RIGHT_2PS(new_work, work);
		VECTOR_MATRIX_CROSS_PS(intrp_samples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrp_samples));

		SHIFT_RIGHT_2PS(new_work, work);
		VECTOR_MATRIX_CROSS_PS(intrp_samples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrp_samples));
	}

	float r;
	hmax_ps(r, peak);
	return r;
}

/* points contain the first four samples to calculate the sinc interpolation
 * over. They will have come from a previous iteration.
 */
float AudioPeakMagnitudeCalc::get_sample_peak(__m128 previous_samples, const float* samples,
	size_t nr_samples)
{
	__m128 peak = previous_samples;
	for (size_t i = 0; (i + 3) < nr_samples; i += 4) {
		__m128 new_work = _mm_load_ps(&samples[i]);
		peak = _mm_max_ps(peak, abs_ps(new_work));
	}

	float r;
	hmax_ps(r, peak);
	return r;
}


 void AudioPeakMagnitudeCalc::volmeter_process_peak_last_samples(int channel_nr, float* samples,
	size_t nr_samples)
{
	/* Take the last 4 samples that need to be used for the next peak
	 * calculation. If there are less than 4 samples in total the new
	 * samples shift out the old samples. */

	switch (nr_samples) {
	case 0:
		break;
	case 1:
		m_prev_samples[channel_nr][0] = m_prev_samples[channel_nr][1];
		m_prev_samples[channel_nr][1] = m_prev_samples[channel_nr][2];
		m_prev_samples[channel_nr][2] = m_prev_samples[channel_nr][3];
		m_prev_samples[channel_nr][3] = samples[nr_samples - 1];
		break;
	case 2:
		m_prev_samples[channel_nr][0] = m_prev_samples[channel_nr][2];
		m_prev_samples[channel_nr][1] = m_prev_samples[channel_nr][3];
		m_prev_samples[channel_nr][2] = samples[nr_samples - 2];
		m_prev_samples[channel_nr][3] = samples[nr_samples - 1];
		break;
	case 3:
		m_prev_samples[channel_nr][0] = m_prev_samples[channel_nr][3];
		m_prev_samples[channel_nr][1] = samples[nr_samples - 3];
		m_prev_samples[channel_nr][2] = samples[nr_samples - 2];
		m_prev_samples[channel_nr][3] = samples[nr_samples - 1];
		break;
	default:
		m_prev_samples[channel_nr][0] = samples[nr_samples - 4];
		m_prev_samples[channel_nr][1] = samples[nr_samples - 3];
		m_prev_samples[channel_nr][2] = samples[nr_samples - 2];
		m_prev_samples[channel_nr][3] = samples[nr_samples - 1];
	}
}

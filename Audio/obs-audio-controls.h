#pragma once

/**
 * @brief Peak meter types
 */
enum obs_peak_meter_type {
	/**
	 * @brief A simple peak meter measuring the maximum of all samples.
	 *
	 * This was a very common type of peak meter used for audio, but
	 * is not very accurate with regards to further audio processing.
	 */
	SAMPLE_PEAK_METER,

	/**
	 * @brief An accurate peak meter measure the maximum of inter-samples.
	 *
	 * This meter is more computational intensive due to 4x oversampling
	 * to determine the true peak to an accuracy of +/- 0.5 dB.
	 */
	TRUE_PEAK_METER
};



///* msb(h, g, f, e) lsb(d, c, b, a)   -->  msb(h, h, g, f) lsb(e, d, c, b)
// */
//#define SHIFT_RIGHT_2PS(msb, lsb)                                          \
//	{                                                                  \
//		__m128 tmp =                                               \
//			_mm_shuffle_ps(lsb, msb, _MM_SHUFFLE(0, 0, 3, 3)); \
//		lsb = _mm_shuffle_ps(lsb, tmp, _MM_SHUFFLE(2, 1, 2, 1));   \
//		msb = _mm_shuffle_ps(msb, msb, _MM_SHUFFLE(3, 3, 2, 1));   \
//	}
//
// /* x(d, c, b, a) --> (|d|, |c|, |b|, |a|)
//  */
//#define abs_ps(v) _mm_andnot_ps(_mm_set1_ps(-0.f), v)
//
//  /* Take cross product of a vector with a matrix resulting in vector.
//   */
//#define VECTOR_MATRIX_CROSS_PS(out, v, m0, m1, m2, m3)    \
//	{                                                 \
//		out = _mm_mul_ps(v, m0);                  \
//		__m128 mul1 = _mm_mul_ps(v, m1);          \
//		__m128 mul2 = _mm_mul_ps(v, m2);          \
//		__m128 mul3 = _mm_mul_ps(v, m3);          \
//                                                          \
//		_MM_TRANSPOSE4_PS(out, mul1, mul2, mul3); \
//                                                          \
//		out = _mm_add_ps(out, mul1);              \
//		out = _mm_add_ps(out, mul2);              \
//		out = _mm_add_ps(out, mul3);              \
//	}
//
//   /* x4(d, c, b, a)  -->  max(a, b, c, d)
//	*/
//#define hmax_ps(r, x4)                     \
//	do {                               \
//		float x4_mem[4];           \
//		_mm_storeu_ps(x4_mem, x4); \
//		r = x4_mem[0];             \
//		r = fmaxf(r, x4_mem[1]);   \
//		r = fmaxf(r, x4_mem[2]);   \
//		r = fmaxf(r, x4_mem[3]);   \
//	} while (false)
//


float log_def_to_db(const float def);

 float log_db_to_def(const float db);
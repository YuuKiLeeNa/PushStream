#pragma once
#include<Profileapi.h>
#include<stdint.h>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

static inline uint64_t util_mul_div64(uint64_t num, uint64_t mul, uint64_t div)
{
#if defined(_MSC_VER) && defined(_M_X64) && (_MSC_VER >= 1920)
	unsigned __int64 high;
	const unsigned __int64 low = _umul128(num, mul, &high);
	unsigned __int64 rem;
	return _udiv128(high, low, div, &rem);
#else
	const uint64_t rem = num % div;
	return (num / div) * mul + (rem * mul) / div;
#endif
}

static bool have_clockfreq = false;
static LARGE_INTEGER clock_freq;

static inline uint64_t get_clockfreq(void)
{
	if (!have_clockfreq) {
		QueryPerformanceFrequency(&clock_freq);
		have_clockfreq = true;
	}
	return clock_freq.QuadPart;
}

uint64_t os_gettime_ns(void)
{
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	return util_mul_div64(current_time.QuadPart, 1000000000,
		get_clockfreq());
}


#include<Windows.h>
#include "platform.h"
#include "util_uint64.h"

#define MAX_SZ_LEN 256

static bool have_clockfreq = false;
static LARGE_INTEGER clock_freq;
//static uint32_t winver = 0;
//static char win_release_id[MAX_SZ_LEN] = "unavailable";

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
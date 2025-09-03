#include "CAudioFrameCastTs.h"
#include "Util/logger.h"
#include "Network/Socket.h"
#include "util_uint64.h"


#define RESAMPLER_TIMESTAMP_SMOOTHING_THRESHOLD 70000000ULL

CAudioFrameCastTs::CAudioFrameCastTs():CAudioFrameCast()
{
}

CAudioFrameCastTs::~CAudioFrameCastTs()
{
}

void CAudioFrameCastTs::reset()
{
	CAudioFrameCast::reset();
	m_ts_offset = 0;
	m_ts = AV_NOPTS_VALUE;
	m_tsCorrect = false;
}

bool CAudioFrameCastTs::translate(UPTR_FME f)
{
	if (f->pts == AV_NOPTS_VALUE)
	{
		PrintW("frame pts is not expect:%lld", f->pts);
		return false;
	}
	int64_t pts_copy = f->pts;
	uint64_t ts_offset = 0;
	bool b = CAudioFrameCast::translate(std::move(f), &ts_offset);
	if (!b)
		return false;

	int64_t fts = util_i_mul_div64(pts_copy, 1000000000, m_iInSampleRate);
	if (m_ts == AV_NOPTS_VALUE)
		m_ts = fts;
	else 
	{
		m_ts_offset = fts - m_ts - ts_offset;
		if (std::abs(m_ts_offset) >= RESAMPLER_TIMESTAMP_SMOOTHING_THRESHOLD)
		{
			PrintW("Incoming frame pts exceeds timestamp smooth threshold(%lld)", m_ts_offset);
			if (m_tsCorrect) 
			{
				m_ts = fts - ts_offset;
				m_ts_offset = 0;
				PrintW("Correct frame pts timestamp");
			}
		}
		//else
		//	PrintD("Incoming frame pts diff timestamp(%lld)", m_ts_offset);
	}
	return true;
}

std::vector<UPTR_FME> CAudioFrameCastTs::getFrame(int iSamples)
{
	std::vector<UPTR_FME>sets;
	UPTR_FME f;
	while (f = CAudioFrameCast::getFrame(iSamples))
	{
		f->pts = util_i_mul_div64(m_ts, m_iOutSampleRate, 1000000000);
		//printf("pts (%lld) sample rate %d,samplers %d\n", f->pts, m_iOutSampleRate, f->nb_samples);
		m_ts += util_i_mul_div64(iSamples, 1000000000, m_iOutSampleRate);
		sets.emplace_back(std::move(f));
	}
	return sets;
}

void CAudioFrameCastTs::enableTsCorrect(bool b)
{
	m_tsCorrect = b;
}


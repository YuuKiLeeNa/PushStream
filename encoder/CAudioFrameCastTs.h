#pragma once

#include "Audio/CAudioFrameCast.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/avutil.h"
#ifdef __cplusplus
}
#endif
#include<vector>




class CAudioFrameCastTs :protected CAudioFrameCast 
{
public:
	CAudioFrameCastTs();
	~CAudioFrameCastTs();
	void reset();
	using CAudioFrameCast::set;
	bool translate(UPTR_FME f);
	std::vector<UPTR_FME> getFrame(int iSamples);
	using CAudioFrameCast::operator bool;
	void enableTsCorrect(bool b);
protected:
	int64_t m_ts_offset = 0;// in 1000000000 unit
	int64_t m_ts = AV_NOPTS_VALUE;//currrent timestamp in 1000000000 unit
	bool m_tsCorrect = false;
};
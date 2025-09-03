#ifdef __cplusplus
extern "C"{
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/channel_layout.h"

#ifdef __cplusplus
}
#endif
#include "AudioMixer.h"
#include "utilfun.h"
#include "define_type.h"


int initframe(AVFrame**f,int framelen,int sampleRate, uint64_t channel_layout, AVSampleFormat fmt) 
{
	AVFrame*tmpf = av_frame_alloc();
	if (!f) 
	{
		PrintE("av_frame_alloc failed:no memory");
		return AVERROR(ENOMEM);
	}
	UPTR_FME tmp(tmpf);

	int ret;
	if ((ret = av_channel_layout_from_mask(&tmpf->ch_layout, channel_layout)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_channel_layout_from_mask error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	tmpf->sample_rate = sampleRate;
	tmpf->format = fmt;
	tmpf->nb_samples = framelen;

	if ((ret = av_frame_get_buffer(tmpf, 0)) != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_get_buffer error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}
	
	if ((ret = av_frame_make_writable(tmpf)) != 0) 
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_make_writable error:%s\n", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	*f = tmp.release();
	return 0;
}


struct free_file 
{
	void operator()(FILE* f) { fclose(f); }
};

typedef std::unique_ptr<FILE, free_file>UPTR_FILE;

int main(int argc,char*argv[])
{

	initLog();

	AudioMixer mixer;
	mixer.setOutput(48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
	mixer.addInput(48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,1.0F, 0.0f);
	mixer.addInput(48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 1.0F, 0.0f);

	if (mixer.init(AudioMixer::MixerMode::longest) != 0) 
		return 0;

	auto fun = std::bind(initframe, std::placeholders::_1, 1024, 48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
	

	FILE* file1 = fopen(R"(F:\PushStream\testMixerAduio\shuixian48000_2_s16le.pcm)", "rb");


	FILE* file2 = fopen(R"(F:\PushStream\testMixerAduio\xingfu_48000_2_s16le.pcm)", "rb");


	//FILE* file3 = fopen(R"(F:\PushStream\testMixerAduio\test_48000_2_s16le.pcm)", "wb");


	if (!file1 || !file2 /* || !file3*/)
		return -1;

	UPTR_FILE free_file1(file1);
	UPTR_FILE free_file2(file2);

	int ret;

	for (int i = 0; i < 1900000000; ++i)
	{
		AVFrame* f1 = NULL, * f2 = NULL, * fres = NULL;//av_frame_alloc();

		if (fun(&f1) != 0 || fun(&f2) != 0)
			return -1;

		while (true)
		{
			if ((ret= fread(f1->data[0], 2 * 2, 1024, file1)) != 1024)
			{
				
				fseek(file1, 0, SEEK_SET);
				printf("seek f1\n");
				continue;
			}
			else
				break;
		}

		while (true)
		{
			if ((ret=fread(f2->data[0], 2 * 2, 1024, file2)) != 1024)
			{
				fseek(file2, 0, SEEK_SET);
				printf("seek f2\n");
				continue;
			}
			else
				break;
		}

		UPTR_FME ptr1(f1) ,ptr2(f2);
		//continue;

		if (mixer.addFrame(0, f1) != 0
			|| mixer.addFrame(1, f2) != 0) 
		{
			return -2;
		}
		//ptr1.release();
		//ptr2.release();

		
		//if (initframe(&fres, 1024, 48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLTP) != 0) 
		//	return -3;
		if ((fres = av_frame_alloc()) == NULL) 
		{
			printf("av_frame_alloc no memory\n");
			return -5;
		}

		if (mixer.getFrame(fres, 1024) != 0)
			return -4;

		/*if ((ret=fwrite(fres->data[0], 2 * 2, fres->nb_samples, file3)) != fres->nb_samples)
		{
			printf("fwrite error\n");
		}*/

		if (i % 10 == 0)
			printf("times = %d\n", i);

		UPTR_FME ptrRes(fres);
	}


	return 0;
}



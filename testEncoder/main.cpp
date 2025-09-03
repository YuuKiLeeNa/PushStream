#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/channel_layout.h"

#ifdef __cplusplus
}
#endif
#include "utilfun.h"
#include "pushstream/define_type.h"
//#include "PushStream/encoder/CEncodeHelp.h"
#include "encoder/CEncodeHelp.h"
#include "Util/logger.h"
#include "Network/Socket.h"



int initframe(AVFrame** f, int framelen, int sampleRate, uint64_t channel_layout, AVSampleFormat fmt)
{
	AVFrame* tmpf = av_frame_alloc();
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
		PrintE("av_channel_layout_from_mask error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	tmpf->sample_rate = sampleRate;
	tmpf->format = fmt;
	tmpf->nb_samples = framelen;

	if ((ret = av_frame_get_buffer(tmpf, 0)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	if ((ret = av_frame_make_writable(tmpf)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_make_writable error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	*f = tmp.release();
	return 0;
}



int initvideoframe_yuv420p(AVFrame**f, int w, int h) 
{
	AVFrame*tmpf = av_frame_alloc();
	if (!tmpf)
	{
		PrintE("av_frame_alloc error");
		return AVERROR(ENOMEM);
	}

	UPTR_FME tmp(tmpf);

	tmpf->width = w;
	tmpf->height = h;
	tmpf->format = AV_PIX_FMT_YUV420P;
	
	int ret;
	if ((ret = av_frame_get_buffer(tmpf, 0)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_get_buffer error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
		return ret;
	}

	if ((ret = av_frame_make_writable(tmpf)) != 0)
	{
		char szErr[AV_ERROR_MAX_STRING_SIZE];
		PrintE("av_frame_make_writable error:%s", av_make_error_string(szErr, sizeof(szErr), ret));
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
	CEncodeHelp help;
	CMediaOption option = help.getOption();
	int pic_width = 720;
	int pic_height = 480;



	option["b:a"] = 128000;
	option["b:v"] = 2048 * 1024;
	option["ar"] = 48000;
	option["channel_layout"] = AV_CH_LAYOUT_STEREO;
	option["sample_fmt"] = AV_SAMPLE_FMT_S16;
	option["s"] = AVRational{ pic_width ,pic_height };
	option["aspect"] = AVRational{ 1,1 };
	option["codecid:v"] = (AVCodecID)AV_CODEC_ID_H265;
	option["codecid:a"] = (AVCodecID)AV_CODEC_ID_OPUS;
	option["v:time_base"] = AVRational{ 1,30 };
	option["r"] = AVRational{ 30,1 };
	option["pix_fmt"] = AV_PIX_FMT_YUV420P;

	help.setOption(option);
	help.startRecord("", 3);

	int frame_len = 960;

	FILE* file1 = fopen(R"(F:\PushStream\testMixerAduio\shuixian48000_2_s16le.pcm)", "rb");
	FILE* file2 = fopen(R"(F:\output720_480.yuv)","rb");

	if (!file1 || !file2)
		return -1;
	UPTR_FILE free_file1(file1);
	UPTR_FILE free_file2(file2);
	
	//AVFrame* tmp = NULL;
	//initframe(&tmp, 1024, 48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
	//UPTR_FME tmp_free(tmp);

	int64_t audio_pts = 0;
	int64_t video_pts = 0;

	AVRational audio_base = {1, 48000};
	AVRational video_base = {1, 30 };

	AVRational base = AV_TIME_BASE_Q;
	int ret;
	CFileEncoder::STREAM_TYPE type;
	for (int i = 0; i < 999999; ++i)
	{

		AVFrame* f = NULL;

		if (av_rescale_q_rnd(audio_pts, audio_base, base, AVRounding::AV_ROUND_NEAR_INF)
			< av_rescale_q_rnd(video_pts, video_base, base, AVRounding::AV_ROUND_NEAR_INF)) 
		{
			type = CFileEncoder::STREAM_TYPE::ENCODE_AUDIO;
			if (initframe(&f, frame_len, 48000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16) != 0)
			{
				return -1;
			}
			if (fread(f->data[0], 4, frame_len, file1) != frame_len)
			{
				fseek(file1, 0, SEEK_SET);
				printf("seek f1\n");
				if (fread(f->data[0], 4, frame_len, file1) != frame_len)
				{
					PrintE("fread error");
					av_frame_free(&f);
					return -1;
				}
			}
			f->time_base = audio_base;
			f->pts = audio_pts;
			audio_pts += frame_len;
		}
		else 
		{
			type = CFileEncoder::STREAM_TYPE::ENCODE_VIDEO;
			if((ret = initvideoframe_yuv420p(&f, pic_width, pic_height)) != 0)
			{
				return -1;
			}

			bool bIsSeek = false;
			for (int h = 0; h < pic_height; ++h)
				if (fread(f->data[0] + f->linesize[0] * h, 1, pic_width, file2) != pic_width)
				{
					printf("seek f2\n");
					fseek(file2, 0, SEEK_SET);
					bIsSeek = true;
					break;
				}
			if (bIsSeek)
			{
				av_frame_free(&f);
				continue;
			}
			for (int h = 0; h < pic_height/2; ++h)
				if (fread(f->data[1] + f->linesize[1] * h, 1, pic_width / 2, file2) != pic_width / 2)
				{
					printf("seek f2\n");
					fseek(file2, 0, SEEK_SET);
					bIsSeek = true;
					break;
				}
			if (bIsSeek)
			{
				av_frame_free(&f);
				continue;
			}
			for (int h = 0; h < pic_height/2; ++h)
				if (fread(f->data[2] + f->linesize[2] * h, 1, pic_width / 2, file2) != pic_width / 2)
				{
					printf("seek f2\n");
					fseek(file2, 0, SEEK_SET);
					bIsSeek = true;
					break;
				}

			if (bIsSeek)
			{
				av_frame_free(&f);
				continue;
			}
			f->time_base = video_base;
			f->pts = video_pts++;
		}
		help.push(UPTR_FME(f), type);
		//av_frame_free(&f);
	}
	printf("go to end\n");
	help.EndRecord();


	system("pause");
	return 0;

}
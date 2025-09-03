#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "speex/speex_preprocess.h"
#include <stdio.h>

#include<assert.h>

//#define NN 160
#define NN 1024

int main()
{
   short in[NN];
   int i;
   SpeexPreprocessState *st;
   int count=0;
   float f;

   FILE* file = fopen("E:\\obs record\\48000_1_s16le.pcm", "rb");
   FILE* file_dsp = fopen("E:\\obs record\\48000_1_s16le_dsp.pcm", "wb");

   assert(file && file_dsp);

   //st = speex_preprocess_state_init(NN, 8000);
   st = speex_preprocess_state_init(NN, 48000);
   i=1;
   speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &i);
   spx_int32_t de_level = -80;
   speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &de_level);


   i=0;
   speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &i);
   i=80000;
   //i = 48000;
   speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);
   i=1;
   speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB, &i);
   f=0.0;
   speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
   f=.0;
   //f = 25.0;
   speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);
   //f = 25.0;
  

   while (1)
   {
      int vad;
      if (fread(in, sizeof(short), NN, file) == NN) 
      {
          vad = speex_preprocess_run(st, in);
          fwrite(in, sizeof(short), NN, file_dsp);
          count++;
      }
      else
      //if (feof(file))
         break;
      //vad = speex_preprocess_run(st, in);
      /*fprintf (stderr, "%d\n", vad);*/
     /* fwrite(in, sizeof(short), NN, file_dsp);
      count++;*/
   }
   speex_preprocess_state_destroy(st);
   fflush(file_dsp);

   fclose(file);
   fclose(file_dsp);
   return 0;
}

/*
	decode_ntom.c: N->M down/up sampling. Not optimized for speed.

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123lib_intern.h"

int synth_ntom_set_step(mpg123_handle *fr)
{
	long m,n;
	m = frame_freq(fr);
	n = fr->af.rate;
	if(VERBOSE2)
		fprintf(stderr,"Init rate converter: %ld->%ld\n",m,n);

	if(n > NTOM_MAX_FREQ || m > NTOM_MAX_FREQ || m <= 0 || n <= 0) {
		if(NOQUIET) error("NtoM converter: illegal rates");
		fr->err = MPG123_BAD_RATE;
		return -1;
	}

	n *= NTOM_MUL;
	fr->ntom_step = (unsigned long) n / m;

	if(fr->ntom_step > (unsigned long)NTOM_MAX*NTOM_MUL) {
		if(NOQUIET) error3("max. 1:%i conversion allowed (%lu vs %lu)!", NTOM_MAX, fr->ntom_step, (unsigned long)8*NTOM_MUL);
		fr->err = MPG123_BAD_RATE;
		return -1;
	}

	fr->ntom_val[0] = fr->ntom_val[1] = NTOM_MUL>>1;
	return 0;
}

int synth_ntom_8bit(real *bandPtr,int channel, mpg123_handle *fr, int final)
{
  sample_t samples_tmp[8*64];
  sample_t *tmp1 = samples_tmp + channel;
  int i,ret;

  int pnt = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_ntom(bandPtr, channel, fr, 1);
  fr->buffer.data = samples;

  samples += channel + pnt;
  for(i=0;i<(fr->buffer.fill>>2);i++) {
#ifdef FLOATOUT
    *samples = 0;
#else
    *samples = fr->conv16to8[*tmp1>>AUSHIFT];
#endif
    samples += 2;
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (final ? fr->buffer.fill>>1 : 0);

  return ret;
}

int synth_ntom_8bit_mono(real *bandPtr, mpg123_handle *fr)
{
  sample_t samples_tmp[8*64];
  sample_t *tmp1 = samples_tmp;
  int i,ret;

  int pnt = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_ntom(bandPtr, 0, fr, 1);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<(fr->buffer.fill>>2);i++) {
#ifdef FLOATOUT
    *samples++ = 0;
#else
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
#endif
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (fr->buffer.fill>>2);
  
  return ret;
}

int synth_ntom_8bit_mono2stereo(real *bandPtr, mpg123_handle *fr)
{
  sample_t samples_tmp[8*64];
  sample_t *tmp1 = samples_tmp;
  int i,ret;

  int pnt = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_ntom(bandPtr, 0, fr, 1);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<(fr->buffer.fill>>2);i++) {
#ifdef FLOATOUT
    *samples++ = 0;
    *samples++ = 0;
#else
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
    *samples++ = fr->conv16to8[*tmp1>>AUSHIFT];
#endif
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (fr->buffer.fill>>1);

  return ret;
}

int synth_ntom_mono(real *bandPtr, mpg123_handle *fr)
{
  sample_t samples_tmp[8*64];
  sample_t *tmp1 = samples_tmp;
  int i,ret;

  int pnt = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data;
  fr->buffer.data = (unsigned char*) samples_tmp;
  fr->buffer.fill = 0;
  ret = synth_ntom(bandPtr, 0, fr, 1);
  fr->buffer.data = samples;

  samples += pnt;
  for(i=0;i<(fr->buffer.fill>>2);i++) {
    *( (sample_t *)samples) = *tmp1;
    samples += sizeof(sample_t);
    tmp1 += 2;
  }
  fr->buffer.fill = pnt + (fr->buffer.fill>>2)*sizeof(sample_t);

  return ret;
}


int synth_ntom_mono2stereo(real *bandPtr, mpg123_handle *fr)
{
  int i,ret;
  int pnt1 = fr->buffer.fill;
  unsigned char *samples = fr->buffer.data + pnt1;

  ret = synth_ntom(bandPtr, 0, fr, 1);

  for(i=0;i<((fr->buffer.fill-pnt1)>>2);i++) {
    ((sample_t *)samples)[1] = ((sample_t *)samples)[0];
    samples+=2*sizeof(sample_t);
  }

  return ret;
}


int synth_ntom(real *bandPtr,int channel, mpg123_handle *fr, int final)
{
  static const int step = 2;
  sample_t *samples = (sample_t *) (fr->buffer.data + fr->buffer.fill);

  real *b0, **buf; /* (*buf)[0x110]; */
  int clip = 0; 
  int bo1;
  int ntom;

  if(fr->have_eq_settings) do_equalizer(bandPtr,channel,fr->equalizer);

  if(!channel) {
    fr->bo[0]--;
    fr->bo[0] &= 0xf;
    buf = fr->real_buffs[0];
    ntom = fr->ntom_val[1] = fr->ntom_val[0];
  }
  else {
    samples++;
    buf = fr->real_buffs[1];
    ntom = fr->ntom_val[1];
  }

  if(fr->bo[0] & 0x1) {
    b0 = buf[0];
    bo1 = fr->bo[0];
    opt_dct64(fr)(buf[1]+((fr->bo[0]+1)&0xf),buf[0]+fr->bo[0],bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = fr->bo[0]+1;
    opt_dct64(fr)(buf[0]+fr->bo[0],buf[1]+fr->bo[0]+1,bandPtr);
  }


  {
    register int j;
    real *window = opt_decwin(fr) + 16 - bo1;
 
    for (j=16;j;j--,window+=0x10)
    {
      real sum;

      ntom += fr->ntom_step;
      if(ntom < NTOM_MUL) {
        window += 16;
        b0 += 16;
        continue;
      }

      sum  = *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;

      while(ntom >= NTOM_MUL) {
        WRITE_SAMPLE(samples,sum,clip);
        samples += step;
        ntom -= NTOM_MUL;
      }
    }

    ntom += fr->ntom_step;
    if(ntom >= NTOM_MUL)
    {
      real sum;
      sum  = window[0x0] * b0[0x0];
      sum += window[0x2] * b0[0x2];
      sum += window[0x4] * b0[0x4];
      sum += window[0x6] * b0[0x6];
      sum += window[0x8] * b0[0x8];
      sum += window[0xA] * b0[0xA];
      sum += window[0xC] * b0[0xC];
      sum += window[0xE] * b0[0xE];

      while(ntom >= NTOM_MUL) {
        WRITE_SAMPLE(samples,sum,clip);
        samples += step;
        ntom -= NTOM_MUL;
      }
    }

    b0-=0x10,window-=0x20;
    window += bo1<<1;

    for (j=15;j;j--,b0-=0x20,window-=0x10)
    {
      real sum;

      ntom += fr->ntom_step;
      if(ntom < NTOM_MUL) {
        window -= 16;
        b0 += 16;
        continue;
      }

      sum = -*(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;

      while(ntom >= NTOM_MUL) {
        WRITE_SAMPLE(samples,sum,clip);
        samples += step;
        ntom -= NTOM_MUL;
      }
    }
  }

  fr->ntom_val[channel] = ntom;
  if(final) fr->buffer.fill = ((unsigned char *) samples - fr->buffer.data - (channel ? 2 : 0));

  return clip;
}



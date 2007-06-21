/*
	decode.h: common definitions for decode functions

	copyright 2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, taking WRITE_SAMPLE from decode.c
*/
#ifndef MPG123_DECODE_H
#define MPG123_DECODE_H

#ifdef FLOATOUT
#define WRITE_SAMPLE(samples,sum,clip) *(samples) = sum
#define sample_t float
#else
#define WRITE_SAMPLE(samples,sum,clip) \
  if( (sum) > REAL_PLUS_32767) { *(samples) = 0x7fff; (clip)++; } \
  else if( (sum) < REAL_MINUS_32768) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = REAL_TO_SHORT(sum); }
#define sample_t short
#endif

/* synth_1to1 in optimize.h, one should also use opts for these here... */

int synth_2to1 (real *,int, struct frame*, int);
int synth_2to1_8bit (real *,int, struct frame *,int);
int synth_2to1_mono (real *, struct frame *);
int synth_2to1_mono2stereo (real *, struct frame *);
int synth_2to1_8bit_mono (real *, struct frame *);
int synth_2to1_8bit_mono2stereo (real *, struct frame *);

int synth_4to1 (real *,int, struct frame*, int);
int synth_4to1_8bit (real *,int, struct frame *,int);
int synth_4to1_mono (real *, struct frame *);
int synth_4to1_mono2stereo (real *, struct frame *);
int synth_4to1_8bit_mono (real *, struct frame *);
int synth_4to1_8bit_mono2stereo (real *, struct frame *);

int synth_ntom (real *,int, struct frame*, int);
int synth_ntom_8bit (real *,int, struct frame *,int);
int synth_ntom_mono (real *, struct frame *);
int synth_ntom_mono2stereo (real *, struct frame *);
int synth_ntom_8bit_mono (real *, struct frame *);
int synth_ntom_8bit_mono2stereo (real *, struct frame *);

int synth_ntom_set_step(struct frame *fr, long,long);

int do_layer3(struct frame *fr,int,struct audio_info_struct *);
int do_layer2(struct frame *fr,int,struct audio_info_struct *);
int do_layer1(struct frame *fr,int,struct audio_info_struct *);
void do_equalizer(real *bandPtr,int channel, real equalizer[2][32]);

#endif

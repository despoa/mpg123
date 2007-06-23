#ifndef MPG123_PARSE_H
#define MPG123_PARSE_H

#include "frame.h"

int read_frame_init(struct frame* fr);
int frame_bitrate(struct frame *fr);
long frame_freq(struct frame *fr);
int read_frame_recover(struct frame* fr); /* dead? */
int read_frame(struct frame *fr);
void set_pointer(struct frame *fr, long backstep);
int position_info(struct frame* fr, unsigned long no, long buffsize, unsigned long* frames_left, double* current_seconds, double* seconds_left);
double compute_tpf(struct frame *fr);
double compute_bpf(struct frame *fr);
long time_to_frame(struct frame *fr, double seconds);
int get_songlen(struct frame *fr,int no);
#ifdef GAPLESS
unsigned long samples_to_bytes(struct frame *fr , unsigned long s);
#endif

#endif

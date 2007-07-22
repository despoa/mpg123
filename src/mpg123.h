/*
	mpg123: main code of the program (not of the decoder...)

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	mpg123 defines 
	used source: musicout.h from mpegaudio package
*/

#ifndef MPG123_H
#define MPG123_H

/* everyone needs it */
#include "config.h"
#include "debug.h"
#include "httpget.h"
#include "mpg123lib.h"
#define MPG123_REMOTE
#define REMOTE_BUFFER_SIZE 2048

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include        <stdio.h>
#include        <string.h>
#include        <signal.h>

#ifndef WIN32
#include        <sys/signal.h>
#include        <unistd.h>
#endif
/* want to suport large files in future */
#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#ifndef off_t
	#define off_t long
#endif
#include        <math.h>

#ifdef OS2
#include <float.h>
#endif

#ifdef MPG123_WIN32
#undef MPG123_REMOTE           /* Get rid of this stuff for Win32 */
#endif

typedef unsigned char byte;
#include "xfermem.h"

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

#include "audio.h"

#define VERBOSE_MAX 3

struct parameter
{
  int aggressive; /* renice to max. priority */
  int shuffle;	/* shuffle/random play */
  int remote;	/* remote operation */
  int remote_err;	/* remote operation to stderr */
  int outmode;	/* where to out the decoded sampels */
  int quiet;	/* shut up! */
  int xterm_title;	/* Change xterm title to song names? */
  long usebuffer;	/* second level buffer size */
  int verbose;    /* verbose level */
#ifdef HAVE_TERMIOS
  int term_ctrl;
#endif
  int checkrange;
  int force_reopen;
  int test_cpu;
  long realtime;
  char filename[256];
  long listentry; /* possibility to choose playback of one entry in playlist (0: off, > 0 : select, < 0; just show list*/
  char* listname; /* name of playlist */
  int long_id3;
  int list_cpu;
	char *cpu;
#ifdef FIFO
	char* fifo;
#endif
	/* parameters for mpg123 handle */
	int down_sample;
	long rva; /* (which) rva to do: 0: nothing, 1: radio/mix/track 2: album/audiophile */
	long halfspeed;
	long doublespeed;
	long start_frame;  /* frame offset to begin with */
	long frame_number; /* number of frames to decode */
#ifdef FLOATOUT
	double outscale;
#else
	long outscale;
#endif
	int flags;
	long force_rate;
};

extern char *equalfile;
extern long framenum;
extern struct httpdata htd;

extern int buffer_fd[2];
extern txfermem *buffermem;

#ifndef NOXFERMEM
extern void buffer_loop(struct audio_info_struct *ai,sigset_t *oldsigset);
#endif

extern int OutputDescriptor;

#ifdef VARMODESUPPORT
extern int varmode;
extern int playlimit;
#endif

/* why extern? */
void prepare_audioinfo(struct frame *fr, struct audio_info_struct *nai);
extern int play_frame(void);

extern int control_generic(struct frame *fr);

extern int cdr_open(struct audio_info_struct *ai, char *ame);
extern int au_open(struct audio_info_struct *ai, char *name);
extern int wav_open(struct audio_info_struct *ai, char *wavfilename);
extern int wav_write(unsigned char *buf,int len);
extern int cdr_close(void);
extern int au_close(void);
extern int wav_close(void);

extern int au_open(struct audio_info_struct *ai, char *aufilename);
extern int au_close(void);

extern int cdr_open(struct audio_info_struct *ai, char *cdrfilename);
extern int cdr_close(void);

extern struct parameter param;

/* avoid the SIGINT in terminal control */
void next_track(void);
int  open_track(char *fname);
void close_track(void);
#endif 

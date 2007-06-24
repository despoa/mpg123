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

#include "mpg123lib.h"

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

typedef unsigned char byte;

#ifdef OS2
#include <float.h>
#endif

#define MPG123_REMOTE
#define REMOTE_BUFFER_SIZE 2048


#ifdef MPG123_WIN32
#undef MPG123_REMOTE           /* Get rid of this stuff for Win32 */
#endif

#include "xfermem.h"

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

#include "audio.h"


#include "id3.h"
#include "getcpuflags.h"
#include "frame.h"

#define VERBOSE_MAX 3

struct parameter {
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
  long doublespeed;
  int force_reopen;
  /* the testing part shared between 3dnow and multi mode */
#ifdef OPT_MULTI
  int test_cpu;
#endif
  long realtime;
  char filename[256];
  long listentry; /* possibility to choose playback of one entry in playlist (0: off, > 0 : select, < 0; just show list*/
  char* listname; /* name of playlist */
  int long_id3;
#ifdef OPT_MULTI
  int list_cpu;
#endif
#ifdef FIFO
	char* fifo;
#endif
};

extern char *equalfile;

extern int buffer_fd[2];
extern txfermem *buffermem;

#ifndef NOXFERMEM
extern void buffer_loop(struct audio_info_struct *ai,sigset_t *oldsigset);
#endif

/* ------ Declarations from "httpget.c" ------ */

extern char *proxyurl;
extern unsigned long proxyip;
/* takes url and content type string address, opens resource, returns fd for data, allocates and sets content type */
extern int http_open (struct frame *fr, char* url, char** content_type);
extern char *httpauth;

extern int OutputDescriptor;

#ifdef VARMODESUPPORT
extern int varmode;
extern int playlimit;
#endif

#include "parse.h"

/* why extern? */
void prepare_audioinfo(struct frame *fr, struct audio_info_struct *nai);
extern int play_frame(int init,struct frame *fr);

#include "reader.h"
#include "decode.h"

extern void rewindNbits(int bits);
extern int  hsstell(void);
extern void set_pointer(struct frame*, long);
extern void huffman_decoder(int ,int *);
extern void huffman_count1(int,int *);

extern void init_layer3(void);
extern void init_layer3_stuff(struct frame *fr);
extern void init_layer2(void);
extern void init_layer2_stuff(struct frame *fr);
extern int make_conv16to8_table(struct frame *fr, int);

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

extern struct audio_name audio_val2name[];

extern struct parameter param;

/* avoid the SIGINT in terminal control */
void next_track(void);

#include "optimize.h"

#endif 

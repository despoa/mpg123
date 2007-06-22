/*
	mpg123: main code of the program (not of the decoder...)

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	mpg123 defines 
	used source: musicout.h from mpegaudio package
*/

/* everyone needs it */
#include "config.h"
#include "debug.h"

struct frame; /* a forward declaration */

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



#ifndef _AUDIO_H_
#define _AUDIO_H_

typedef unsigned char byte;

#ifdef OS2
#include <float.h>
#endif

#define MPG123_REMOTE
#define REMOTE_BUFFER_SIZE 2048
#ifdef HPUX
#define random rand
#define srandom srand
#endif

#define SKIP_JUNK 1

#ifdef _WIN32	/* Win32 Additions By Tony Million */
# undef WIN32
# define WIN32

# define M_PI       3.14159265358979323846
# define M_SQRT2	1.41421356237309504880
# ifndef REAL_IS_FLOAT
#  define REAL_IS_FLOAT
# endif
# define NEW_DCT9

# define random rand
# define srandom srand

# undef MPG123_REMOTE           /* Get rid of this stuff for Win32 */
#endif

#include "xfermem.h"

#ifdef SUNOS
#define memmove(dst,src,size) bcopy(src,dst,size)
#endif

#ifdef REAL_IS_FLOAT
#  define real float
#  define REAL_SCANF "%f"
#  define REAL_PRINTF "%f"
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#  define REAL_SCANF "%Lf"
#  define REAL_PRINTF "%Lf"
#elif defined(REAL_IS_FIXED)
# define real long

# define REAL_RADIX            15
# define REAL_FACTOR           (32.0 * 1024.0)

# define REAL_PLUS_32767       ( 32767 << REAL_RADIX )
# define REAL_MINUS_32768      ( -32768 << REAL_RADIX )

# define DOUBLE_TO_REAL(x)     ((int)((x) * REAL_FACTOR))
# define REAL_TO_SHORT(x)      ((x) >> REAL_RADIX)
# define REAL_MUL(x, y)                (((long long)(x) * (long long)(y)) >> REAL_RADIX)
#  define REAL_SCANF "%ld"
#  define REAL_PRINTF "%ld"

#else
#  define real double
#  define REAL_SCANF "%lf"
#  define REAL_PRINTF "%f"
#endif

#ifndef DOUBLE_TO_REAL
# define DOUBLE_TO_REAL(x)     (x)
#endif
#ifndef REAL_TO_SHORT
# define REAL_TO_SHORT(x)      (x)
#endif
#ifndef REAL_PLUS_32767
# define REAL_PLUS_32767       32767.0
#endif
#ifndef REAL_MINUS_32768
# define REAL_MINUS_32768      -32768.0
#endif
#ifndef REAL_MUL
# define REAL_MUL(x, y)                ((x) * (y))
#endif

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

#include "audio.h"

/* AUDIOBUFSIZE = n*64 with n=1,2,3 ...  */
#define		AUDIOBUFSIZE		16384

#define         FALSE                   0
#define         TRUE                    1

#define         MAX_NAME_SIZE           81
#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/* float output only for generic decoder! */
#ifdef FLOATOUT
#define MAXOUTBURST 1.0
#define scale_t double
#else
/* I suspect that 32767 would be a better idea here, but Michael put this in... */
#define MAXOUTBURST 32768
#define scale_t long
#endif

/* Pre Shift fo 16 to 8 bit converter table */
#define AUSHIFT (3)

#include "id3.h"
#include "getcpuflags.h"
#include "frame.h"

#define VERBOSE_MAX 3

#define MONO_LEFT 1
#define MONO_RIGHT 2
#define MONO_MIX 4

struct parameter {
  int aggressive; /* renice to max. priority */
  int shuffle;	/* shuffle/random play */
  int remote;	/* remote operation */
  int remote_err;	/* remote operation to stderr */
  int outmode;	/* where to out the decoded sampels */
  int quiet;	/* shut up! */
  int xterm_title;	/* Change xterm title to song names? */
  long usebuffer;	/* second level buffer size */
  int tryresync;  /* resync stream after error */
  int verbose;    /* verbose level */
#ifdef HAVE_TERMIOS
  int term_ctrl;
#endif
  int force_mono;
  int force_stereo;
  int force_8bit;
  long force_rate;
  int down_sample;
  int checkrange;
  long doublespeed;
  long halfspeed;
  int force_reopen;
  /* the testing part shared between 3dnow and multi mode */
#ifdef OPT_3DNOW
  int stat_3dnow; /* automatic/force/force-off 3DNow! optimized code */
#endif
#ifdef OPT_MULTI
  int test_cpu;
#endif
  long realtime;
  char filename[256];
#ifdef GAPLESS	
  int gapless; /* (try to) remove silence padding/delay to enable gapless playback */
#endif
  long listentry; /* possibility to choose playback of one entry in playlist (0: off, > 0 : select, < 0; just show list*/
  int rva; /* (which) rva to do: 0: nothing, 1: radio/mix/track 2: album/audiophile */
  char* listname; /* name of playlist */
  int long_id3;
  #ifdef OPT_MULTI
  char* cpu; /* chosen optimization, can be NULL/""/"auto"*/
  int list_cpu;
  #endif
#ifdef FIFO
	char* fifo;
#endif
};

extern char *equalfile;

extern int halfspeed;
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

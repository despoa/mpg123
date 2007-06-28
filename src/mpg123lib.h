#ifndef MPG123_LIB_H
#define MPG123_LIB_H

/* not decided... how anonymous should the handle be? */
struct frame;
typedef struct frame mpg123_handle;

struct mpg123_parameter
{
	int verbose;    /* verbose level */
#define MPG123_FORCE_MONO   0x7  /*     0111 */
#define MPG123_MONO_LEFT    0x1  /*     0001 */
#define MPG123_MONO_RIGHT   0x2  /*     0010 */
#define MPG123_MONO_MIX     0x4  /*     0100 */
#define MPG123_FORCE_STEREO 0x8  /*     1000 */
#define MPG123_FORCE_8BIT   0x10 /* 00010000 */
#define MPG123_QUIET        0x20 /* 00100000 suppress any printouts (overrules verbose) */
#define MPG123_GAPLESS      0x40 /* 01000000 flag always defined... */
#define MPG123_NO_RESYNC    0x80 /* 10000000 disable resync stream after error */
	int flags; /* combination of above */
	long force_rate;
	int down_sample;
	int rva; /* (which) rva to do: 0: nothing, 1: radio/mix/track 2: album/audiophile */
#ifdef OPT_MULTI
	char* cpu; /* chosen optimization, can be NULL/""/"auto"*/
#endif
	long halfspeed;
	long doublespeed;
	int formats; /* possible formats... modelling mpg123 output device */
#define NUM_CHANNELS 2
#define NUM_ENCODINGS 6
#define NUM_RATES 10
};

/* non-threadsafe init/exit, call _once_ */
void mpg123_init(void);
void mpg123_exit(void);
/* threadsafely create and delete handles */
mpg123_handle* mpg123_new(void);
void mpg123_delete(mpg123_handle *mh);

/* 16 or 8 bits, signed or unsigned... all flags fit into 16 bits, float/double are not yet standard and special anyway */
#define MPG123_ENC_16     0x40 /* 0100 0000 */
#define MPG123_ENC_SIGNED 0x80 /* 1000 0000 */

#define MPG123_ENC_SIGNED_16    (MPG123_ENC_16|MPG123_ENC_SIGNED|0x10) /* 1101 0000 */
#define MPG123_ENC_UNSIGNED_16  (MPG123_ENC_16|0x20)                   /* 0110 0000 */
#define MPG123_ENC_UNSIGNED_8   0x01                                   /* 0000 0001 */
#define MPG123_ENC_SIGNED_8     (MPG123_SIGNED|0x02)                   /* 1000 0010 */
#define MPG123_ENC_ULAW_8       0x04                                   /* 0000 0100 */
#define MPG123_ENC_ALAW_8       0x08                                   /* 0000 1000 */

#define MPG123_ENC_ANY ( MPG123_ENC_SIGNED_16  | MPG123_ENC_UNSIGNED_16 | \
                         MPG123_ENC_UNSIGNED_8 | MPG123_ENC_SIGNED_8    | \
                         MPG123_ENC_ULAW_8 | MPG123_ENC_ALAW_8 | MPG123_ENC_ANY )

/* kind of trivial... but symbolic */
#define MPG123_MONO 1
#define MPG123_STEREO 2

int mpg123_format_clear(); /* clear table of possible output formats */
/* possible output formats for chosen rate and channels
   rate is one of 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 or _one_ custom rate <=96000 */
int mpg123_format(mpg123_handle *mh, long rate, int channels, int encodings);
int mpg123_get_output(mpg123_handle *mh, int *format, int *channels, long *rate);

int mpg123_open_feed  (mpg123_handle *mh);
int mpg123_open_stream(mpg123_handle *mh, char *url);
int mpg123_open_fd    (mpg123_handle *mh, int fd);

void mpg123_close(mpg123_handle *mh);


/* replacement for mpglib's decodeMP3, same usage */
#define MPG123_ERR -1
#define MPG123_OK  0
#define MPG123_NEED_MORE -10
int mpg123_decode(mpg123_handle *mh, unsigned char *inmemory,int inmemsize, unsigned char *outmemory,int outmemsize,int *done);
void mpg123_track(mpg123_handle *mh); /* clear out buffers/state for track change */



/* missing various functions to change properties: RVA, equalizer */
/* also: functions to access properties: ID3, RVA, ... */
#endif

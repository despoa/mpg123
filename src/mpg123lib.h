#ifndef MPG123_LIB_H
#define MPG123_LIB_H

/* not decided... how anonymous should the handle be? */
struct frame;
typedef struct frame mpg123_handle;

/* non-threadsafe init/exit, call _once_ */
void mpg123_init(void);
void mpg123_exit(void);
/* threadsafely create and delete handles */
mpg123_handle* mpg123_new(void);
void mpg123_delete(mpg123_handle *mh);

/* 16 or 8 bits, signed or unsigned... all flags fit into 16 bits, float/double are not yet standard and special anyway */
#define MPG123_ENC_16     0x40 /* 0100 0000 */
#define MPG123_ENC_SIGNED 0x80 /* 1000 0000 */
#define MPG123_ENC_8(f)   (!((f) & MPG123_ENC_16)) /* it's 8bit encoding of not 16bit, this changes in case float output will be integrated in the normal library */

#define MPG123_ENC_SIGNED_16    (MPG123_ENC_16|MPG123_ENC_SIGNED|0x10) /* 1101 0000 */
#define MPG123_ENC_UNSIGNED_16  (MPG123_ENC_16|0x20)                   /* 0110 0000 */
#define MPG123_ENC_UNSIGNED_8   0x01                                   /* 0000 0001 */
#define MPG123_ENC_SIGNED_8     (MPG123_ENC_SIGNED|0x02)                   /* 1000 0010 */
#define MPG123_ENC_ULAW_8       0x04                                   /* 0000 0100 */
#define MPG123_ENC_ALAW_8       0x08                                   /* 0000 1000 */

#define MPG123_ENC_ANY ( MPG123_ENC_SIGNED_16  | MPG123_ENC_UNSIGNED_16 | \
                         MPG123_ENC_UNSIGNED_8 | MPG123_ENC_SIGNED_8    | \
                         MPG123_ENC_ULAW_8 | MPG123_ENC_ALAW_8 | MPG123_ENC_ANY )

enum mpg123_errors { MPG123_ERR_NOTHING=0, MPG123_ERR_OUTFORMAT, MPG123_ERR_CHANNEL,
                     MPG123_ERR_RATE, MPG123_ERR_16TO8TABLE, MPG123_BAD_PARAM};

/* accept no output format at all, used before specifying supported formats with mpg123_format */
void mpg123_format_none(mpg123_handle *mh);
/* accept all formats, also accepts _any_ forced sample rate (resets the custom rate) */
void mpg123_format_all(mpg123_handle *mh);
/*
	Mark a set of formats as supported:
	rate: 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 or _one_ custom rate <=96000
	channels: 1 (mono) or 2 (stereo)
	encodings: combination of accepted encodings for rate and channels, p.ex MPG123_ENC_SIGNED16|MPG123_ENC_ULAW_8

	When the rate is a non-standard one, the one supported custom rate is set to that value.
	After that, forcing the output rate to a specific value will only work with that one rate.
*/
int mpg123_format(mpg123_handle *mh, long rate, int channels, int encodings); /* 0 is good, -1 is error */

#define MPG123_FORCE_MONO   0x7  /*     0111 */
#define MPG123_MONO_LEFT    0x1  /*     0001 */
#define MPG123_MONO_RIGHT   0x2  /*     0010 */
#define MPG123_MONO_MIX     0x4  /*     0100 */
#define MPG123_FORCE_STEREO 0x8  /*     1000 */
#define MPG123_FORCE_8BIT   0x10 /* 00010000 */
#define MPG123_QUIET        0x20 /* 00100000 suppress any printouts (overrules verbose) */
#define MPG123_GAPLESS      0x40 /* 01000000 flag always defined... */
#define MPG123_NO_RESYNC    0x80 /* 10000000 disable resync stream after error */
enum mpg123_parms { MPG123_VERBOSE, MPG123_FLAGS, MPG123_FORCE_RATE, MPG123_DOWN_SAMPLE,
                    MPG123_RVA, MPG123_HALFSPEED, MPG123_DOUBLESPEED,
                    MPG123_SPECIAL_RATE, MPG123_START_FRAME, MPG123_FRAME_NUM, MPG123_ADD_FLAGS };
int mpg123_param(mpg123_handle *mh, int key, long value);
int mpg123_force_rate(mpg123_handle *mh, long rate); /* force the specific sample rate for all playback from now on, 0 is good, -1 is error */

/* The open fucntions reset stuff and make a new, different stream possible - even if there isn't actually a resource involved like with open_feed. */
int mpg123_open     (mpg123_handle *mh, char *url); /* a file or http url */
int mpg123_open_feed(mpg123_handle *mh);            /* prepare for direct feeding */
int mpg123_open_fd  (mpg123_handle *mh, int fd);    /* use an already opened file descriptor */

void mpg123_close(mpg123_handle *mh);

/* replacement for mpglib's decodeMP3, similar usage */
#define MPG123_ERR -1
#define MPG123_OK  0
#define MPG123_NEED_MORE  -10
#define MPG123_NEW_FORMAT -11
int mpg123_decode(mpg123_handle *mh, unsigned char *inmemory,int inmemsize, unsigned char *outmemory,int outmemsize,int *done);

/* missing various functions to change properties: RVA, equalizer */
/* also: functions to access properties: ID3, RVA, ... */
#endif

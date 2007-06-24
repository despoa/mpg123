#ifndef MPG123_LIB_H
#define MPG123_LIB_H

#include "config.h"


#define SKIP_JUNK 1

/* should these really be here? */
#ifdef _WIN32	/* Win32 Additions By Tony Million */
# undef MPG123_WIN32
# define MPG122_WIN32

# define M_PI       3.14159265358979323846
# define M_SQRT2	1.41421356237309504880
# ifndef REAL_IS_FLOAT
#  define REAL_IS_FLOAT
# endif
# define NEW_DCT9
#endif

#ifdef SUNOS
#define memmove(dst,src,size) bcopy(src,dst,size)
#endif

/* some stuff has to go back to mpg123.h */
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

#define MPG123_FORMAT_MASK 0x100
#define MPG123_FORMAT_16   0x100
#define MPG123_FORMAT_8    0x000

#define MPG123_FORMAT_SIGNED_16    0x110
#define MPG123_FORMAT_UNSIGNED_16  0x120
#define MPG123_FORMAT_UNSIGNED_8   0x1
#define MPG123_FORMAT_SIGNED_8     0x2
#define MPG123_FORMAT_ULAW_8       0x4
#define MPG123_FORMAT_ALAW_8       0x8

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
};

/* non-threadsafe init/exit, call _once_ */
void mpg123_init(void);
void mpg123_exit(void);
/* threadsafely create and delete handles */
mpg123_handle* mpg123_new(void);
void mpg123_delete(mpg123_handle *mh);

/* set/get output parameters, default 16bit stereo */
#define MPG123_ANY -1 /* Do not enforce this output parameter. */
int  mpg123_output    (mpg123_handle *mh, int  format, int  channels, long rate);
int  mpg123_get_output(mpg123_handle *mh, int *format, int *channels, long rate);

int mpg123_open  (mpg123_handle *mh, char *url);
int mpg123_fdopen(mpg123_handle *mh, int fd);

void mpg123_close(mpg123_handle *mh);


/* replacement for mpglib's decodeMP3, same usage */
#define MPG123_ERR -1
#define MPG123_OK  0
#define MPG123_NEED_MORE 1
int mpg123_decode(mpg123_handle *mh,char *inmemory,int inmemsize, char *outmemory,int outmemsize,int *done);
void mpg123_track(mpg123_handle *mh); /* clear out buffers/state for track change */



/* missing various functions to change properties: RVA, equalizer */
/* also: functions to access properties: ID3, RVA, ... */
#endif

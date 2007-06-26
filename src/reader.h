#ifndef MPG123_READER_H
#define MPG123_READER_H

#include "config.h"
#include "mpg123lib.h"

struct buffy
{
	unsigned char *data;
	off_t size;
	struct buffy *next;
};

struct reader_data
{
	off_t filelen; /* total file length or total buffer size */
	off_t filepos; /* position in file or position in buffer chain */
	int   filept;
	int   flags;
	unsigned char id3buf[128];
	/* variables specific to feed reader */
	off_t firstpos; /* the point of return on non-forget() */
	struct buffy *buf;  /* first in buffer chain */
};

/* start to use off_t to properly do LFS in future ... used to be long */
struct reader
{
	int     (*init)           (struct frame *);
	void    (*close)          (struct frame *);
	ssize_t (*fullread)       (struct frame *, unsigned char *, ssize_t);
	int     (*head_read)      (struct frame *, unsigned long *newhead);    /* succ: TRUE, else <= 0 (FALSE or READER_MORE) */
	int     (*head_shift)     (struct frame *, unsigned long *head);       /* succ: TRUE, else <= 0 (FALSE or READER_MORE) */
	off_t   (*skip_bytes)     (struct frame *, off_t len);                 /* succ: >=0, else error or READER_MORE         */
	int     (*read_frame_body)(struct frame *, unsigned char *, int size);
	int     (*back_bytes)     (struct frame *, off_t bytes);
	int     (*back_frame)     (struct frame *, long num);
	off_t   (*tell)           (struct frame *);
	void    (*rewind)         (struct frame *);
	void    (*forget)         (struct frame *);
};

int open_stream(struct frame *, char *, int fd);
/* feed based operation has some specials */
int  open_feed(struct frame *);
/* externally called function, returns 0 on success, -1 on error */
int  feed_more(struct frame *fr, unsigned char *in, long count);
void feed_forget(struct frame *fr);  /* forget the data that has been read (free some buffers) */

#define READER_FD_OPENED 0x1
#define READER_ID3TAG    0x2
#define READER_SEEKABLE  0x4
#define READER_BUFFERED  0x8
#define READER_MICROSEEK 0x10

#define READER_STREAM 0
#define READER_ICY_STREAM 1
#define READER_FEED       2

#ifdef READ_SYSTEM
#define READER_SYSTEM 3
#define READERS 4
#else
#define READERS 3
#endif

#define READER_ERROR -1
#define READER_MORE  MPG123_NEED_MORE

#endif

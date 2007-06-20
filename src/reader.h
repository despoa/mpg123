#ifndef MPG123_READER_H
#define MPG123_READER_H

struct reader_data
{
	off_t filelen;
	off_t filepos;
	int   filept;
	int   flags;
	unsigned char id3buf[128];
};

/* start to use off_t to properly do LFS in future ... used to be long */
struct reader
{
	int     (*init)           (struct frame *);
	void    (*close)          (struct frame *);
	ssize_t (*fullread)       (struct frame *, unsigned char *, ssize_t);
	int     (*head_read)      (struct frame *, unsigned long *newhead);
	int     (*head_shift)     (struct frame *, unsigned long *head);
	off_t   (*skip_bytes)     (struct frame *, off_t len);
	int     (*read_frame_body)(struct frame *, unsigned char *, int size);
	int     (*back_bytes)     (struct frame *, off_t bytes);
	int     (*back_frame)     (struct frame *, long num);
	off_t   (*tell)           (struct frame *);
	void    (*rewind)         (struct frame *);
};
#define READER_FD_OPENED 0x1
#define READER_ID3TAG    0x2
#define READER_SEEKABLE  0x4

#define READER_STREAM 0
#define READER_ICY_STREAM 1

#ifdef READ_SYSTEM
#define READER_SYSTEM 2
#define READERS 3
#else
#define READERS 2
#endif

#endif

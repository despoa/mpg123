#ifndef MPG123_READER_H
#define MPG123_READER_H


/* start to use off_t to properly do LFS in future ... used to be long */
struct reader {
  int  (*init)(struct reader *);
  void (*close)(struct reader *);
  int  (*head_read)(struct reader *,unsigned long *newhead);
  int  (*head_shift)(struct reader *,unsigned long *head);
  off_t  (*skip_bytes)(struct reader *,off_t len);
  int  (*read_frame_body)(struct reader *,unsigned char *,int size);
  int  (*back_bytes)(struct reader *,off_t bytes);
  int  (*back_frame)(struct reader *,struct frame *,long num);
  off_t (*tell)(struct reader *);
  void (*rewind)(struct reader *);
  off_t filelen;
  off_t filepos;
  int  filept;
  int  flags;
  unsigned char id3buf[128];
};
#define READER_FD_OPENED 0x1
#define READER_ID3TAG    0x2
#define READER_SEEKABLE  0x4

#endif


#ifndef MPG123_ID3_H
#define MPG123_ID3_H

#include "stringbuf.h"

struct taginfo
{
	unsigned char version; /* 1, 2 */
	struct stringbuf title;
	struct stringbuf artist;
	struct stringbuf album;
	struct stringbuf year; /* be ready for 20570! */
	struct stringbuf comment;
	struct stringbuf genre;
};

/* really need it _here_! */
#include "frame.h"
#include "reader.h"

void init_id3(struct frame *fr);
void exit_id3(struct frame *fr);
void reset_id3(struct frame *fr);
void print_id3_tag(struct frame *fr, unsigned char *id3v1buf);
int parse_new_id3(struct frame *fr, unsigned long first4bytes, struct reader *rds);

#endif

/*
	readers.c: reading input data

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mpg123lib_intern.h"

static off_t get_fileinfo(struct frame *);

/* stream based operation  with icy meta data*/
static ssize_t icy_fullread(struct frame *fr, unsigned char *buf, ssize_t count)
{
	ssize_t ret,cnt;
	cnt = 0;

	/*
		We check against READER_ID3TAG instead of rds->filelen >= 0 because if we got the ID3 TAG we know we have the end of the file.
		If we don't have an ID3 TAG, then it is possible the file has grown since we started playing, so we want to keep reading from it if possible.
	*/
	if((fr->rdat.flags & READER_ID3TAG) && fr->rdat.filepos + count > fr->rdat.filelen) count = fr->rdat.filelen - fr->rdat.filepos;

	while(cnt < count)
	{
		/* all icy code is inside this if block, everything else is the plain fullread we know */
		/* debug1("read: %li left", (long) count-cnt); */
		if(fr->icy.interval && (fr->rdat.filepos+count > fr->icy.next))
		{
			unsigned char temp_buff;
			size_t meta_size;
			ssize_t cut_pos;

			/* we are near icy-metaint boundary, read up to the boundary */
			cut_pos = fr->icy.next - fr->rdat.filepos;
			ret = read(fr->rdat.filept,buf,cut_pos);
			if(ret < 0) return READER_ERROR;

			fr->rdat.filepos += ret;
			cnt += ret;

			/* now off to read icy data */

			/* one byte icy-meta size (must be multiplied by 16 to get icy-meta length) */
			ret = read(fr->rdat.filept,&temp_buff,1);
			if(ret < 0) return READER_ERROR;
			if(ret == 0) break;

			debug2("got meta-size byte: %u, at filepos %li", temp_buff, (long)fr->rdat.filepos );
			fr->rdat.filepos += ret; /* 1... */

			if((meta_size = ((size_t) temp_buff) * 16))
			{
				/* we have got some metadata */
				char *meta_buff;
				meta_buff = (char*) malloc(meta_size+1);
				if(meta_buff != NULL)
				{
					ret = read(fr->rdat.filept,meta_buff,meta_size);
					meta_buff[meta_size] = 0; /* string paranoia */
					if(ret < 0) return READER_ERROR;

					fr->rdat.filepos += ret;

					if(fr->icy.data) free(fr->icy.data);
					fr->icy.data = meta_buff;
					fr->icy.changed = 1;
					debug2("icy-meta: %s size: %d bytes", fr->icy.data, (int)meta_size);
				}
				else
				{
					error1("cannot allocate memory for meta_buff (%lu bytes) ... trying to skip the metadata!", (unsigned long)meta_size);
					fr->rd->skip_bytes(fr, meta_size);
				}
			}
			fr->icy.next = fr->rdat.filepos+fr->icy.interval;
		}

		ret = read(fr->rdat.filept,buf+cnt,count-cnt);
		if(ret < 0) return READER_ERROR;
		if(ret == 0) break;

		fr->rdat.filepos += ret;
		cnt += ret;
	}
	/* debug1("done reading, got %li", (long)cnt); */
	return cnt;
}

/* stream based operation */
static ssize_t plain_fullread(struct frame *fr,unsigned char *buf, ssize_t count)
{
	ssize_t ret,cnt=0;

	/*
		We check against READER_ID3TAG instead of rds->filelen >= 0 because if we got the ID3 TAG we know we have the end of the file.
		If we don't have an ID3 TAG, then it is possible the file has grown since we started playing, so we want to keep reading from it if possible.
	*/
	if((fr->rdat.flags & READER_ID3TAG) && fr->rdat.filepos + count > fr->rdat.filelen) count = fr->rdat.filelen - fr->rdat.filepos;
	while(cnt < count)
	{
		ret = read(fr->rdat.filept,buf+cnt,count-cnt);
		if(ret < 0) return READER_ERROR;
		if(ret == 0) break;
		fr->rdat.filepos += ret;
		cnt += ret;
	}
	return cnt;
}

static off_t stream_lseek(struct reader_data *rds, off_t pos, int whence)
{
	off_t ret;
	ret = lseek(rds->filept, pos, whence);
	if (ret >= 0)	rds->filepos = ret;
	else ret = READER_ERROR; /* not the original value */
	return ret;
}

static int default_init(struct frame *fr)
{
	fr->rdat.filelen = get_fileinfo(fr);
	fr->rdat.filepos = 0;
	if(fr->rdat.filelen >= 0)
	{
		fr->rdat.flags |= READER_SEEKABLE;
		if(!strncmp((char*)fr->rdat.id3buf,"TAG",3)) fr->rdat.flags |= READER_ID3TAG;
	}
	return 0;
}

void stream_close(struct frame *fr)
{
	if (fr->rdat.flags & READER_FD_OPENED) close(fr->rdat.filept);
}

/**************************************** 
 * HACK,HACK,HACK: step back <num> frames 
 * can only work if the 'stream' isn't a real stream but a file
 * returns 0 on success; 
 */
static int stream_back_bytes(struct frame *fr, off_t bytes)
{
	if(stream_lseek(&fr->rdat,-bytes,SEEK_CUR) < 0) return READER_ERROR;

	return 0;
}

/* this function strangely is defined to seek num frames _back_ (and is called with -offset - duh!) */
/* also... let that int be a long in future! */
static int stream_back_frame(struct frame *fr, long num)
{
	if(fr->rdat.flags & READER_SEEKABLE)
	{
		unsigned long newframe, preframe;
		if(num > 0) /* back! */
		{
			if(num > fr->num) newframe = 0;
			else newframe = fr->num-num;
		}
		else newframe = fr->num-num;

		/* two leading frames? hm, doesn't seem to be really needed... */
		/*if(newframe > 1) newframe -= 2;
		else newframe = 0;*/

		/* now seek to nearest leading index position and read from there until newframe is reached */
		if(stream_lseek(&fr->rdat,frame_index_find(fr, newframe, &preframe),SEEK_SET) < 0)
		return READER_ERROR;
		debug2("going to %lu; just got %lu", newframe, preframe);
		fr->num = preframe;
		while(fr->num < newframe)
		{
			/* try to be non-fatal now... frameNum only gets advanced on success anyway */
			if(!read_frame(fr)) break;
		}
		/* this is not needed at last? */
		/*read_frame(fr);
		read_frame(fr);*/

		if(fr->lay == 3) set_pointer(fr, 512);

		debug1("arrived at %lu", fr->num);

		return 0;
	}
	else return READER_ERROR; /* invalid, no seek happened */
}

/* return FALSE on error, TRUE on success, READER_MORE on occasion */
static int generic_head_read(struct frame *fr,unsigned long *newhead)
{
	unsigned char hbuf[4];
	int ret = fr->rd->fullread(fr,hbuf,4);
	if(ret == READER_MORE) return ret;
	if(ret != 4) return FALSE;

	*newhead = ((unsigned long) hbuf[0] << 24) |
	           ((unsigned long) hbuf[1] << 16) |
	           ((unsigned long) hbuf[2] << 8)  |
	            (unsigned long) hbuf[3];

	return TRUE;
}

/* return FALSE on error, TRUE on success, READER_MORE on occasion */
static int generic_head_shift(struct frame *fr,unsigned long *head)
{
	unsigned char hbuf;
	int ret = fr->rd->fullread(fr,&hbuf,1);
	if(ret == READER_MORE) return ret;
	if(ret != 1) return FALSE;

	*head <<= 8;
	*head |= hbuf;
	*head &= 0xffffffff;
	return TRUE;
}

/* returns reached position... negative ones are bad... */
static off_t stream_skip_bytes(struct frame *fr,off_t len)
{
	if((fr->rdat.flags & READER_SEEKABLE) && (fr->rdat.filelen >= 0))
	{
		off_t ret = stream_lseek(&fr->rdat, len, SEEK_CUR);

		return ret<0 ? READER_ERROR : ret;
	}
	else if(len >= 0)
	{
		unsigned char buf[1024]; /* ThOr: Compaq cxx complained and it makes sense to me... or should one do a cast? What for? */
		ssize_t ret;
		while (len > 0)
		{
			ssize_t num = len < (off_t)sizeof(buf) ? (ssize_t)len : (ssize_t)sizeof(buf);
			ret = fr->rd->fullread(fr, buf, num);
			if (ret < 0) return ret;
			len -= ret;
		}
		return fr->rdat.filepos;
	}
	else return READER_ERROR;
}

/* returns size on success... */
static int generic_read_frame_body(struct frame *fr,unsigned char *buf, int size)
{
	long l;

	if((l=fr->rd->fullread(fr,buf,size)) != size)
	{
		long ll = l;
		if(ll <= 0) ll = 0;

		memset(buf+ll,0,size-ll); /* why, actually? */
	}
	return l;
}

static off_t generic_tell(struct frame *fr){ return fr->rdat.filepos; }

static void stream_rewind(struct frame *fr)
{
	stream_lseek(&fr->rdat,0,SEEK_SET);
}

/*
 * returns length of a file (if filept points to a file)
 * reads the last 128 bytes information into buffer
 * ... that is not totally safe...
 */
static off_t get_fileinfo(struct frame *fr)
{
	off_t len;

	if((len=lseek(fr->rdat.filept,0,SEEK_END)) < 0)	return -1;

	if(lseek(fr->rdat.filept,-128,SEEK_END) < 0) return -1;

	if(fr->rd->fullread(fr,(unsigned char *)fr->rdat.id3buf,128) != 128)	return -1;

	if(!strncmp((char*)fr->rdat.id3buf,"TAG",3))	len -= 128;

	if(lseek(fr->rdat.filept,0,SEEK_SET) < 0)	return -1;

	if(len <= 0)	return -1;

	return len;
}

/* reader for input via manually provided buffers */

static int feed_init(struct frame *fr)
{
	fr->rdat.buf = NULL;
	fr->rdat.filelen = 0;
	fr->rdat.filepos = 0;
	fr->rdat.firstpos = 0;
	fr->rdat.flags |= READER_BUFFERED | READER_MICROSEEK;
	return 0;
}

static void feed_close(struct frame *fr)
{
	/* free the buffer chain */
	struct buffy *b = fr->rdat.buf;
	while(b != NULL)
	{
		struct buffy *n = b->next;
		free(b->data);
		free(b);
		b = n;
	}
	feed_init(fr);
}

/* externally called function, returns 0 on success, -1 on error */
int feed_more(struct frame *fr, unsigned char *in, long count)
{
	/* the pointer to the pointer for the buffy after the end... */
	struct buffy **b = &fr->rdat.buf;
	while(*b != NULL){ b = &(*b)->next; }
	*b = (struct buffy*)malloc(sizeof(struct buffy));
	if(*b == NULL) return -1;
	(*b)->data = (unsigned char*)malloc(count);
	if((*b)->data == NULL){ free(*b); *b = NULL; return -1; }
	memcpy((*b)->data, in, count);
	(*b)->size = count;
	(*b)->next = NULL; /* Hurray, the new last buffer! */
	fr->rdat.filelen += count;
	return 0;
}

static ssize_t feed_read(struct frame *fr, unsigned char *out, ssize_t count)
{
	struct buffy *b = fr->rdat.buf;
	ssize_t gotcount = 0;
	ssize_t offset = 0;
	if(fr->rdat.filelen - fr->rdat.filepos < count)
	{
		/* go back to firstpos, undo the previous reads */
		fr->rdat.filepos = fr->rdat.firstpos;
		return MPG123_NEED_MORE;
	}
	/* find the current buffer */
	while(b != NULL && (offset + b->size) <= fr->rdat.filepos)
	{
		offset += b->size;
		b = b->next;
	}
	/* now start copying from there */
	while(gotcount < count && (b != NULL))
	{
		ssize_t loff = fr->rdat.filepos - offset;
		ssize_t chunk = count - gotcount; /* amount of bytes to get from here... */
		if(chunk > b->size - loff) chunk = b->size - loff;
		memcpy(out+gotcount, b->data+loff, chunk);
		gotcount += chunk;
	}
	fr->rdat.filepos += gotcount;
	if(gotcount != count) return -1; /* That must be an error. */
	return gotcount;
}

/* returns reached position... negative ones are bad... */
static off_t feed_skip_bytes(struct frame *fr,off_t len)
{
	if(len >= 0)
	{
		if(fr->rdat.filelen - fr->rdat.filepos < len) return READER_MORE;
		else return fr->rdat.filepos += len;
	}
	else return READER_ERROR;
}

static int feed_back_bytes(struct frame *fr, off_t bytes)
{
	if(bytes >=0)
	{
		if(bytes <= fr->rdat.filepos) fr->rdat.filepos -= bytes;
		else return READER_ERROR;
	}
	else
	{
		off_t ret = feed_skip_bytes(fr, -bytes);
		if(ret > 0) ret = 0;
		return ret; /* could be 0, could be error code */
	}
	return 0;
}

static int feed_back_frame(struct frame *fr, long num){ return READER_ERROR; }

void feed_rewind(struct frame *fr)
{
	fr->rdat.filepos  = 0;
	fr->rdat.firstpos = 0;
}

void feed_forget(struct frame *fr)
{
	/* free all buffers that are def'n'tly outdated */
	struct buffy *b = fr->rdat.buf;
	/* we have buffers until filepos... delete all buffers fully below it */
	while(fr->rdat.filepos >= b->size)
	{
		struct buffy *n = b->next; /* != NULL or this is indeed the end and the last cycle anyway */
		fr->rdat.filepos -= b->size;
		fr->rdat.filelen -= b->size;
		free(b->data);
		free(b);
		b = n;
	}
	fr->rdat.buf = b;
	fr->rdat.firstpos = fr->rdat.filepos;
}

/*****************************************************************
 * read frame helper
 */

struct reader readers[] =
{
	{
		default_init,
		stream_close,
		plain_fullread,
		generic_head_read,
		generic_head_shift,
		stream_skip_bytes,
		generic_read_frame_body,
		stream_back_bytes,
		stream_back_frame,
		generic_tell,
		stream_rewind,
		NULL
	} ,
	{
		default_init,
		stream_close,
		icy_fullread,
		generic_head_read,
		generic_head_shift,
		stream_skip_bytes,
		generic_read_frame_body,
		stream_back_bytes,
		stream_back_frame,
		generic_tell,
		stream_rewind,
		NULL
	},
	{
		feed_init,
		feed_close,
		feed_read,
		generic_head_read,
		generic_head_shift,
		feed_skip_bytes,
		generic_read_frame_body,
		feed_back_bytes,
		feed_back_frame,
		generic_tell,
		feed_rewind,
		feed_forget
	}
/* buffer readers... can also be icy? nah, drop it... plain mpeg audio buffer reader */
#ifdef READ_SYSTEM
	,{
		system_init,
		NULL,	/* filled in by system_init() */
		fullread,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	} 
#endif
};

int open_feed(struct frame *fr)
{
	clear_icy(&fr->icy);
	fr->rd = &readers[READER_FEED];
	fr->rdat.flags = 0;
	if(fr->rd->init(fr) < 0) return -1;
	return 0;
}

int open_stream(struct frame *fr, char *bs_filenam, int fd)
{
	int filept_opened = 1;
	int filept; /* descriptor of opened file/stream */

	clear_icy(&fr->icy); /* can be done inside frame_clear ...? */
	if(!bs_filenam) /* no file to open, got a descriptor (stdin) */
	{
		filept = fd;
		filept_opened = 0; /* and don't try to close it... */
	}
	#ifndef O_BINARY
	#define O_BINARY (0)
	#endif
	else if((filept = open(bs_filenam, O_RDONLY|O_BINARY)) < 0) /* a plain old file to open... */
	{
		perror(bs_filenam);
		return filept; /* error... */
	}

	/* now we have something behind filept and can init the reader */
	fr->rdat.filelen = -1;
	fr->rdat.filept  = filept;
	fr->rdat.flags = 0;
	if(filept_opened)	fr->rdat.flags |= READER_FD_OPENED;

	if(fr->p.icy_interval > 0)
	{
		fr->icy.interval = fr->p.icy_interval;
		fr->icy.next = fr->icy.interval;
		fr->rd = &readers[READER_ICY_STREAM];
	}
	else fr->rd = &readers[READER_STREAM];

	if(fr->rd->init(fr) < 0) return -1;

	return filept;
}

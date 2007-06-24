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

#include "mpg123.h"
#include "buffer.h"
#include "common.h"

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
			if(ret < 0) return ret;

			fr->rdat.filepos += ret;
			cnt += ret;

			/* now off to read icy data */

			/* one byte icy-meta size (must be multiplied by 16 to get icy-meta length) */
			ret = read(fr->rdat.filept,&temp_buff,1);
			if(ret < 0) return ret;
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
					if(ret < 0) return ret;

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
		if(ret < 0) return ret;
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
		if(ret < 0) return ret;
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

	return ret;
}

static int default_init(struct frame *fr)
{
	fr->rdat.filelen = get_fileinfo(fr);
	fr->rdat.filepos = 0;
	if(fr->rdat.filelen >= 0)
	{
		fr->rdat.flags |= READER_SEEKABLE;
		if(!strncmp(fr->rdat.id3buf,"TAG",3)) fr->rdat.flags |= READER_ID3TAG;
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
	if(stream_lseek(&fr->rdat,-bytes,SEEK_CUR) < 0) return -1;

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
		return -1;
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
	else return -1; /* invalid, no seek happened */
}

static int stream_head_read(struct frame *fr,unsigned long *newhead)
{
	unsigned char hbuf[4];

	if(fr->rd->fullread(fr,hbuf,4) != 4) return FALSE;

	*newhead = ((unsigned long) hbuf[0] << 24) |
	           ((unsigned long) hbuf[1] << 16) |
	           ((unsigned long) hbuf[2] << 8)  |
	            (unsigned long) hbuf[3];

	return TRUE;
}

static int stream_head_shift(struct frame *fr,unsigned long *head)
{
	unsigned char hbuf;

	if(fr->rd->fullread(fr,&hbuf,1) != 1) return 0;

	*head <<= 8;
	*head |= hbuf;
	*head &= 0xffffffff;
	return 1;
}

/* returns reached position... negative ones are bad */
static off_t stream_skip_bytes(struct frame *fr,off_t len)
{
	if (fr->rdat.filelen >= 0)
	{
		off_t ret = stream_lseek(&fr->rdat, len, SEEK_CUR);

		return ret;
	}
	else if(len >= 0)
	{
		unsigned char buf[1024]; /* ThOr: Compaq cxx complained and it makes sense to me... or should one do a cast? What for? */
		off_t ret;
		while (len > 0)
		{
			off_t num = len < sizeof(buf) ? len : sizeof(buf);
			ret = fr->rd->fullread(fr, buf, num);
			if (ret < 0) return ret;
			len -= ret;
		}
		return fr->rdat.filepos;
	}
	else return -1;
}

static int stream_read_frame_body(struct frame *fr,unsigned char *buf, int size)
{
	long l;

	if( (l=fr->rd->fullread(fr,buf,size)) != size)
	{
		if(l <= 0) return 0;

		memset(buf+l,0,size-l);
	}

	return 1;
}

static off_t stream_tell(struct frame *fr)
{
	return fr->rdat.filepos;
}

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

	if(!strncmp(fr->rdat.id3buf,"TAG",3))	len -= 128;

	if(lseek(fr->rdat.filept,0,SEEK_SET) < 0)	return -1;

	if(len <= 0)	return -1;

	return len;
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
		stream_head_read,
		stream_head_shift,
		stream_skip_bytes,
		stream_read_frame_body,
		stream_back_bytes,
		stream_back_frame,
		stream_tell,
		stream_rewind
	} ,
	{
		default_init,
		stream_close,
		icy_fullread,
		stream_head_read,
		stream_head_shift,
		stream_skip_bytes,
		stream_read_frame_body,
		stream_back_bytes,
		stream_back_frame,
		stream_tell,
		stream_rewind
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
		NULL
	} 
#endif
};


/* open the device to read the bit stream from it */
int open_stream(struct frame *fr, char *bs_filenam, int fd)
{
	int i;
	int filept_opened = 1;
	int filept; /* descriptor of opened file/stream */

	clear_icy(&fr->icy); /* can be done inside frame_clear ...? */
	if(!bs_filenam) /* no file to open, got a descriptor (stdin) */
	{
		if(fd < 0) /* special: read from stdin */
		{
			filept = 0;
			filept_opened = 0; /* and don't try to close it... */
		}
		else filept = fd;
	}
	else if (!strncmp(bs_filenam, "http://", 7)) /* http stream */
	{
		char* mime = NULL;
		filept = http_open(fr, bs_filenam, &mime);
		/* now check if we got sth. and if we got sth. good */
		if((filept >= 0) && (mime != NULL) && strcmp(mime, "audio/mpeg") && strcmp(mime, "audio/x-mpeg"))
		{
			fprintf(stderr, "Error: unknown mpeg MIME type %s - is it perhaps a playlist (use -@)?\nError: If you know the stream is mpeg1/2 audio, then please report this as "PACKAGE_NAME" bug\n", mime == NULL ? "<nil>" : mime);
			filept = -1;
		}
		if(mime != NULL) free(mime);
		if(filept < 0) return filept; /* error... */
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

	if(fr->icy.interval)
	{
		fr->icy.next = fr->icy.interval;
		fr->rd = &readers[READER_ICY_STREAM];
	}
	else fr->rd = &readers[READER_STREAM];

	if(fr->rd->init(fr) < 0) return -1;

	return filept;
}

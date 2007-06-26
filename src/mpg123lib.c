#include "mpg123lib_intern.h"

static int initialized = 0;

void mpg123_init(void)
{
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3();
#ifndef OPT_MMX_ONLY
	prepare_decode_tables();
#endif
	initialized = 1;
}

void mpg123_exit(void)
{
	/* nothing yet, but something later perhaps */
}

mpg123_handle *mpg123_new()
{
	mpg123_handle *fr = NULL;
	if(initialized) fr = (mpg123_handle*) malloc(sizeof(mpg123_handle));
	else error("You didn't initialize the library!");

	if(fr != NULL)
	{
		frame_init(fr);
#ifdef OPT_MULTI
		frame_cpu_opt(fr);
#endif
		if((frame_outbuffer(fr) != 0) || (frame_buffers(fr) != 0))
		{
			error("Unable to initialize frame buffers!");
			frame_exit(fr);
			fr = NULL;
		}
		else
		{
			opt_make_decode_tables(&fr);
			init_layer3_stuff(fr);
			init_layer2_stuff(fr)
		}
	}
	else error("Unable to create a handle!");

	return fr;
}

int mpg123_open_feed(mpg123_handle *mh)
{
	mpg123_close(mh);
	frame_reset(mh);
	return open_feed(mh);
}

/* Intended usage:
	while(1) {
		len = read(0,buf,16384);
		if(len <= 0)
			break;
		ret = decodeMP3(&mp,buf,len,out,8192,&size);
		while(ret == MP3_OK) {
			write(1,out,size);
			ret = decodeMP3(&mp,NULL,0,out,8192,&size);
		}
	}
	
	So this function reads from buf up to len bytes and decodes what it can, up to a limit, stores decoded bytes.
	I need a buffer fake reader for that. Then get read_frame from common.c straight.
	Also, I need to set up fixed otuput mode 16bit Stereo... hm... forced? What did old mpglib do?
	For the full decoding pleasure we need interface to mpg123's param struct and integration of file readers.
	Most often, you'll have a path/URL or a file descriptor to share.
*/

int mpg123_decode(mpg123_handle *mh,unsigned char *inmemory,int inmemsize, unsigned char *outmemory,int outmemsize,int *done)
{
	/* NOT WORKING YET! need more code from play_frame; set_synth_functions and stuff */
	if(inmemsize > 0)
	if(feed_more(mh, inmemory, inmemsize) == -1) return -1;
	while(*done < outmemsize)
	{
		if(mh->buffer.fill)
		{
			/* get what is needed - or just what is there */
			int a = mh->buffer.fill > outmemsize ? outmemsize : mh->buffer.fill;
			memcpy(outmemory, mh->buffer.data, a);
			/* less data in frame buffer, less needed, output pointer increase, more data given... */
			mh->buffer.fill -= a;
			outmemsize -= a;
			outmemory  += a;
			*done += a;
			/* move rest of frame buffer to beginning */
			if(mh->buffer.fill) memmove(mh->buffer.data, mh->buffer.data + a, mh->buffer.fill);
		}
		else
		{
			int b = read_frame(mh);
			if(b == READER_ERROR) return -1;
			if(b == MPG123_NEED_MORE) return b; /* need another call with data */
			/* Now, there should be new data to copy on next run */
		}
	}
	return 0;
}

int mpg123_output(mpg123_handle *mh, int  format, int  channels, long rate)
{
	/* check format, too... */
	if(channels > 2) return -1;
	if(rate > 96000) return -1;
	frame_outformat(mh, format, channels, rate);
	return 0;
}

int mpg123_get_output(mpg123_handle *mh, int *format, int *channels, long *rate)
{
	*format   = mh->af.format;
	*channels = mh->af.channels;
	*rate     = mh->af.rate;
	return 0;
}

void mpg123_close(mpg123_handle *mh)
{
	if(mh->rd != NULL && mh->rd->close != NULL) mh->rd->close(mh);
	mh->rd = NULL;
}

void mpg123_delete(mpg123_handle *mh)
{
	if(mh != NULL)
	{
		mpg123_close(mh);
		frame_exit(mh); /* free buffers in frame */
		free(mh); /* free struct; cast? */
	}
}

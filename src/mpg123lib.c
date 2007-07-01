#include "mpg123lib_intern.h"
#include "getcpuflags.h"

static int initialized = 0;

void mpg123_init(void)
{
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3();
#ifndef OPT_MMX_ONLY
	prepare_decode_tables();
#endif
	getcpuflags(&cpu_flags);
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
		debug("cpu opt setting");
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
			opt_make_decode_tables(fr);
			/* happening on frame change instead:
			init_layer3_stuff(fr);
			init_layer2_stuff(fr); */
		}
	}
	else error("Unable to create a handle!");

	return fr;
}

int mpg123_param(mpg123_handle *mh, int key, long val)
{
	int ret = 0;
	switch(key)
	{
		case MPG123_FORCE_RATE:
			if(val <= 0 || val > 96000){ mh->err = MPG123_ERR_RATE; ret = -1; }
			mh->p.force_rate = val;
		break;
		case MPG123_VERBOSE:
			mh->p.verbose = val;
		break;
		case MPG123_ADD_FLAGS:
			mh->p.flags |= val;
		break;
		case MPG123_START_FRAME:
			mh->p.start_frame = val > 0 ? val : 0;
		break;
		default:
			mh->err = MPG123_BAD_PARAM;
			ret = -1;
	}
	return ret;
}

int mpg123_down_sample(mpg123_handle *mh, int down)
{
	if(down < 0 || down > 2)
	{
		mh->err = MPG123_ERR_RATE;
		return -1;
	}
	mh->p.down_sample = down;
	return 0;
}

int mpg123_open_feed(mpg123_handle *mh)
{
	mpg123_close(mh);
	frame_reset(mh);
	return open_feed(mh);
}

int decode_update(mpg123_handle *mh)
{
	long native_rate = frame_freq(mh);
	debug("updating decoder structure");
#ifdef GAPLESS
	if(mh->p.flags & MPG123_GAPLESS) /* The gapless info must be there after we got the first real frame. */
	{
		frame_gapless_bytify(mh);
		frame_gapless_position(mh);
	}
#endif
	if(mh->af.rate == native_rate) mh->down_sample = 0;
	else if(mh->af.rate == native_rate>>1) mh->down_sample = 1;
	else if(mh->af.rate == native_rate>>2) mh->down_sample = 2;
	else mh->down_sample = 3; /* flexible (fixed) rate */
	switch(mh->down_sample)
	{
		case 0:
		case 1:
		case 2:
			mh->down_sample_sblimit = SBLIMIT>>(mh->down_sample);
		break;
		case 3:
		{
			if(synth_ntom_set_step(mh) != 0) return -1;
			if(frame_freq(mh) > mh->af.rate)
			{
				mh->down_sample_sblimit = SBLIMIT * mh->af.rate;
				mh->down_sample_sblimit /= frame_freq(mh);
			}
			else mh->down_sample_sblimit = SBLIMIT;
		}
		break;
	}

	if(!(mh->p.flags & MPG123_FORCE_MONO))
	{
		if(mh->af.channels == 1) mh->single = SINGLE_MIX;
		else mh->single = SINGLE_STEREO;
	}
	else mh->single = (mh->p.flags & MPG123_FORCE_MONO)-1;
	if(set_synth_functions(mh) != 0) return -1;;
	init_layer3_stuff(mh);
	init_layer2_stuff(mh);

	return 0;
}

/*
	The old picture:
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
*/

int mpg123_decode(mpg123_handle *mh,unsigned char *inmemory,int inmemsize, unsigned char *outmemory,int outmemsize,int *done)
{
	int ret = 0;
	static long outbytes = 0; /* for debugging only! */
	*done = 0;
	if(inmemsize > 0)
	if(feed_more(mh, inmemory, inmemsize) == -1) return -1;
	while(*done < outmemsize && ret == 0)
	{
		debug1("decode loop, fill %i", mh->buffer.fill);
#ifdef GAPLESS
		/* That skips unwanted samples and advances byte position. */
		if(mh->p.flags & MPG123_GAPLESS) frame_gapless_buffercheck(mh);
#endif
		if(mh->buffer.fill)
		{
			/* get what is needed - or just what is there */
			int a = mh->buffer.fill > (outmemsize - *done) ? outmemsize - *done : mh->buffer.fill;
			debug4("buffer fill: %i; copying %i (%i - %i)", mh->buffer.fill, a, outmemsize, *done);
			memcpy(outmemory, mh->buffer.data, a);
			/* less data in frame buffer, less needed, output pointer increase, more data given... */
			mh->buffer.fill -= a;
			outmemory  += a;
			*done += a;
			outbytes += a;
			debug1("outbytes=%li", outbytes);
			/* move rest of frame buffer to beginning */
			if(mh->buffer.fill) memmove(mh->buffer.data, mh->buffer.data + a, mh->buffer.fill);
		}
		else
		{
			/* read a frame... */
			int b;
			debug("read frame");
			b = read_frame(mh);
			debug1("read frame returned %i", b);
			if(b == READER_ERROR) return MPG123_ERR;
			if(b == MPG123_NEED_MORE) return b; /* need another call with data */
			/* Now, there should be new data to decode ... and also possibly new stream properties */
			if(mh->header_change > 1)
			{
				debug("big header change");
				/* The buffer is already empty - so there is no data left in old format in case that changes.
				   Also, the position is accurately at the frame boundary. */
				b = frame_output_format(mh);
				if(b < 0) return b; /* not nice to fail here... perhaps once should add possibility to repeat this step */
				if(decode_update(mh) < 0) return MPG123_ERR; /* dito... */
				if(b == 1) ret = MPG123_NEW_FORMAT;
			}
			/* finally, the decoding */
			if(mh->num < mh->p.start_frame || (mh->p.frame_number >= 0 && mh->num-mh->p.start_frame > mh->num)) continue;
			mh->clip += (mh->do_layer)(mh);
			debug2("decoded frame %li, got %li samples in buffer", mh->num, mh->buffer.fill / (samples_to_bytes(mh, 1)));
		}
	}
	return MPG123_OK;
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

static char *mpg123_error[] =
{
	"No error... (code 0)",
	"Unable to set up output format! (code 1)",
	"Invalid channel number specified. (code 2)",
	"Invalid sample rate specified. (code 3)",
	"Unable to allocate memory for 16 to 8 converter table! (code 4)",
	"Bad parameter id! (code 5)"
};

char* mpg123_strerror(mpg123_handle *mh)
{
	if(mh->err >= 0 && mh->err < sizeof(mpg123_error)/sizeof(char*))
	return mpg123_error[mh->err];
	else return "I have no idea - an unknown error code!";
}

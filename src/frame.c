#include "mpg123.h"
#include "common.h"

#include <stdlib.h>

void frame_invalid(struct frame *fr)
{
	fr->buffer.data = NULL;
	/* unnecessary: fr->buffer.size = fr->buffer.fill = 0; */
}

int frame_buffer(struct frame *fr, int s)
{
	if(fr->buffer.data != NULL && fr->buffer.size != s)
	{
		free(fr->buffer.data);
		fr->buffer.data = NULL;
	}
	fr->buffer.size = s;
	if(fr->buffer.data == NULL) fr->buffer.data = (unsigned char*) malloc(fr->buffer.size);
	if(fr->buffer.data == NULL) return -1;
	fr->buffer.fill = 0;
	return 0;
}

/* Prepare the handle for a new track.
   That includes (re)allocation or reuse of the output buffer */
int frame_init(struct frame* fr)
{
	fr->buffer.fill = 0;
	fr->num = 0;
	fr->oldhead = 0;
	fr->firsthead = 0;
	fr->vbr = CBR;
	fr->abr_rate = 0;
	fr->track_frames = 0;
	fr->mean_frames = 0;
	fr->mean_framesize = 0;
	fr->rva.lastscale = -1;
	fr->rva.level[0] = -1;
	fr->rva.level[1] = -1;
	fr->rva.gain[0] = 0;
	fr->rva.gain[1] = 0;
	fr->rva.peak[0] = 0;
	fr->rva.peak[1] = 0;
	fr->index.fill = 0;
	fr->index.step = 1;
	/* perhaps make the bsspace all-zero here */
	fr->bsbuf = fr->bsspace[1];
	fr->bsnum = 0;
	fr->fsizeold = 0;
	fr->do_recover = 0;
	#ifdef GAPLESS
	/* one can at least skip the delay at beginning - though not add it at end since end is unknown */
	if(param.gapless) frame_gapless_init(fr, DECODER_DELAY+GAP_SHIFT, 0);
	#endif
	return 0;
}

void frame_clear(struct frame *fr)
{
	if(fr->buffer.data != NULL) free(fr->buffer.data);
	fr->buffer.data = NULL;
}

void print_frame_index(struct frame *fr, FILE* out)
{
	size_t c;
	for(c=0; c < fr->index.fill;++c) fprintf(out, "[%lu] %lu: %li (+%li)\n", (unsigned long) c, (unsigned long) c*fr->index.step, (long)fr->index.data[c], (long) (c ? fr->index.data[c]-fr->index.data[c-1] : 0));
}

/*
	find the best frame in index just before the wanted one, seek to there
	then step to just before wanted one with read_frame
	do not care tabout the stuff that was in buffer but not played back
	everything that left the decoder is counted as played
	
	Decide if you want low latency reaction and accurate timing info or stable long-time playback with buffer!
*/

off_t frame_index_find(struct frame *fr, unsigned long want_frame, unsigned long* get_frame)
{
	/* default is file start if no index position */
	off_t gopos = 0;
	*get_frame = 0;
	if(fr->index.fill)
	{
		/* find in index */
		size_t fi;
		/* at index fi there is frame step*fi... */
		fi = want_frame/fr->index.step;
		if(fi >= fr->index.fill) fi = fr->index.fill - 1;
		*get_frame = fi*fr->index.step;
		gopos = fr->index.data[fi];
	}
	return gopos;
}

#ifdef GAPLESS

/* take into account: channels, bytes per sample, resampling (integer samples!) */
unsigned long samples_to_bytes(struct frame *fr , unsigned long s, struct audio_info_struct* ai)
{
	/* rounding positive number... */
	double sammy, samf;
	sammy = (1.0*s) * (1.0*ai->rate)/freqs[fr->sampling_frequency];
	debug4("%lu samples to bytes with freq %li (ai.rate %li); sammy %f", s, freqs[fr->sampling_frequency], ai->rate, sammy);
	samf = floor(sammy);
	return (unsigned long)
		(((ai->format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16) ? 2 : 1)
#ifdef FLOATOUT
		* 2
#endif
		* ai->channels
		* (int) (((sammy - samf) < 0.5) ? samf : ( sammy-samf > 0.5 ? samf+1 : ((unsigned long) samf % 2 == 0 ? samf : samf + 1)));
}

/* input in bytes already */
void frame_gapless_init(struct frame *fr, unsigned long b, unsigned long e)
{
	fr->bytified = 0;
	fr->position = 0;
	fr->ignore = 0;
	fr->begin = b;
	fr->end = e;
	debug2("layer3_gapless_init: from %lu to %lu samples", fr->begin, fr->end);
}

void frame_gapless_position(struct frame* fr, unsigned long frames, struct audio_info_struct *ai)
{
	fr->position = samples_to_bytes(fr, frames*spf(fr), ai);
	debug1("set; position now %lu", fr->position);
}

void frame_gapless_bytify(struct frame *fr, struct audio_info_struct *ai)
{
	if(!fr->bytified)
	{
		fr->begin = samples_to_bytes(fr, fr->begin, ai);
		fr->end = samples_to_bytes(fr, fr->end, ai);
		fr->bytified = 1;
		debug2("bytified: begin=%lu; end=%5lu", fr->begin, fr->end);
	}
}

/* I need initialized fr here! */
void frame_gapless_ignore(struct frame *fr, unsigned long frames, struct audio_info_struct *ai)
{
	fr->ignore = samples_to_bytes(fr, frames*spf(fr), ai);
}

/*
	take the (partially or fully) filled and remove stuff for gapless mode if needed
	fr->buffer.fill may then be smaller than before...
*/
void frame_gapless_buffercheck(struct frame *fr)
{
	/* fr->buffer.fill bytes added since last position... */
	unsigned long new_pos = fr->position + fr->buffer.fill;
	if(fr->begin && (fr->position < fr->begin))
	{
		debug4("new_pos %lu (old: %lu), begin %lu, fr->buffer.fill %i", new_pos, fr->position, fr->begin, fr->buffer.fill);
		if(new_pos < fr->begin)
		{
			if(fr->ignore > fr->buffer.fill) fr->ignore -= fr->buffer.fill;
			else fr->ignore = 0;
			fr->buffer.fill = 0; /* full of padding/delay */
		}
		else
		{
			unsigned long ignored = fr->begin - fr->position;
			/* we need to shift the memory to the left... */
			debug3("old fr->buffer.fill: %i, begin %lu; good bytes: %i", fr->buffer.fill, fr->begin, (int)(new_pos - fr->begin));
			if(fr->ignore > ignored) fr->ignore -= ignored;
			else fr->ignore = 0;
			fr->buffer.fill -= ignored;
			debug3("shifting %i bytes from %p to %p", fr->buffer.fill, fr->buffer.data+(int)(fr->begin - fr->position), fr->buffer.data);
			memmove(fr->buffer.data, fr->buffer.data+(int)(fr->begin - fr->position), fr->buffer.fill);
		}
	}
	/* I don't cover the case with both end and begin in chunk! */
	else if(fr->end && (new_pos > fr->end))
	{
		fr->ignore = 0;
		/* either end in current chunk or chunk totally out */
		debug2("ending at position %lu / point %i", new_pos, fr->buffer.fill);
		if(fr->position < fr->end)	fr->buffer.fill -= new_pos - fr->end;
		else fr->buffer.fill = 0;
		debug1("set fr->buffer.fill to %i", fr->buffer.fill);
	}
	else if(fr->ignore)
	{
		if(fr->buffer.fill < fr->ignore)
		{
			fr->ignore -= fr->buffer.fill;
			debug2("ignored %i bytes; fr->buffer.fill = 0; %lu bytes left", fr->buffer.fill, fr->ignore);
			fr->buffer.fill = 0;
		}
		else
		{
			/* we need to shift the memory to the left... */
			debug3("old fr->buffer.fill: %i, to ignore: %lu; good bytes: %i", fr->buffer.fill, fr->ignore, fr->buffer.fill-(int)fr->ignore);
			fr->buffer.fill -= fr->ignore;
			debug3("shifting %i bytes from %p to %p", fr->buffer.fill, fr->buffer.data+fr->ignore, fr->buffer.data);
			memmove(fr->buffer.data, fr->buffer.data+fr->ignore, fr->buffer.fill);
			fr->ignore = 0;
		}
	}
	fr->position = new_pos;
}
#endif

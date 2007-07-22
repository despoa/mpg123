/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include <stdlib.h>
#include "mpg123.h"

void audio_info_struct_init(struct audio_info_struct *ai)
{
  ai->fn = -1;
  ai->rate = -1;
  ai->gain = -1;
  ai->output = -1;
  ai->handle = NULL;
  ai->device = NULL;
  ai->channels = -1;
  ai->format = -1;
}


void audio_info_struct_dump(struct audio_info_struct *ai)
{
	fprintf(stderr, "ai->fn=%d\n", ai->fn);
	fprintf(stderr, "ai->handle=%p\n", ai->handle);
	fprintf(stderr, "ai->rate=%ld\n", ai->rate);
	fprintf(stderr, "ai->gain=%ld\n", ai->gain);
	fprintf(stderr, "ai->output=%d\n", ai->output);
	fprintf(stderr, "ai->device='%s'\n", ai->device);
	fprintf(stderr, "ai->channels=%d\n", ai->channels);
	fprintf(stderr, "ai->format=%d\n", ai->format);
}


#define NUM_CHANNELS 2
#define NUM_ENCODINGS 6
#define NUM_RATES 10

/* Safer as function... */
const char* audio_encoding_name(const int encoding, const int longer)
{
	const char *name = longer ? "unknown" : "???";
	switch(encoding)
	{
		case MPG123_ENC_SIGNED_16:   name = longer ? "signed 16 bit"   : "s16";  break;
		case MPG123_ENC_UNSIGNED_16: name = longer ? "unsigned 16 bit" : "u16";  break;
		case MPG123_ENC_UNSIGNED_8:  name = longer ? "unsigned 8 bit"  : "u8";   break;
		case MPG123_ENC_SIGNED_8:    name = longer ? "signed 8 bit"    : "s8";   break;
		case MPG123_ENC_ULAW_8:      name = longer ? "mu-law (8 bit)"  : "ulaw"; break;
		case MPG123_ENC_ALAW_8:      name = longer ? "a-law (8 bit)"   : "alaw"; break;
	}
	return name;
}

static void capline(mpg123_handle *mh, int ratei)
{
	int enci;
	fprintf(stderr," %5ld  |", ratei >= 0 ? mpg123_rates[ratei] : param.force_rate);
	for(enci=0; enci<MPG123_ENCODINGS; ++enci)
	{
		switch(mpg123_format_support(mh, ratei, enci))
		{
			case MPG123_MONO:               fprintf(stderr, "   M   |"); break;
			case MPG123_STEREO:             fprintf(stderr, "   S   |"); break;
			case MPG123_MONO|MPG123_STEREO: fprintf(stderr, "  M/S  |"); break;
			default:                        fprintf(stderr, "       |");
		}
	}
	fprintf(stderr, "\n");
}

void print_capabilities(struct audio_info_struct *ai, mpg123_handle *mh)
{
	int r,e;
	fprintf(stderr,"\nAudio device: %s\nAudio capabilities:\n(matrix of [S]tereo or [M]ono support for sample format and rate in Hz)\n        |", ai->device != NULL ? ai->device : "<none>");
	for(e=0;e<MPG123_ENCODINGS;e++) fprintf(stderr," %5s |",audio_encoding_name(mpg123_encodings[e], 0));

	fprintf(stderr,"\n --------------------------------------------------------\n");
	for(r=0; r<MPG123_RATES; ++r) capline(mh, r);

	if(param.force_rate) capline(mh, -1);

	fprintf(stderr,"\n");
}


void audio_capabilities(struct audio_info_struct *ai, mpg123_handle *mh)
{
	int fmts;
	int ri;
	struct audio_info_struct ai1 = *ai; /* a copy */

	if(mpg123_param(mh, MPG123_FORCE_RATE, param.force_rate, 0) != MPG123_OK)
	{
		error1("Cannot set forced rate (%s)!", mpg123_strerror(mh));
		mpg123_format_none(mh);
		return;
	}
	if(param.outmode != DECODE_AUDIO)
	{ /* File/stdout writers can take anything. */
		mpg123_format_all(mh);
		return;
	}

	mpg123_format_none(mh); /* Start with nothing. */

	/* If audio_open fails, the device is just not capable of anything... */
	if(audio_open(&ai1) < 0) perror("audio");
	else
	{
		for(ai1.channels=1; ai1.channels<=2; ai1.channels++)
		for(ri=-1;ri<MPG123_RATES;ri++)
		{
			ai1.rate = ri >= 0 ? mpg123_rates[ri] : param.force_rate;
			fmts = audio_get_formats(&ai1);
			if(fmts < 0) continue;
			else mpg123_format(mh, ri, ai1.channels, fmts);
		}
		audio_close(&ai1);
	}

	if(param.verbose > 1) print_capabilities(ai, mh);
}

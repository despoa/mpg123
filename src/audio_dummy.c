/*
	audio_dummy.c: dummy audio output

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include "config.h"
#include "mpg123.h"

static int
open_dummy(audio_output_t *ao)
{
  fprintf(stderr,"No audio device support compiled into this binary (use -s).\n");
  return -1;
}

static int
get_formats_dummy(audio_output_t *ao)
{
  return AUDIO_FORMAT_SIGNED_16;
}

static int
write_dummy(audio_output_t *ao,unsigned char *buf,int len)
{
  return len;
}

static void
flush_dummy(audio_output_t *ao)
{
}

static int
close_dummy(audio_output_t *ao)
{
  return 0;
}


audio_output_t*
init_audio_output(void)
{
	audio_output_t* ao = alloc_audio_output();
	
	/* Set callbacks */
	ao->open = open_dummy;
	ao->flush = flush_dummy;
	ao->write = write_dummy;
	ao->get_formats = get_formats_dummy;
	ao->close = close_dummy;
	
	
	return ao;
}

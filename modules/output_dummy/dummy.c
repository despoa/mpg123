/*
	audio_dummy.c: dummy audio output

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include "config.h"
#include "mpg123.h"
#include "module.h"
#include "debug.h"

static int
open_dummy(audio_output_t *ao)
{
	debug("open_dummy()");
	warning("Using the dummy audio output module.");
	return 0;
}

static int
get_formats_dummy(audio_output_t *ao)
{
	debug("get_formats_dummy()");
	return AUDIO_FORMAT_SIGNED_16;
}

static int
write_dummy(audio_output_t *ao,unsigned char *buf,int len)
{
	debug("write_dummy()");
	return len;
}

static void
flush_dummy(audio_output_t *ao)
{
	debug("flush_dummy()");
}

static int
close_dummy(audio_output_t *ao)
{
	debug("close_dummy()");
	return 0;
}


static audio_output_t*
init_dummy(void)
{
	audio_output_t* ao = alloc_audio_output();
	
	debug("init_audio_output()");
	
	/* Set callbacks */
	ao->open = open_dummy;
	ao->flush = flush_dummy;
	ao->write = write_dummy;
	ao->get_formats = get_formats_dummy;
	ao->close = close_dummy;
	
	
	return ao;
}



/* 
	Module inforamtion data structure
*/
mpg123_module_t mpg123_module_info = {
	MPG123_MODULE_API_VERSION,
	"dummy",						/* name */
	"This is a dummy module",		/* description */
	"Nicholas J Humfrey",			/* author */
	"$Id: $",						/* version */
	
	init_dummy,						/* init_output */
};




/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ltdl.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "config.h"
#include "mpg123.h"
#include "debug.h"
#include "module.h"
#include "config.h"



/* Open an audio output module */
audio_output_t*
open_output_module( const char* name )
{
	mpg123_module_t *module = NULL;
	audio_output_t *ao = NULL;


	/* Open the module */
	module = open_module( name );
	if (module == NULL) return NULL;

	/* Check module supports output */
	if (module->init_output == NULL) {
		error1("Module '%s' does not support audio output.", name);
		close_module( module );
		return NULL;
	}
	
	/* Call the init function */
	ao = module->init_output();
	if (ao==NULL) {
		error( "Module's init function failed." );
		close_module( module );
		return NULL;
	}
	

	return ao;
}




/* Usually called by the module to allocate and initialise memory */
audio_output_t*
alloc_audio_output()
{
	audio_output_t* ao = malloc( sizeof( audio_output_t ) );
	if (ao==NULL) error( "Failed to allocate memory for audio_output_t." );

	/* Initialise variables */
	ao->fn = -1;
	ao->rate = -1;
	ao->gain = -1;
	ao->userptr = NULL;
	ao->device = NULL;
	/*ao->handle = NULL;*/
	ao->channels = -1;
	ao->format = -1;
	
	/* Set the callbacks to NULL */
	ao->open = NULL;
	ao->get_formats = NULL;
	ao->write = NULL;
	ao->flush = NULL;
	ao->close = NULL;
	ao->deinit = NULL;
	
	return ao;
}

void
deinit_audio_output( audio_output_t* ao ) 
{
	if (!ao) return;

	/* Close the audio output */
	if (ao->close) ao->close( ao );

	/* FIXME: Unload the module */
	

	/* Free up memory */
	free( ao );
}


void audio_output_dump(audio_output_t *ao)
{
	fprintf(stderr, "ao->fn=%d\n", ao->fn);
	fprintf(stderr, "ao->userptr=%p\n", ao->userptr);
	fprintf(stderr, "ao->rate=%ld\n", ao->rate);
	fprintf(stderr, "ao->gain=%ld\n", ao->gain);
	fprintf(stderr, "ao->device='%s'\n", ao->device);
	fprintf(stderr, "ao->channels=%d\n", ao->channels);
	fprintf(stderr, "ao->format=%d\n", ao->format);
}


#define NUM_CHANNELS 2
#define NUM_ENCODINGS 6
#define NUM_RATES 10

struct audio_format_name audio_val2name[NUM_ENCODINGS+1] = {
	{ AUDIO_FORMAT_SIGNED_16  , "signed 16 bit" , "s16 " } ,
	{ AUDIO_FORMAT_UNSIGNED_16, "unsigned 16 bit" , "u16 " } ,  
	{ AUDIO_FORMAT_UNSIGNED_8 , "unsigned 8 bit" , "u8  " } ,
	{ AUDIO_FORMAT_SIGNED_8   , "signed 8 bit" , "s8  " } ,
	{ AUDIO_FORMAT_ULAW_8     , "mu-law (8 bit)" , "ulaw " } ,
	{ AUDIO_FORMAT_ALAW_8     , "a-law (8 bit)" , "alaw " } ,
	{ -1 , NULL }
};


static int channels[NUM_CHANNELS] = { 1 , 2 };
static int rates[NUM_RATES] = { 
	8000, 11025, 12000, 
	16000, 22050, 24000,
	32000, 44100, 48000,
	8000	/* 8000 = dummy for user forced */
};

static int encodings[NUM_ENCODINGS] = {
	AUDIO_FORMAT_SIGNED_16, 
	AUDIO_FORMAT_UNSIGNED_16,
	AUDIO_FORMAT_UNSIGNED_8,
	AUDIO_FORMAT_SIGNED_8,
	AUDIO_FORMAT_ULAW_8,
	AUDIO_FORMAT_ALAW_8
};

static char capabilities[NUM_CHANNELS][NUM_ENCODINGS][NUM_RATES];

void audio_capabilities(audio_output_t *ao)
{
	int fmts;
	int i,j,k,k1=NUM_RATES-1;
	audio_output_t ao1 = *ao;

	if (param.outmode != DECODE_AUDIO) {
		memset(capabilities,1,sizeof(capabilities));
		return;
	}

	memset(capabilities,0,sizeof(capabilities));
	if(param.force_rate) {
		rates[NUM_RATES-1] = param.force_rate;
		k1 = NUM_RATES;
	}

	/* if audio_open faols, the device is just not capable of anything... */
	if(ao1.open(&ao1) < 0) {
		error("failed to open audio device");
	}
	else
	{
		for(i=0;i<NUM_CHANNELS;i++) {
			for(j=0;j<NUM_RATES;j++) {
				ao1.channels = channels[i];
				ao1.rate = rates[j];
				fmts = ao1.get_formats(&ao1);
				if(fmts < 0)
					continue;
				for(k=0;k<NUM_ENCODINGS;k++) {
					if((fmts & encodings[k]) == encodings[k])
						capabilities[i][k][j] = 1;
				}
			}
		}
		ao1.close(&ao1);
	}

	if(param.verbose > 1) {
		fprintf(stderr,"\nAudio capabilities:\n        |");
		for(j=0;j<NUM_ENCODINGS;j++) {
			fprintf(stderr," %5s |",audio_val2name[j].sname);
		}
		fprintf(stderr,"\n --------------------------------------------------------\n");
		for(k=0;k<k1;k++) {
			fprintf(stderr," %5d  |",rates[k]);
			for(j=0;j<NUM_ENCODINGS;j++) {
				if(capabilities[0][j][k]) {
					if(capabilities[1][j][k])
						fprintf(stderr,"  M/S  |");
					else
						fprintf(stderr,"   M   |");
				}
				else if(capabilities[1][j][k])
					fprintf(stderr,"   S   |");
				else
					fprintf(stderr,"       |");
			}
			fprintf(stderr,"\n");
		}
		fprintf(stderr,"\n");
	}
}

static int rate2num(int r)
{
	int i;
	for(i=0;i<NUM_RATES;i++) 
		if(rates[i] == r)
			return i;
	return -1;
}


static int audio_fit_cap_helper(audio_output_t *ao,int rn,int f0,int f2,int c)
{
	int i;
	
	if(rn >= 0) {
		for(i=f0;i<f2;i++) {
			if(capabilities[c][i][rn]) {
				ao->rate = rates[rn];
				ao->format = encodings[i];
				ao->channels = channels[c];
				return 1;
			}
		}
	}
	return 0;
	
}

/*
 * c=num of channels of stream
 * r=rate of stream
 * return 0 on error
 */
int audio_fit_capabilities(audio_output_t *ao,int c,int r)
{
	int rn;
	int f0=0;
	
	if(param.force_8bit) {
		f0 = 2;
	}

	c--; /* stereo=1 ,mono=0 */

	if(param.force_mono >= 0)
		c = 0;
	if(param.force_stereo)
		c = 1;

	if(param.force_rate) {
		rn = rate2num(param.force_rate);
		if(audio_fit_cap_helper(ao,rn,f0,2,c))
			return 1;
		if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
			return 1;

		if(c == 1 && !param.force_stereo)
			c = 0;
		else if(c == 0 && !param.force_mono)
			c = 1;

		if(audio_fit_cap_helper(ao,rn,f0,2,c))
			return 1;
		if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
			return 1;

		error("No supported rate found!");
		return 0;
	}

	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ao,rn,f0,2,c))
		return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ao,rn,f0,2,c))
		return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ao,rn,f0,2,c))
		return 1;

	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
		return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
		return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
		return 1;


        if(c == 1 && !param.force_stereo)
		c = 0;
        else if(c == 0 && !param.force_mono)
                c = 1;

        rn = rate2num(r>>0);
        if(audio_fit_cap_helper(ao,rn,f0,2,c))
                return 1;
        rn = rate2num(r>>1);
        if(audio_fit_cap_helper(ao,rn,f0,2,c))
                return 1;
        rn = rate2num(r>>2);
        if(audio_fit_cap_helper(ao,rn,f0,2,c))
                return 1;

        rn = rate2num(r>>0);
        if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
                return 1;
        rn = rate2num(r>>1);
        if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
                return 1;
        rn = rate2num(r>>2);
        if(audio_fit_cap_helper(ao,rn,2,NUM_ENCODINGS,c))
                return 1;

	error("No supported rate found!");
	return 0;
}

char *audio_encoding_name(int format)
{
	int i;

	for(i=0;i<NUM_ENCODINGS;i++) {
		if(audio_val2name[i].val == format)
			return audio_val2name[i].name;
	}
	return "Unknown";
}




#if !defined(WIN32) && !defined(GENERIC)
static void catch_child(void)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}
#endif


/* FIXME: Old output initialization code that needs updating */

int init_output( audio_output_t *ao )
{
	static int init_done = FALSE;
	
	if (init_done) return 0;
	init_done = TRUE;
	
#ifndef NOXFERMEM
	/*
	 * Only DECODE_AUDIO and DECODE_FILE are sanely handled by the
	 * buffer process. For now, we just ignore the request
	 * to buffer the output. [dk]
	 */
	if (param.usebuffer && (param.outmode != DECODE_AUDIO) && (param.outmode != DECODE_FILE)) {
		fprintf(stderr, "Sorry, won't buffer output unless writing plain audio.\n");
		param.usebuffer = 0;
	} 
  
	if (param.usebuffer) {
		unsigned int bufferbytes;
		sigset_t newsigset, oldsigset;
		if (param.usebuffer < 32) param.usebuffer = 32; /* minimum is 32 Kbytes! */
		bufferbytes = (param.usebuffer * 1024);
		bufferbytes -= bufferbytes % FRAMEBUFUNIT;
		/* +1024 for NtoM rounding problems */
		xfermem_init (&buffermem, bufferbytes ,0,1024);
		pcm_sample = (unsigned char *) buffermem->data;
		pcm_point = 0;
		sigemptyset (&newsigset);
		sigaddset (&newsigset, SIGUSR1);
		sigprocmask (SIG_BLOCK, &newsigset, &oldsigset);
		#if !defined(WIN32) && !defined(GENERIC)
			catchsignal (SIGCHLD, catch_child);
		#endif
		
		switch ((buffer_pid = fork())) {
			case -1: /* error */
				perror("fork()");
				return 1;
			case 0: /* child */
				if(rd) rd->close(rd); /* child doesn't need the input stream */
				xfermem_init_reader (buffermem);
				buffer_loop(ao, &oldsigset);
				xfermem_done_reader (buffermem);
				xfermem_done (buffermem);
				exit(0);
			default: /* parent */
				xfermem_init_writer (buffermem);
				param.outmode = DECODE_BUFFER;
			break;
		}
	} else {
#endif

	/* + 1024 for NtoM rate converter */
	if (!(pcm_sample = (unsigned char *) malloc(audiobufsize * 2 + 1024))) {
		perror ("malloc()");
		return 1;
#ifndef NOXFERMEM
	}
#endif

	}

	switch(param.outmode) {
		case DECODE_AUDIO:
			if(ao->open(ao) < 0) {
				error("failed to open audio device");
				return 1;
			}
		break;
		case DECODE_WAV:
			wav_open(ao,param.filename);
		break;
		case DECODE_AU:
			au_open(ao,param.filename);
		break;
		case DECODE_CDR:
			cdr_open(ao,param.filename);
		break;
	}
	
	return 0;
}


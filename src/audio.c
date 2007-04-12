/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include <stdlib.h>
#include "config.h"
#include "mpg123.h"
#include "debug.h"

audio_output_t*
alloc_audio_output()
{
	audio_output_t* ao = malloc( sizeof( audio_output_t ) );
	if (ao==NULL) error( "Failed to allocate memory for audio_output_t." );

	/* Initialise variables */
	ao->fn = -1;
	ao->rate = -1;
	ao->gain = -1;
	ao->handle = NULL;
	ao->device = NULL;
	ao->channels = -1;
	ao->format = -1;
	
	/* Set the callbacks to NULL */
	ao->open = NULL;
	ao->get_formats = NULL;
	ao->write = NULL;
	ao->flush = NULL;
	ao->close = NULL;
	
	return ao;
}


void audio_output_struct_dump(audio_output_t *ao)
{
	fprintf(stderr, "ao->fn=%d\n", ao->fn);
	fprintf(stderr, "ao->handle=%p\n", ao->handle);
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
		perror("audio");
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


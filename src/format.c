#include "mpg123lib_intern.h"

/* static int chans[NUM_CHANNELS] = { 1 , 2 }; */
static long rats[NUM_RATES] = /* only the standard rates */
{
	 8000, 11025, 12000, 
	16000, 22050, 24000,
	32000, 44100, 48000
};
static int encs[NUM_ENCODINGS] =
{
	MPG123_ENC_SIGNED_16, 
	MPG123_ENC_UNSIGNED_16,
	MPG123_ENC_UNSIGNED_8,
	MPG123_ENC_SIGNED_8,
	MPG123_ENC_ULAW_8,
	MPG123_ENC_ALAW_8
};
/*	char audio_caps[NUM_CHANNELS][NUM_RATES+1][NUM_ENCODINGS]; */

static int rate2num(struct frame *fr, long r)
{
	int i;
	for(i=0;i<NUM_RATES;i++) if(rats[i] == r) return i;
	if(fr->p.special_rate == 0 || fr->p.special_rate == r) return NUM_RATES;

	return -1;
}

static int cap_fit(struct frame *fr, struct audioformat *nf, int f0, int f2)
{
	int i;
	int c  = nf->channels-1;
	int rn = rate2num(fr, nf->rate);
	if(rn >= 0)	for(i=f0;i<f2;i++)
	{
		if(fr->p.audio_caps[c][rn][i])
		{
			nf->encoding = encs[i];
			return 1;
		}
	}
	return 0;
}

static int freq_fit(struct frame *fr, struct audioformat *nf, int f0, int f2)
{
	nf->rate = frame_freq(fr)>>fr->p.down_sample;
	if(cap_fit(fr,nf,f0,f2)) return 1;
	nf->rate>>=1;
	if(cap_fit(fr,nf,f0,f2)) return 1;
	nf->rate>>=1;
	if(cap_fit(fr,nf,f0,f2)) return 1;
	return 0;
}

/* match constraints against supported audio formats, store possible setup in frame
  return: -1: error; 0: no format change; 1: format change */
int frame_output_format(struct frame *fr)
{
	struct audioformat nf;
	int f0=0;
	struct mpg123_parameter *p = &fr->p;
	/* initialize new format, encoding comes later */
	nf.channels = fr->stereo;

	if(p->flags & MPG123_FORCE_8BIT) f0 = 2; /* skip the 16bit encodings */

	/* force stereo is stronger */
	if(p->flags & MPG123_FORCE_MONO)   nf.channels = 1;
	if(p->flags & MPG123_FORCE_STEREO) nf.channels = 2;

	if(p->force_rate)
	{
		nf.rate = p->force_rate;
		if(cap_fit(fr,&nf,f0,2)) goto end;            /* 16bit encodings */
		if(cap_fit(fr,&nf,2,NUM_ENCODINGS)) goto end; /*  8bit encodings */

		/* try again with different stereoness */
		if(nf.channels == 2 && !(p->flags & MPG123_FORCE_STEREO)) nf.channels = 1;
		else if(nf.channels == 1 && !(p->flags & MPG123_FORCE_MONO)) nf.channels = 2;

		if(cap_fit(fr,&nf,f0,2)) goto end;            /* 16bit encodings */
		if(cap_fit(fr,&nf,2,NUM_ENCODINGS)) goto end; /*  8bit encodings */

		if(NOQUIET)
		error3( "Unable to set up output format! Constraints: %s%s%liHz.",
		        ( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
		          (p->flags & MPG123_FORCE_MONO ? "mono, " : "") ),
		        (p->flags & MPG123_FORCE_8BIT ? "8bit, " : ""),
		        p->force_rate );
/*		if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

		fr->err = MPG123_BAD_OUTFORMAT;
		return -1;
	}

	if(freq_fit(fr, &nf, f0, 2)) goto end; /* try rates with 16bit */
	if(freq_fit(fr, &nf,  2, NUM_ENCODINGS)) goto end; /* ... 8bit */

	/* try again with different stereoness */
	if(nf.channels == 2 && !(p->flags & MPG123_FORCE_STEREO)) nf.channels = 1;
	else if(nf.channels == 1 && !(p->flags & MPG123_FORCE_MONO)) nf.channels = 2;

	if(freq_fit(fr, &nf, f0, 2)) goto end; /* try rates with 16bit */
	if(freq_fit(fr, &nf,  2, NUM_ENCODINGS)) goto end; /* ... 8bit */

	/* Here is the _bad_ end. */
	if(NOQUIET)
	error5( "Unable to set up output format! Constraints: %s%s%li, %li or %liHz.",
	        ( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
	          (p->flags & MPG123_FORCE_MONO ? "mono, "  : "") ),
	        (p->flags & MPG123_FORCE_8BIT  ? "8bit, " : ""),
	        frame_freq(fr),  frame_freq(fr)>>1, frame_freq(fr)>>2 );
/*	if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

	fr->err = MPG123_BAD_OUTFORMAT;
	return -1;

end: /* Here is the _good_ end. */
	/* we had a successful match, now see if there's a change */
	if(nf.rate == fr->af.rate && nf.channels == fr->af.channels && nf.encoding == fr->af.encoding)
	return 0; /* the same format as before */
	else /* a new format */
	{
		fr->af.rate = nf.rate;
		fr->af.channels = nf.channels;
		fr->af.encoding = nf.encoding;
		return 1;
	}
}

void mpg123_format_none(mpg123_handle *mh)
{
	memset(mh->p.audio_caps,0,sizeof(mh->p.audio_caps));
}

void mpg123_format_all(mpg123_handle *mh)
{
	memset(mh->p.audio_caps,1,sizeof(mh->p.audio_caps));
}

int mpg123_format(mpg123_handle *mh, long rate, int channels, int encodings)
{
	int ir, ie;
	int ic = channels-1;
	if(ic != 0 && ic != 1)
	{
		mh->err = MPG123_BAD_CHANNEL;
		return -1;
	}
	if(rate <= 0 || rate > 96000)
	{
		mh->err = MPG123_BAD_RATE;
		return -1;
	}
	/* find the rate */
	ir=rate2num(mh, rate);
	if(ir < 0)
	{
		debug1("Setting special rate to %liHz.", rate);
		ir = NUM_RATES;
		mh->p.special_rate = rate;
	}
	/* now match the encodings */
	for(ie = 0; ie < NUM_ENCODINGS; ++ie)
	if(encs[ie] & encodings) mh->p.audio_caps[ic][ir][ie] = 1;

	return 0;
}

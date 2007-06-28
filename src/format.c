static int chans[NUM_CHANNELS] = { 1 , 2 };
static long rats[NUM_RATES] = 
{
	 8000, 11025, 12000, 
	16000, 22050, 24000,
	32000, 44100, 48000,
	8000	/* 8000 = dummy for user forced - that's not fully clear how to handle atm. */
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
/*	char audio_caps[NUM_CHANNELS][NUM_ENCODINGS][NUM_RATES]; */

static int rate2num(int r)
{
	int i;
	for(i=0;i<NUM_RATES;i++) if(rates[i] == r) return i;

	return -1;
}

static int cap_fit(struct audio_info_struct *ai,int rn,int f0,int f2,int c)
{
	int i;

	if(rn >= 0)	for(i=f0;i<f2;i++)
	{
		if(capabilities[c][i][rn])
		{
			ai->rate = rates[rn];
			ai->format = encodings[i];
			ai->channels = channels[c];
			return 1;
		}
	}
	return 0;
}

/* match constraints against supported audio formats, give a possible setup
   this shall include the fr->down_sample stuff */
int mpg123_audio_caps(mpg123_handle *mh, int r)
{
	int rn;
	int f0=0;
	int c = mh->stereo;
	struct mpg123_parameter *p = &mh->p;
	if(p->flags & MPG123_FORCE_8BIT) f0 = 2; /* skip the 16bit encodings */

	c--; /* stereo=1 ,mono=0 */

	/* force stereo is stronger */
	if(p->flags & MPG123_FORCE_MONO) c = 0;
	if(p->flags & MPG123_FORCE_STEREO) c = 1;

	if(p->force_rate) {
		rn = rate2num(p->force_rate);
		/* 16bit encodings */
		if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
		/* 8bit encodings */
		if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;

		/* try again with different stereoness */
		if(c == 1 && !(p->flags & MPG123_FORCE_STEREO))	c = 0;
		else if(c == 0 && !(p->flags & MPG123_FORCE_MONO)) c = 1;

		/* 16bit encodings */
		if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
		/* 8bit encodings */
		if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;

		error3("Unable to set up output device! Constraints: %s%s%liHz.",
		      (p->flags & MPG123_FORCE_STEREO ? "stereo, " :
		       (p->flags & MPG123_FORCE_MONO ? "mono, " : "")),
		      (p->flags & MPG123_FORCE_8BIT ? "8bit, " : ""),
		      p->force_rate);
		error1("cap: %i\n", capabilities[1][9][2]);
		if(param.verbose <= 1) print_capabilities(ai, p);
		return 0;
	}

	/* try different rates with 16bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,f0,2,c))
		return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,f0,2,c))
		return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,f0,2,c))
		return 1;

	/* try different rates with 8bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c))
		return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c))
		return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c))
		return 1;

	/* try again with different stereoness */
	if(c == 1 && !(p->flags & MPG123_FORCE_STEREO))	c = 0;
	else if(c == 0 && !(p->flags & MPG123_FORCE_MONO)) c = 1;

	/* 16bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,f0,2,c)) return 1;

	/* 8bit */
	rn = rate2num(r>>0);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;
	rn = rate2num(r>>1);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;
	rn = rate2num(r>>2);
	if(audio_fit_cap_helper(ai,rn,2,NUM_ENCODINGS,c)) return 1;

	error5("Unable to set up output device! Constraints: %s%s%i, %i or %iHz.",
	      (p->flags & MPG123_FORCE_STEREO ? "stereo, " :
	       (p->flags & MPG123_FORCE_MONO ? "mono, " : "")),
	      (p->flags & MPG123_FORCE_8BIT ? "8bit, " : ""),
	      r, r>>1, r>>2);
	if(param.verbose <= 1) print_capabilities(ai, p);
	return 0;
}

void mpg123_format_none(mpg123_handle *mh)
{
	memset(mh->audio_caps,0,sizeof(mh->audio_caps));
}

void mpg123_format_all(mpg123_handle *mh)
{
	memset(mh->audio_caps,1,sizeof(mh->audio_caps));
}

int mpg123_format(mpg123_handle *mh, long rate, int channels, int encodings)
{
	int ir, ie;
	int ic = channels-1;
	if(ic != 0 && ic != 1) return -1;
	/* find the rate */
	for(ir=0; ir < NUM_RATES; ++ir) if(rats[ir] == rate) break;
	if(ir == NUM_RATES)
	{
		debug1("Setting special rate to %liHz.", rate);
		ir = NUM_RATES-1;
		mh->special_rate = rate;
	}
	/* now match the encodings */
	for(ie = 0; ie < NUM_ENCODINGS; ++ie)
	if(encs[ie] & encodings) mh->audio_caps[ic][ie][ir] = 1;

	return 0;
}

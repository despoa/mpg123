#include "mpg123lib_intern.h"
#include "getcpuflags.h"

/* that's doubled in decode_ntom.c */
#define NTOM_MUL (32768)
#define aligned_pointer(p,type,alignment) \
	(((char*)(p)-(char*)NULL) % (alignment)) \
	? (type*)((char*)(p) + (alignment) - (((char*)(p)-(char*)NULL) % (alignment))) \
	: (type*)(p)

void frame_init(struct frame *fr)
{
	fr->buffer.data = NULL;
	fr->rawbuffs = NULL;
	fr->rawdecwin = NULL;
	fr->conv16to8_buf = NULL;
	fr->cpu_opts.type = defopt;
	fr->cpu_opts.class = (defopt == mmx || defopt == sse || defopt == dreidnowext) ? mmxsse : normal;
	/* these two look unnecessary, check guarantee for synth_ntom_set_step (in control_generic, even)! */
	fr->ntom_val[0] = NTOM_MUL>>1;
	fr->ntom_val[1] = NTOM_MUL>>1;
	fr->ntom_step = NTOM_MUL;
	/* unnecessary: fr->buffer.size = fr->buffer.fill = 0; */
	fr->outscale = MAXOUTBURST;
	fr->lastscale = -1;
	fr->have_eq_settings = 0;
	fr->rd = NULL;
	init_icy(&fr->icy);
	init_id3(fr);
	/* frame_outbuffer is missing... */
	/* frame_buffers is missing... that one needs cpu opt setting! */
	/* after these... frame_reset is needed before starting full decode */
	fr->af.format = 0;
	fr->af.rate = 0;
	fr->af.channels = 0;
	fr->p.flags = 0;
	fr->p.force_rate = 0;
	fr->p.down_sample = 0;
	fr->p.rva = 0;
#ifdef OPT_MULTI
	fr->p.cpu = NULL; /* chosen optimization, can be NULL/""/"auto"*/
#endif
	fr->p.halfspeed = 0;
}

int frame_outbuffer(struct frame *fr)
{
	int fullsize = AUDIOBUFSIZE* 2 + 1024;
	int size = AUDIOBUFSIZE;
	if(fr->buffer.fullsize == 0) fr->buffer.data = NULL; /* if it's not my own buffer */
	if(fr->buffer.data != NULL && fr->buffer.fullsize != fullsize)
	{
		free(fr->buffer.data);
		fr->buffer.data = NULL;
	}
	fr->buffer.fullsize = fullsize;
	fr->buffer.size = size;
	if(fr->buffer.data == NULL) fr->buffer.data = (unsigned char*) malloc(fr->buffer.fullsize);
	if(fr->buffer.data == NULL) return -1;
	fr->buffer.fill = 0;
	return 0;
}

void frame_replace_outbuffer(struct frame *fr, unsigned char *data, int size)
{
	if(fr->buffer.fullsize != 0 && fr->buffer.data != NULL) free(fr->buffer.data);
	fr->buffer.data = data;
	fr->buffer.size = size;
	fr->buffer.fullsize = 0;
	fr->buffer.fill = 0;
}

int frame_buffers(struct frame *fr)
{
	int buffssize = 0;
	debug1("frame %p buffer", (void*)fr);
/*
	the used-to-be-static buffer of the synth functions, has some subtly different types/sizes

	2to1, 4to1, ntom, generic, i386: real[2][2][0x110]
	mmx, sse: short[2][2][0x110]
	i586(_dither): 4352 bytes; int/long[2][2][0x110]
	i486: int[2][2][17*FIR_BUFFER_SIZE]
	altivec: static real __attribute__ ((aligned (16))) buffs[4][4][0x110]

	Huh, altivec looks like fun. Well, let it be large... then, the 16 byte alignment seems to be implicit on MacOSX malloc anyway.
	Let's make a reasonable attempt to allocate enough memory...
	Keep in mind: biggest ones are i486 and altivec (mutually exclusive!), then follows i586 and normal real.
	mmx/sse use short but also real for resampling.
	Thus, minimum is 2*2*0x110*sizeof(real).
*/
	if(fr->cpu_opts.type == altivec) buffssize = 4*4*0x110*sizeof(real);
#ifdef OPT_I486
	else if(fr->cpu_opts.type == ivier) buffssize = 2*2*17*FIR_BUFFER_SIZE*sizeof(int);
#endif
	else if(fr->cpu_opts.type == ifuenf || fr->cpu_opts.type == ifuenf_dither || fr->cpu_opts.type == dreidnow)
	buffssize = 2*2*0x110*4; /* don't rely on type real, we need 4352 bytes */

	if(2*2*0x110*sizeof(real) > buffssize)
	buffssize = 2*2*0x110*sizeof(real);

	if(fr->rawbuffs != NULL && fr->rawbuffss != buffssize)
	{
		free(fr->rawbuffs);
		fr->rawbuffs = NULL;
	}

	if(fr->rawbuffs == NULL) fr->rawbuffs = (unsigned char*) malloc(buffssize);
	if(fr->rawbuffs == NULL) return -1;
	fr->rawbuffss = buffssize;
	fr->short_buffs[0][0] = (short*) fr->rawbuffs;
	fr->short_buffs[0][1] = fr->short_buffs[0][0] + 0x110;
	fr->short_buffs[1][0] = fr->short_buffs[0][1] + 0x110;
	fr->short_buffs[1][1] = fr->short_buffs[1][0] + 0x110;
	fr->real_buffs[0][0] = (real*) fr->rawbuffs;
	fr->real_buffs[0][1] = fr->real_buffs[0][0] + 0x110;
	fr->real_buffs[1][0] = fr->real_buffs[0][1] + 0x110;
	fr->real_buffs[1][1] = fr->real_buffs[1][0] + 0x110;
#ifdef OPT_I486
	if(fr->cpu_opts.type == ivier)
	{
		fr->int_buffs[0][0] = (int*) fr->rawbuffs;
		fr->int_buffs[0][1] = fr->int_buffs[0][0] + 17*FIR_BUFFER_SIZE;
		fr->int_buffs[1][0] = fr->int_buffs[0][1] + 17*FIR_BUFFER_SIZE;
		fr->int_buffs[1][1] = fr->int_buffs[1][0] + 17*FIR_BUFFER_SIZE;
	}
#endif
#ifdef OPT_ALTIVEC
	if(fr->cpu_opts.type == altivec)
	{
		int i,j;
		fr->areal_buffs[0][0] = (real*) fr->rawbuffs;
		for(i=0; i<4; ++i) for(j=0; j<4; ++j)
		fr->areal_buffs[i][j] = fr->areal_buffs[0][0] + (i*4+j)*0x110;
	}
#endif
	/* now the different decwins... all of the same size, actually */
	/* The MMX ones want 32byte alignment, which I'll try to ensure manually */
	{
		int decwin_size = (512+32)*sizeof(real);
		if(fr->rawdecwin != NULL) free(fr->rawdecwin);
#ifdef OPT_MMXORSSE
#ifdef OPT_MULTI
		if(fr->cpu_opts.class == mmxsse)
		{
#endif
			/* decwin_mmx will share, decwins will be appended ... sizeof(float)==4 */
			if(decwin_size < (512+32)*4) decwin_size = (512+32)*4;
			decwin_size += (512+32)*4 + 32; /* the second window + alignment zone */
			/* (512+32)*4/32 == 2176/32 == 68, so one decwin block retains alignment */
#ifdef OPT_MULTI
		}
#endif
#endif
		fr->rawdecwin = (unsigned char*) malloc(decwin_size);
		if(fr->rawdecwin == NULL) return -1;
		fr->decwin = (real*) fr->rawdecwin;
#ifdef OPT_MMXORSSE
#ifdef OPT_MULTI
		if(fr->cpu_opts.class == mmxsse)
		{
#endif
			/* align decwin, assign that to decwin_mmx, append decwins */
			/* I need to add to decwin what is missing to the next full 32 byte -- also I want to make gcc -pedantic happy... */
			fr->decwin = aligned_pointer(fr->rawdecwin,real,32);
			debug1("aligned decwin: %p", (void*)fr->decwin);
			fr->decwin_mmx = (float*)fr->decwin;
			fr->decwins = fr->decwin_mmx+512+32;
#ifdef OPT_MULTI
		}
		else debug("no decwins/decwin_mmx for that class");
#endif
#endif
	}
	debug1("frame %p buffer done", (void*)fr);
	return 0;
}

/* Prepare the handle for a new track.
   That includes (re)allocation or reuse of the output buffer */
int frame_reset(struct frame* fr)
{
	fr->buffer.fill = 0; /* hm, reset buffer fill... did we do a flush? */
	fr->num = 0;
	fr->oldhead = 0;
	fr->firsthead = 0;
	fr->vbr = CBR;
	fr->abr_rate = 0;
	fr->track_frames = 0;
	fr->mean_frames = 0;
	fr->mean_framesize = 0;
	fr->lastscale = -1;
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
	if(fr->p.flags & MPG123_GAPLESS) frame_gapless_init(fr, DECODER_DELAY+GAP_SHIFT, 0);
#endif
	fr->bo[0] = 1; /* the usual bo */
	fr->bo[1] = 0; /* ditherindex */
#ifdef OPT_I486
	fr->bo[0] = fr->bo[1] = FIR_SIZE-1;
#endif
	fr->hybrid_blc[0] = fr->hybrid_blc[1] = 0;
	memset(fr->hybrid_block, 0, sizeof(real)*2*2*SBLIMIT*SSLIMIT);
	reset_id3(fr);
	reset_icy(&fr->icy);
	fr->halfphase = 0; /* here or indeed only on first-time init? */
	return 0;
}

void frame_free_buffers(struct frame *fr)
{
	if(fr->rawbuffs != NULL) free(fr->rawbuffs);
	fr->rawbuffs = NULL;
	if(fr->rawdecwin != NULL) free(fr->rawdecwin);
	fr->rawdecwin = NULL;
	if(fr->conv16to8_buf != NULL) free(fr->conv16to8_buf);
	fr->conv16to8_buf = NULL;
}

void frame_exit(struct frame *fr)
{
	if(fr->buffer.fullsize != 0 && fr->buffer.data != NULL) free(fr->buffer.data);
	fr->buffer.data = NULL;
	frame_free_buffers(fr);
	exit_id3(fr);
	clear_icy(&fr->icy);
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

/* input in bytes already */
void frame_gapless_init(struct frame *fr, unsigned long b, unsigned long e)
{
	fr->position = 0;
	fr->ignore = 0;
	fr->begin_s = b;
	fr->end_s = e;
	debug2("layer3_gapless_init: from %lu to %lu samples", fr->begin_s, fr->end_s);
	if(fr->af.format) frame_gapless_bytify(fr);

}

void frame_gapless_position(struct frame* fr, unsigned long frames)
{
	fr->position = samples_to_bytes(fr, frames*spf(fr));
	debug1("set; position now %lu", fr->position);
}

void frame_gapless_bytify(struct frame *fr)
{
	fr->begin = samples_to_bytes(fr, fr->begin_s);
	fr->end   = samples_to_bytes(fr, fr->end_s);
	debug2("bytified: begin=%lu; end=%5lu", fr->begin, fr->end);
}

/* I need initialized fr here! */
void frame_gapless_ignore(struct frame *fr, unsigned long frames)
{
	fr->ignore = samples_to_bytes(fr, frames*spf(fr));
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

/* to vanish */
void frame_outformat(struct frame *fr, int format, int channels, long rate)
{
	fr->af.format = format;
	fr->af.rate = rate;
	fr->af.channels = channels;
}

/* set synth functions for current frame, optimizations handled by opt_* macros */
int set_synth_functions(struct frame *fr)
{
	int ds = fr->down_sample;
	int p8=0;
	static func_synth funcs[2][4] = { 
		{ NULL,
		  synth_2to1,
		  synth_4to1,
		  synth_ntom } ,
		{ NULL,
		  synth_2to1_8bit,
		  synth_4to1_8bit,
		  synth_ntom_8bit } 
	};
	static func_synth_mono funcs_mono[2][2][4] = {    
		{ { NULL ,
		    synth_2to1_mono2stereo ,
		    synth_4to1_mono2stereo ,
		    synth_ntom_mono2stereo } ,
		  { NULL ,
		    synth_2to1_8bit_mono2stereo ,
		    synth_4to1_8bit_mono2stereo ,
		    synth_ntom_8bit_mono2stereo } } ,
		{ { NULL ,
		    synth_2to1_mono ,
		    synth_4to1_mono ,
		    synth_ntom_mono } ,
		  { NULL ,
		    synth_2to1_8bit_mono ,
		    synth_4to1_8bit_mono ,
		    synth_ntom_8bit_mono } }
	};

	/* possibly non-constand entries filled here */
	funcs[0][0] = (func_synth) opt_synth_1to1(fr);
	funcs[1][0] = (func_synth) opt_synth_1to1_8bit(fr);
	funcs_mono[0][0][0] = (func_synth_mono) opt_synth_1to1_mono2stereo(fr);
	funcs_mono[0][1][0] = (func_synth_mono) opt_synth_1to1_8bit_mono2stereo(fr);
	funcs_mono[1][0][0] = (func_synth_mono) opt_synth_1to1_mono(fr);
	funcs_mono[1][1][0] = (func_synth_mono) opt_synth_1to1_8bit_mono(fr);

	if((fr->af.format & MPG123_FORMAT_MASK) == MPG123_FORMAT_8)
		p8 = 1;
	fr->synth = funcs[p8][ds];
	fr->synth_mono = funcs_mono[fr->af.channels==2 ? 0 : 1][p8][ds];

	if(p8) {
		if(make_conv16to8_table(fr, fr->af.format) != 0)
		{
			/* it's a bit more work to get proper error propagation up */
			return -1;
		}
	}
	return 0;
}

void do_volume(struct frame *fr, double factor)
{
	if(factor < 0) factor = 0;
	/* change the output scaling and apply with rva */
	fr->outscale = (double) MAXOUTBURST * factor;
	do_rva(fr);
}

/* adjust the volume, taking both fr->outscale and rva values into account */
void do_rva(struct frame *fr)
{
	double rvafact = 1;
	float peak = 0;
	scale_t newscale;

	if(fr->p.rva)
	{
		int rt = 0;
		/* Should one assume a zero RVA as no RVA? */
		if(fr->p.rva == 2 && fr->rva.level[1] != -1) rt = 1;
		if(fr->rva.level[rt] != -1)
		{
			rvafact = pow(10,fr->rva.gain[rt]/20);
			peak = fr->rva.peak[rt];
			if(NOQUIET && fr->p.verbose > 1) fprintf(stderr, "Note: doing RVA with gain %f\n", fr->rva.gain[rt]);
		}
		else
		{
			warning("no RVA value found");
		}
	}

	newscale = fr->outscale*rvafact;

	/* if peak is unknown (== 0) this check won't hurt */
	if((peak*newscale) > MAXOUTBURST)
	{
		newscale = (scale_t) ((double) MAXOUTBURST/peak);
		warning2("limiting scale value to %li to prevent clipping with indicated peak factor of %f", newscale, peak);
	}
	/* first rva setting is forced with fr->lastscale < 0 */
	if(newscale != fr->lastscale)
	{
		debug3("changing scale value from %li to %li (peak estimated to %li)", fr->lastscale != -1 ? fr->lastscale : fr->outscale, newscale, (long) (newscale*peak));
		fr->lastscale = newscale;
		opt_make_decode_tables(fr); /* the actual work */
	}
}

#ifdef OPT_MULTI

int frame_cpu_opt(struct frame *fr)
{
	char* chosen = ""; /* the chosed decoder opt as string */
	int auto_choose = 0;
	int done = 0;
	if(   (fr->p.cpu == NULL)
	   || (fr->p.cpu[0] == 0)
	   || !strcasecmp(fr->p.cpu, "auto") )
	auto_choose = 1;

	/* covers any i386+ cpu; they actually differ only in the synth_1to1 function... */
	#ifdef OPT_X86

	#ifdef OPT_MMXORSSE
	fr->cpu_opts.make_decode_tables   = make_decode_tables;
	fr->cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2;
	fr->cpu_opts.init_layer2_table    = init_layer2_table;
	#endif
	#ifdef OPT_3DNOW
	fr->cpu_opts.dct36 = dct36;
	#endif
	#ifdef OPT_3DNOWEXT
	fr->cpu_opts.dct36 = dct36;
	#endif

	if(cpu_i586(cpu_flags))
	{
		debug2("standard flags: 0x%08x\textended flags: 0x%08x\n", cpu_flags.std, cpu_flags.ext);
		#ifdef OPT_3DNOWEXT
		if(   !done && (auto_choose || !strcasecmp(fr->p.cpu, "3dnowext"))
		   && cpu_3dnow(cpu_flags)
		   && cpu_3dnowext(cpu_flags)
		   && cpu_mmx(cpu_flags) )
		{
			int go = 1;
			if(fr->p.force_rate)
			{
				#if defined(K6_FALLBACK) || defined(PENTIUM_FALLBACK)
				if(!auto_choose){ if(NOQUIET) error("I refuse to choose 3DNowExt as this will screw up with forced rate!"); }
				else if(VERBOSE) fprintf(stderr, "Note: Not choosing 3DNowExt because flexible rate not supported.\n");

				go = 0;
				#else
				if(NOQUIET) error("You will hear some awful sound because of flexible rate being chosen with SSE decoder!");
				#endif
			}
			if(go){ /* temporary hack for flexible rate bug, not going indent this - fix it instead! */
			chosen = "3DNowExt";
			fr->cpu_opts.type = dreidnowext;
			fr->cpu_opts.class = mmxsse;
			fr->cpu_opts.dct36 = dct36_3dnowext;
			fr->cpu_opts.synth_1to1 = synth_1to1_3dnowext;
			fr->cpu_opts.dct64 = dct64_mmx; /* only use the 3dnow version in the synth_1to1_sse */
			fr->cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			fr->cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			fr->cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			fr->cpu_opts.mpl_dct64 = dct64_3dnowext;
			done = 1;
			}
		}
		#endif
		#ifdef OPT_SSE
		if(   !done && (auto_choose || !strcasecmp(fr->p.cpu, "sse"))
		   && cpu_sse(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			int go = 1;
			if(fr->p.force_rate)
			{
				#ifdef PENTIUM_FALLBACK
				if(!auto_choose){ if(NOQUIET) error("I refuse to choose SSE as this will screw up with forced rate!"); }
				else if(VERBOSE) fprintf(stderr, "Note: Not choosing SSE because flexible rate not supported.\n");

				go = 0;
				#else
				if(NOQUIET) error("You will hear some awful sound because of flexible rate being chosen with SSE decoder!");
				#endif
			}
			if(go){ /* temporary hack for flexible rate bug, not going indent this - fix it instead! */
			chosen = "SSE";
			fr->cpu_opts.type = sse;
			fr->cpu_opts.class = mmxsse;
			fr->cpu_opts.synth_1to1 = synth_1to1_sse;
			fr->cpu_opts.dct64 = dct64_mmx; /* only use the sse version in the synth_1to1_sse */
			fr->cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			fr->cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			fr->cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			fr->cpu_opts.mpl_dct64 = dct64_sse;
			done = 1;
			}
		}
		#endif
		#ifdef OPT_3DNOW
		fr->cpu_opts.dct36 = dct36;
		/* TODO: make autodetection for _all_ x86 optimizations (maybe just for i586+ and keep separate 486 build?) */
		/* check cpuflags bit 31 (3DNow!) and 23 (MMX) */
		if(    !done && (auto_choose || !strcasecmp(fr->p.cpu, "3dnow"))
		    && cpu_3dnow(cpu_flags) && cpu_mmx(cpu_flags) )
		{
			chosen = "3DNow";
			fr->cpu_opts.type = dreidnow;
			fr->cpu_opts.dct36 = dct36_3dnow; /* 3DNow! optimized dct36() */
			fr->cpu_opts.synth_1to1 = synth_1to1_3dnow;
			fr->cpu_opts.dct64 = dct64_i386; /* use the 3dnow one? */
			done = 1;
		}
		#endif
		#ifdef OPT_MMX
		if(   !done && (auto_choose || !strcasecmp(fr->p.cpu, "mmx"))
		   && cpu_mmx(cpu_flags) )
		{
			int go = 1;
			if(fr->p.force_rate)
			{
				#ifdef PENTIUM_FALLBACK
				if(!auto_choose){ if(NOQUIET) error("I refuse to choose MMX as this will screw up with forced rate!"); }
				else if(VERBOSE) fprintf(stderr, "Note: Not choosing MMX because flexible rate not supported.\n");

				go = 0;
				#else
				error("You will hear some awful sound because of flexible rate being chosen with MMX decoder!");
				#endif
			}
			if(go){ /* temporary hack for flexible rate bug, not going indent this - fix it instead! */
			chosen = "MMX";
			fr->cpu_opts.type = mmx;
			fr->cpu_opts.class = mmxsse;
			fr->cpu_opts.synth_1to1 = synth_1to1_mmx;
			fr->cpu_opts.dct64 = dct64_mmx;
			fr->cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			fr->cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			fr->cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			done = 1;
			}
		}
		#endif
		#ifdef OPT_I586
		if(!done && (auto_choose || !strcasecmp(fr->p.cpu, "i586")))
		{
			chosen = "i586/pentium";
			fr->cpu_opts.type = ifuenf;
			fr->cpu_opts.synth_1to1 = synth_1to1_i586;
			fr->cpu_opts.synth_1to1_i586_asm = synth_1to1_i586_asm;
			fr->cpu_opts.dct64 = dct64_i386;
			done = 1;
		}
		#endif
		#ifdef OPT_I586_DITHER
		if(!done && (auto_choose || !strcasecmp(fr->p.cpu, "i586_dither")))
		{
			chosen = "dithered i586/pentium";
			fr->cpu_opts.type = ifuenf_dither;
			fr->cpu_opts.synth_1to1 = synth_1to1_i586;
			fr->cpu_opts.dct64 = dct64_i386;
			fr->cpu_opts.synth_1to1_i586_asm = synth_1to1_i586_asm_dither;
			done = 1;
		}
		#endif
	}
	#ifdef OPT_I486 /* that won't cooperate nicely in multi opt mode - forcing i486 in layer3.c */
	if(!done && (auto_choose || !strcasecmp(fr->p.cpu, "i486")))
	{
		chosen = "i486";
		fr->cpu_opts.type = ivier;
		fr->cpu_opts.synth_1to1 = synth_1to1_i386; /* i486 function is special */
		fr->cpu_opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif
	#ifdef OPT_I386
	if(!done && (auto_choose || !strcasecmp(fr->p.cpu, "i386")))
	{
		chosen = "i386";
		fr->cpu_opts.type = idrei;
		fr->cpu_opts.synth_1to1 = synth_1to1_i386;
		fr->cpu_opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif

	if(done) /* set common x86 functions */
	{
		fr->cpu_opts.synth_1to1_mono = synth_1to1_mono_i386;
		fr->cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_i386;
		fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_i386;
		fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_i386;
		fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_i386;
	}
	#endif /* OPT_X86 */

	#ifdef OPT_ALTIVEC
	if(!done && (auto_choose || !strcasecmp(fr->p.cpu, "altivec")))
	{
		chosen = "AltiVec";
		fr->cpu_opts.type = altivec;
		fr->cpu_opts.dct64 = dct64_altivec;
		fr->cpu_opts.synth_1to1 = synth_1to1_altivec;
		fr->cpu_opts.synth_1to1_mono = synth_1to1_mono_altivec;
		fr->cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_altivec;
		fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit_altivec;
		fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_altivec;
		fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_altivec;
		done = 1;
	}
	#endif

	#ifdef OPT_GENERIC
	if(!done && (auto_choose || !strcasecmp(fr->p.cpu, "generic")))
	{
		chosen = "generic";
		fr->cpu_opts.type = generic;
		fr->cpu_opts.dct64 = dct64;
		fr->cpu_opts.synth_1to1 = synth_1to1;
		fr->cpu_opts.synth_1to1_mono = synth_1to1_mono;
		fr->cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo;
		fr->cpu_opts.synth_1to1_8bit = synth_1to1_8bit;
		fr->cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono;
		fr->cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo;
		done = 1;
	}
	#endif

	if(done)
	{
		if(VERBOSE) fprintf(stderr, "decoder: %s\n", chosen);
		return 1;
	}
	else
	{
		if(NOQUIET) error("Could not set optimization!");
		return 0;
	}
}

#endif

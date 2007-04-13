/*
	mpg123: main code of the program (not of the decoder...)

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include "config.h"
#include "debug.h"
#define ME "main"

#include <stdlib.h>
#include <sys/types.h>
#if !defined(WIN32) && !defined(GENERIC)
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

#include "mpg123.h"
#include "common.h"
#include "getlopt.h"
#include "buffer.h"
#include "term.h"
#ifdef GAPLESS
#include "layer3.h"
#endif
#include "playlist.h"
#include "id3.h"
#include "icy.h"

#ifdef MPLAYER
/* disappear! */
func_dct64 mpl_dct64;
#endif

static void usage(int err);
static void want_usage(char* arg);
static void long_usage(int err);
static void want_long_usage(char* arg);
static void print_title(FILE* o);
static void give_version(char* arg);

struct parameter param = { 
  FALSE,		/* aggressiv */
  FALSE,		/* shuffle */
  FALSE,		/* remote */
  FALSE,		/* remote to stderr */
  DECODE_AUDIO,	/* write samples to audio device */
  FALSE,		/* silent operation */
  FALSE,		/* xterm title on/off */
  0,			/* second level buffer size */
  TRUE,			/* resync after stream error */
  0,			/* verbose level */
  DEFAULT_OUTPUT_MODULE,	/* output module */
  NULL,   					/* output device */
#ifdef HAVE_TERMIOS
  FALSE,		/* term control */
#endif
  -1,			/* force mono */
  0,			/* force stereo */
  0,			/* force 8bit */
  0,			/* force rate */
  0,			/* down sample */
  FALSE,		/* checkrange */
  0,			/* doublespeed */
  0,			/* halfspeed */
  0,			/* force_reopen, always (re)opens audio device for next song */
  #ifdef OPT_3DNOW
  0,     /* autodetect from CPUFLAGS */
  #endif
  /* test_cpu flag is valid for multi and 3dnow.. even if 3dnow is built alone; ensure it appears only once */
  #ifdef OPT_MULTI
  FALSE, /* normal operation */
  #else
  #ifdef OPT_3DNOW
  FALSE, /* normal operation */
  #endif
  #endif
  FALSE,  /* try to run process in 'realtime mode' */   
  { 0,},  /* wav,cdr,au Filename */
#ifdef GAPLESS
	0, /* gapless off per default - yet */
#endif
	0, /* default is to play all titles in playlist */
	0, /* do not use rva per default */
	NULL, /* no playlist per default */
	0 /* condensed id3 per default */
	#ifdef OPT_MULTI
	,NULL /* choose optimization */
	,0
	#endif
};

char *prgName = NULL;
char *equalfile = NULL;
/* ThOr: pointers are not TRUE or FALSE */
int have_eq_settings = FALSE;
long outscale  = MAXOUTBURST;
long numframes = -1;
long startFrame= 0;
int buffer_fd[2];
int buffer_pid;

static int intflag = FALSE;
int OutputDescriptor;

audio_output_t *ao = NULL;
audio_output_t pre_ao;
static struct frame fr;
txfermem *buffermem = NULL;




#if !defined(WIN32) && !defined(GENERIC)
static void catch_interrupt(void)
{
	intflag = TRUE;
}
#endif

/* oh, what a mess... */
void next_track(void)
{
	intflag = TRUE;
}

void safe_exit(int code)
{
#ifdef HAVE_TERMIOS
	if(param.term_ctrl)
		term_restore();
#endif
	exit(code);
}


void set_synth_functions(struct frame *fr);

static void set_output( char *arg )
{
	int i;
		
	/* Search for a colon and set the device if found */
	for(i=0; i< strlen( arg ); i++) {
		if (arg[i] == ':') {
			arg[i] = 0;
			param.output_device = &arg[i+1];
			debug1("Setting output device: %s", param.output_device);
			break;
		}	
	}

	/* Set the output module */
	param.output_module = arg;
	debug1("Setting output module: %s", param.output_module );

}

static void set_verbose (char *arg)
{
    param.verbose++;
}

static void set_out_wav(char *arg)
{
	param.outmode = DECODE_WAV;
	strncpy(param.filename,arg,255);
	param.filename[255] = 0;
}

static void set_out_cdr(char *arg)
{
	param.outmode = DECODE_CDR;
	strncpy(param.filename,arg,255);
	param.filename[255] = 0;
}

static void set_out_au(char *arg)
{
	param.outmode = DECODE_AU;
	strncpy(param.filename,arg,255);
	param.filename[255] = 0;
}

static void set_out_file(char *arg)
{
	param.outmode=DECODE_FILE;
	OutputDescriptor=open(arg,O_WRONLY,0);
	if(OutputDescriptor==-1) {
		error2("Can't open %s for writing (%s).\n",arg,strerror(errno));
		safe_exit(1);
	}
}

static void set_out_stdout(char *arg)
{
	param.outmode=DECODE_FILE;
	OutputDescriptor=1;
}

static void set_out_stdout1(char *arg)
{
	param.outmode=DECODE_AUDIOFILE;
	OutputDescriptor=1;
}

void realtime_not_compiled(char *arg)
{
	fprintf(stderr,"Option '-T / --realtime' not compiled into this binary.\n");
}

/* Please note: GLO_NUM expects point to LONG! */
/* ThOr:
 *  Yeah, and despite that numerous addresses to int variables were 
passed.
 *  That's not good on my Alpha machine with int=32bit and long=64bit!
 *  Introduced GLO_INT and GLO_LONG as different bits to make that clear.
 *  GLO_NUM no longer exists.
 */
topt opts[] = {
	{'k', "skip",        GLO_ARG | GLO_LONG, 0, &startFrame, 0},
	{'o', "output",      GLO_ARG | GLO_CHAR, set_output, NULL,  0},
	{'a', "audiodevice", GLO_ARG | GLO_CHAR, 0, &param.output_device,  0},
	{'2', "2to1",        GLO_INT,  0, &param.down_sample, 1},
	{'4', "4to1",        GLO_INT,  0, &param.down_sample, 2},
	{'t', "test",        GLO_INT,  0, &param.outmode, DECODE_TEST},
	{'s', "stdout",      GLO_INT,  set_out_stdout, &param.outmode, DECODE_FILE},
	{'S', "STDOUT",      GLO_INT,  set_out_stdout1, &param.outmode,DECODE_AUDIOFILE},
	{'O', "outfile",     GLO_ARG | GLO_CHAR, set_out_file, NULL, 0},
	{'c', "check",       GLO_INT,  0, &param.checkrange, TRUE},
	{'v', "verbose",     0,        set_verbose, 0,           0},
	{'q', "quiet",       GLO_INT,  0, &param.quiet, TRUE},
	{'y', "resync",      GLO_INT,  0, &param.tryresync, FALSE},
	{'0', "single0",     GLO_INT,  0, &param.force_mono, 0},
	{0,   "left",        GLO_INT,  0, &param.force_mono, 0},
	{'1', "single1",     GLO_INT,  0, &param.force_mono, 1},
	{0,   "right",       GLO_INT,  0, &param.force_mono, 1},
	{'m', "singlemix",   GLO_INT,  0, &param.force_mono, 3},
	{0,   "mix",         GLO_INT,  0, &param.force_mono, 3},
	{0,   "mono",        GLO_INT,  0, &param.force_mono, 3},
	{0,   "stereo",      GLO_INT,  0, &param.force_stereo, 1},
	{0,   "reopen",      GLO_INT,  0, &param.force_reopen, 1},
/*	{'g', "gain",        GLO_ARG | GLO_LONG, 0, &param.gain,    0}, FIXME */
	{'r', "rate",        GLO_ARG | GLO_LONG, 0, &param.force_rate,  0},
	{0,   "8bit",        GLO_INT,  0, &param.force_8bit, 1},
	{'f', "scale",       GLO_ARG | GLO_LONG, 0, &outscale,   0},
	{'n', "frames",      GLO_ARG | GLO_LONG, 0, &numframes,  0},
	#ifdef HAVE_TERMIOS
	{'C', "control",     GLO_INT,  0, &param.term_ctrl, TRUE},
	#endif
	{'b', "buffer",      GLO_ARG | GLO_LONG, 0, &param.usebuffer,  0},
	{'R', "remote",      GLO_INT,  0, &param.remote, TRUE},
	{0,   "remote-err",  GLO_INT,  0, &param.remote_err, TRUE},
	{'d', "doublespeed", GLO_ARG | GLO_LONG, 0, &param.doublespeed,0},
	{'h', "halfspeed",   GLO_ARG | GLO_LONG, 0, &param.halfspeed,  0},
	{'p', "proxy",       GLO_ARG | GLO_CHAR, 0, &proxyurl,   0},
	{'@', "list",        GLO_ARG | GLO_CHAR, 0, &param.listname,   0},
	/* 'z' comes from the the german word 'zufall' (eng: random) */
	{'z', "shuffle",     GLO_INT,  0, &param.shuffle, 1},
	{'Z', "random",      GLO_INT,  0, &param.shuffle, 2},
	{'E', "equalizer",	 GLO_ARG | GLO_CHAR, 0, &equalfile,1},
	#ifdef HAVE_SETPRIORITY
	{0,   "aggressive",	 GLO_INT,  0, &param.aggressive, 2},
	#endif
	#ifdef OPT_3DNOW
	{0,   "force-3dnow", GLO_INT,  0, &param.stat_3dnow, 1},
	{0,   "no-3dnow",    GLO_INT,  0, &param.stat_3dnow, 2},
	{0,   "test-3dnow",  GLO_INT,  0, &param.test_cpu, TRUE},
	#endif
	#ifdef OPT_MULTI
	{0, "cpu", GLO_ARG | GLO_CHAR, 0, &param.cpu,  0},
	{0, "test-cpu",  GLO_INT,  0, &param.test_cpu, TRUE},
	{0, "list-cpu", GLO_INT,  0, &param.list_cpu, 1},
	#endif
	#if !defined(WIN32) && !defined(GENERIC)
	{'u', "auth",        GLO_ARG | GLO_CHAR, 0, &httpauth,   0},
	#endif
	#ifdef HAVE_SCHED_SETSCHEDULER
	/* check why this should be a long variable instead of int! */
	{'T', "realtime",    GLO_LONG,  0, &param.realtime, TRUE },
	#else
	{'T', "realtime",    0,  realtime_not_compiled, 0,           0 },    
	#endif
	{0, "title",         GLO_INT,  0, &param.xterm_title, TRUE },
	{'w', "wav",         GLO_ARG | GLO_CHAR, set_out_wav, 0, 0 },
	{0, "cdr",           GLO_ARG | GLO_CHAR, set_out_cdr, 0, 0 },
	{0, "au",            GLO_ARG | GLO_CHAR, set_out_au, 0, 0 },
	#ifdef GAPLESS
	{0,   "gapless",	 GLO_INT,  0, &param.gapless, 1},
	#endif
	{'?', "help",            0,  want_usage, 0,           0 },
	{0, "longhelp",        0,  want_long_usage, 0,      0 },
	{0, "version",         0,  give_version, 0,         0 },
	{'l', "listentry",       GLO_ARG | GLO_LONG, 0, &param.listentry, 0 },
	{0, "rva-mix",         GLO_INT,  0, &param.rva, 1 },
	{0, "rva-radio",         GLO_INT,  0, &param.rva, 1 },
	{0, "rva-album",         GLO_INT,  0, &param.rva, 2 },
	{0, "rva-audiophile",         GLO_INT,  0, &param.rva, 2 },
	{0, "long-tag",         GLO_INT,  0, &param.long_id3, 1 },
	{0, 0, 0, 0, 0, 0}
};

/*
 *   Change the playback sample rate.
 *   Consider that changing it after starting playback is not covered by gapless code!
 */
static void reset_audio(void)
{
#ifndef NOXFERMEM
	if (param.usebuffer) {
		/* wait until the buffer is empty,
		 * then tell the buffer process to
		 * change the sample rate.   [OF]
		 */
		while (xfermem_get_usedspace(buffermem)	> 0)
			if (xfermem_block(XF_WRITER, buffermem) == XF_CMD_TERMINATE) {
				intflag = TRUE;
				break;
			}
		buffermem->freeindex = -1;
		buffermem->readindex = 0; /* I know what I'm doing! ;-) */
		buffermem->freeindex = 0;
		if (intflag)
			return;
		buffermem->buf[0] = ao->rate; 
		buffermem->buf[1] = ao->channels; 
		buffermem->buf[2] = ao->format;
		buffer_reset();
	}
	else 
#endif
	if (param.outmode == DECODE_AUDIO) {
		/* audio_reset_parameters(ao); */
		/*   close and re-open in order to flush
		 *   the device's internal buffer before
		 *   changing the sample rate.   [OF]
		 */
		ao->close(ao);
		if (ao->open(ao) < 0) {
			error("failed to open audio device");
			safe_exit(1);
		}
	}
}


/*
	precog the audio rate that will be set before output begins
	this is needed to give gapless code a chance to keep track for firstframe != 0
*/
void prepare_audioinfo(struct frame *fr, audio_output_t *ao)
{
	long newrate = freqs[fr->sampling_frequency]>>(param.down_sample);
	fr->down_sample = param.down_sample;
	if(!audio_fit_capabilities(ao,fr->stereo,newrate)) safe_exit(1);
}

/*
 * play a frame read by read_frame();
 * (re)initialize audio if necessary.
 *
 * needs a major rewrite .. it's incredible ugly!
 */
int play_frame(int init,struct frame *fr)
{
	int clip;
	long newrate;
	long old_rate,old_format,old_channels;

	if(fr->header_change || init) {

		if (!param.quiet && init) {
			if (param.verbose)
				print_header(fr);
			else
				print_header_compact(fr);
		}

		if(fr->header_change > 1 || init) {
			old_rate = ao->rate;
			old_format = ao->format;
			old_channels = ao->channels;

			newrate = freqs[fr->sampling_frequency]>>(param.down_sample);
			prepare_audioinfo(fr, ao);
			if(param.verbose > 1) fprintf(stderr, "Note: audio output rate = %li\n", ao->rate);
			#ifdef GAPLESS
			if(param.gapless && (fr->lay == 3)) layer3_gapless_bytify(fr, ao);
			#endif
			
			/* check, whether the fitter set our proposed rate */
			if(ao->rate != newrate) {
				if(ao->rate == (newrate>>1) )
					fr->down_sample++;
				else if(ao->rate == (newrate>>2) )
					fr->down_sample+=2;
				else {
					fr->down_sample = 3;
					fprintf(stderr,"Warning, flexible rate not heavily tested!\n");
				}
				if(fr->down_sample > 3)
					fr->down_sample = 3;
			}

			switch(fr->down_sample) {
				case 0:
				case 1:
				case 2:
					fr->down_sample_sblimit = SBLIMIT>>(fr->down_sample);
					break;
				case 3:
					{
						long n = freqs[fr->sampling_frequency];
                                                long m = ao->rate;

						if(!synth_ntom_set_step(n,m)) return 0;

						if(n>m) {
							fr->down_sample_sblimit = SBLIMIT * m;
							fr->down_sample_sblimit /= n;
						}
						else {
							fr->down_sample_sblimit = SBLIMIT;
						}
					}
					break;
			}

			if (init_output( ao )) {
				safe_exit(-1);
			}
			
			if(ao->rate != old_rate || ao->channels != old_channels ||
			   ao->format != old_format || param.force_reopen) {
				if(param.force_mono < 0) {
					if(ao->channels == 1)
						fr->single = 3;
					else
						fr->single = -1;
				}
				else
					fr->single = param.force_mono;

				param.force_stereo &= ~0x2;
				if(fr->single >= 0 && ao->channels == 2) {
					param.force_stereo |= 0x2;
				}

				set_synth_functions(fr);
				init_layer3(fr->down_sample_sblimit);
				reset_audio();
				if(param.verbose) {
					if(fr->down_sample == 3) {
						long n = freqs[fr->sampling_frequency];
						long m = ao->rate;
						if(n > m) {
							fprintf(stderr,"Audio: %2.4f:1 conversion,",(float)n/(float)m);
						}
						else {
							fprintf(stderr,"Audio: 1:%2.4f conversion,",(float)m/(float)n);
						}
					}
					else {
						fprintf(stderr,"Audio: %ld:1 conversion,",(long)pow(2.0,fr->down_sample));
					}
 					fprintf(stderr," rate: %ld, encoding: %s, channels: %d\n",ao->rate,audio_encoding_name(ao->format),ao->channels);
				}
			}
			if (intflag)
				return 1;
		}
	}

	if (fr->error_protection) {
		getbits(16); /* skip crc */
	}

	/* do the decoding */
	clip = (fr->do_layer)(fr,param.outmode,ao);

#ifndef NOXFERMEM
	if (param.usebuffer) {
		if (!intflag) {
			buffermem->freeindex =
				(buffermem->freeindex + pcm_point) % buffermem->size;
			if (buffermem->wakeme[XF_READER])
				xfermem_putcmd(buffermem->fd[XF_WRITER], XF_CMD_WAKEUP_INFO);
		}
		pcm_sample = (unsigned char *) (buffermem->data + buffermem->freeindex);
		pcm_point = 0;
		while (xfermem_get_freespace(buffermem) < (FRAMEBUFUNIT << 1))
			if (xfermem_block(XF_WRITER, buffermem) == XF_CMD_TERMINATE) {
				intflag = TRUE;
				break;
			}
		if (intflag)
			return 1;
	}
#endif

	if(clip > 0 && param.checkrange)
		fprintf(stderr,"%d samples clipped\n", clip);
	return 1;
}

/* set synth functions for current frame, optimizations handled by opt_* macros */
void set_synth_functions(struct frame *fr)
{
	int ds = fr->down_sample;
	int p8=0;
	static func_synth funcs[2][4] = { 
		{ NULL,
		  synth_2to1,
		  synth_4to1,
		  synth_ntom },
		{ NULL,
		  synth_2to1_8bit,
		  synth_4to1_8bit,
		  synth_ntom_8bit } 
	};
	static func_synth_mono funcs_mono[2][2][4] = {    
		{ { NULL,
		    synth_2to1_mono2stereo,
		    synth_4to1_mono2stereo,
		    synth_ntom_mono2stereo },
		  { NULL,
		    synth_2to1_8bit_mono2stereo,
		    synth_4to1_8bit_mono2stereo,
		    synth_ntom_8bit_mono2stereo } },
		{ { NULL,
		    synth_2to1_mono,
		    synth_4to1_mono,
		    synth_ntom_mono },
		  { NULL,
		    synth_2to1_8bit_mono,
		    synth_4to1_8bit_mono,
		    synth_ntom_8bit_mono } }
	};

	/* possibly non-constand entries filled here */
	funcs[0][0] = opt_synth_1to1;
	funcs[1][0] = opt_synth_1to1_8bit;
	funcs_mono[0][0][0] = opt_synth_1to1_mono2stereo;
	funcs_mono[0][1][0] = opt_synth_1to1_8bit_mono2stereo;
	funcs_mono[1][0][0] = opt_synth_1to1_mono;
	funcs_mono[1][1][0] = opt_synth_1to1_8bit_mono;

	if((ao->format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_8)
		p8 = 1;
	fr->synth = funcs[p8][ds];
	fr->synth_mono = funcs_mono[param.force_stereo?0:1][p8][ds];

	if(p8) {
		if(make_conv16to8_table(ao->format) != 0)
		{
			/* it's a bit more work to get proper error propagation up */
			safe_exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	int result;
	char *fname;
#if !defined(WIN32) && !defined(GENERIC)
	struct timeval start_time, now;
	unsigned long secdiff;
#endif	
	int init;
	#ifdef GAPLESS
	int pre_init;
	#endif
	int j;

#ifdef OS2
        _wildcard(&argc,&argv);
#endif

	if(sizeof(short) != 2) {
		fprintf(stderr,"Ouch SHORT has size of %d bytes (required: '2')\n",(int)sizeof(short));
		safe_exit(1);
	}
	if(sizeof(long) < 4) {
		fprintf(stderr,"Ouch LONG has size of %d bytes (required: at least 4)\n",(int)sizeof(long));
	}

	(prgName = strrchr(argv[0], '/')) ? prgName++ : (prgName = argv[0]);


	while ((result = getlopt(argc, argv, opts)))
	switch (result) {
		case GLO_UNKNOWN:
			fprintf (stderr, "%s: Unknown option \"%s\".\n", 
				prgName, loptarg);
			usage(1);
		case GLO_NOARG:
			fprintf (stderr, "%s: Missing argument for option \"%s\".\n",
				prgName, loptarg);
			usage(1);
	}

	#ifdef OPT_MULTI
	if(param.list_cpu)
	{
		list_cpu_opt();
		safe_exit(0);
	}
	if (param.test_cpu)
	{
		test_cpu_flags();
		safe_exit(0);
	}
	if(!set_cpu_opt()) safe_exit(1);
	#else
	#ifdef OPT_3DNOW
	if (param.test_cpu) {
		struct cpuflags cf;
		getcpuflags(&cf);
		fprintf(stderr,"CPUFLAGS = %08x\n",cf.ext);
		if ((cf.ext & 0x00800000) == 0x00800000) {
			fprintf(stderr,"MMX instructions are supported.\n");
		}
		if ((cf.ext & 0x80000000) == 0x80000000) {
			fprintf(stderr,"3DNow! instructions are supported.\n");
		}
		safe_exit(0);
	}
	#endif
	#endif
	#ifdef MPLAYER
	mpl_dct64 = opt_mpl_dct64;
	#endif

	if (loptind >= argc && !param.listname && !param.remote)
		usage(1);

#if !defined(WIN32) && !defined(GENERIC)
	if (param.remote) {
		param.verbose = 0;        
		param.quiet = 1;
	}
#endif

	if (!(param.listentry < 0) && !param.quiet)
		print_title(stderr); /* do not pollute stdout! */

	if(param.force_mono >= 0) {
		fr.single = param.force_mono;
	}

	if(param.force_rate && param.down_sample) {
		error("Down sampling and fixed rate options not allowed together!");
		safe_exit(1);
	}

	/* Open audio output module */
	ao = open_output_module( param.output_module );
	if (!ao) {
		error("Failed to open audio output module.");
		safe_exit(1);
	}
	
	audio_capabilities(ao);
	
	
	
	/* equalizer initialization regardless of equalfile */
	for(j=0; j<32; j++) {
		equalizer[0][j] = equalizer[1][j] = 1.0;
		equalizer_sum[0][j] = equalizer_sum[1][j] = 0.0;
	}
	if(equalfile != NULL) { /* tst; ThOr: not TRUE or FALSE: allocated or not... */
		FILE *fe;
		int i;

		equalizer_cnt = 0;

		fe = fopen(equalfile,"r");
		if(fe) {
			char line[256];
			for(i=0;i<32;i++) {
				float e1,e0; /* %f -> float! */
				line[0]=0;
				fgets(line,255,fe);
				if(line[0]=='#')
					continue;
				sscanf(line,"%f %f",&e0,&e1);
				equalizer[0][i] = e0;
				equalizer[1][i] = e1;	
			}
			fclose(fe);
			have_eq_settings = TRUE;			
		}
		else
			fprintf(stderr,"Can't open equalizer file '%s'\n",equalfile);
	}

#ifdef HAVE_SETPRIORITY
	if(param.aggressive) { /* tst */
		int mypid = getpid();
		setpriority(PRIO_PROCESS,mypid,-20);
	}
#endif

#ifdef HAVE_SCHED_SETSCHEDULER
	if (param.realtime) {  /* Get real-time priority */
	  struct sched_param sp;
	  fprintf(stderr,"Getting real-time priority\n");
	  memset(&sp, 0, sizeof(struct sched_param));
	  sp.sched_priority = sched_get_priority_min(SCHED_FIFO);
	  if (sched_setscheduler(0, SCHED_RR, &sp) == -1)
	    fprintf(stderr,"Can't get real-time priority\n");
	}
#endif

	set_synth_functions(&fr);

	if(!param.remote) prepare_playlist(argc, argv);

	opt_make_decode_tables(outscale);
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3(fr.down_sample);

#if !defined(WIN32) && !defined(GENERIC)
	/* This ctrl+c for title skip only when not in some control mode */
	if
	(
		!param.remote 
		#ifdef HAVE_TERMIOS
		&& !param.term_ctrl
		#endif
	)
	catchsignal (SIGINT, catch_interrupt);

	if(param.remote) {
		int ret;
		init_id3();
		init_icy();
		ret = control_generic(&fr);
		clear_icy();
		exit_id3();
		safe_exit(ret);
	}
#endif

	init_icy();
	init_id3(); /* prepare id3 memory */
	while ((fname = get_next_file())) {
		char *dirname, *filename;
		long leftFrames,newFrame;

		if(!*fname || !strcmp(fname, "-"))
			fname = NULL;
               if (open_stream(fname,-1) < 0)
                       continue;
      
		if (!param.quiet) {
			if (split_dir_file(fname ? fname : "standard input",
				&dirname, &filename))
				fprintf(stderr, "\nDirectory: %s", dirname);
			fprintf(stderr, "\nPlaying MPEG stream %lu of %lu: %s ...\n", (unsigned long)pl.pos, (unsigned long)pl.fill, filename);

#if !defined(GENERIC)
{
	const char *term_type;
	term_type = getenv("TERM");
	if (term_type && param.xterm_title &&
	    (!strncmp(term_type,"xterm",5) || !strncmp(term_type,"rxvt",4)))
	{
		fprintf(stderr, "\033]0;%s\007", filename);
	}
}
#endif

		}

#if !defined(WIN32) && !defined(GENERIC)
#ifdef HAVE_TERMIOS
		if(!param.term_ctrl)
#endif
			gettimeofday (&start_time, NULL);
#endif
		read_frame_init(&fr);

		init = 1;
		#ifdef GAPLESS
		pre_init = 1;
		#endif
		newFrame = startFrame;
		
#ifdef HAVE_TERMIOS
		debug1("param.term_ctrl: %i", param.term_ctrl);
		if(param.term_ctrl)
			term_init();
#endif
		leftFrames = numframes;
		/* read_frame is counting the frames! */
		for(;read_frame(&fr) && leftFrames && !intflag;) {
#ifdef HAVE_TERMIOS			
tc_hack:
#endif
			if(fr.num < startFrame || (param.doublespeed && (fr.num % param.doublespeed))) {
				if(fr.lay == 3)
				{
					set_pointer(512);
					#ifdef GAPLESS
					if(param.gapless)
					{
						if(pre_init)
						{
							prepare_audioinfo(&fr, &pre_ao);
							pre_init = 0;
						}
						/* keep track... */
						layer3_gapless_set_position(fr.num, &fr, &pre_ao);
					}
					#endif
				}
				continue;
			}
			if(leftFrames > 0)
			  leftFrames--;
			if(!play_frame(init,&fr))
			{
				error("frame playback failed, skipping rest of track");
				break;
			}
			init = 0;

			if(param.verbose) {
#ifndef NOXFERMEM
				if (param.verbose > 1 || !(fr.num & 0x7))
					print_stat(&fr,fr.num,xfermem_get_usedspace(buffermem),ao); 
				if(param.verbose > 2 && param.usebuffer)
					fprintf(stderr,"[%08x %08x]",buffermem->readindex,buffermem->freeindex);
#else
				if (param.verbose > 1 || !(fr.num & 0x7))
					print_stat(&fr,fr.num,0,ao);
#endif
			}
#ifdef HAVE_TERMIOS
			if(!param.term_ctrl) {
				continue;
			} else {
				long offset;
				if((offset=term_control(&fr,ao))) {
					if(!rd->back_frame(rd, &fr, -offset)) {
						debug1("seeked to %lu", fr.num);
						#ifdef GAPLESS
						if(param.gapless && (fr.lay == 3))
						layer3_gapless_set_position(fr.num, &fr, ao);
						#endif
					} else { error("seek failed!"); }
				}
			}
#endif

		}
		#ifdef GAPLESS
		/* make sure that the correct padding is skipped after track ended */
		if(param.gapless) audio_flush(param.outmode, ao);
		#endif

#ifndef NOXFERMEM
	if(param.usebuffer) {
		int s;
		while ((s = xfermem_get_usedspace(buffermem))) {
			struct timeval wait170 = {0, 170000};

			buffer_ignore_lowmem();
			
			if(param.verbose)
				print_stat(&fr,fr.num,s,ao);
#ifdef HAVE_TERMIOS
			if(param.term_ctrl) {
				long offset;
				if((offset=term_control(&fr,ao))) {
					if((!rd->back_frame(rd, &fr, -offset)) 
						&& read_frame(&fr))
					{
						debug1("seeked to %lu", fr.num);
						#ifdef GAPLESS
						if(param.gapless && (fr.lay == 3))
						layer3_gapless_set_position(fr.num, &fr, ao);
						#endif
						goto tc_hack;	/* Doh! Gag me with a spoon! */
					} else { error("seek failed!"); }
				}
			}
#endif
			select(0, NULL, NULL, NULL, &wait170);
		}
	}
#endif
	if(param.verbose)
		print_stat(&fr,fr.num,xfermem_get_usedspace(buffermem),ao); 
#ifdef HAVE_TERMIOS
	if(param.term_ctrl)
		term_restore();
#endif

	if (!param.quiet) {
		/* 
		 * This formula seems to work at least for
		 * MPEG 1.0/2.0 layer 3 streams.
		 */
		int secs = get_songlen(&fr,fr.num);
		fprintf(stderr,"\n[%d:%02d] Decoding of %s finished.\n", secs / 60,
			secs % 60, filename);
	}

	rd->close(rd);
#if 0
	if(param.remote)
		fprintf(stderr,"@R MPG123\n");        
	if (remflag) {
		intflag = FALSE;
		remflag = FALSE;
	}
#endif
	
      if (intflag) {

/* 
 * When HAVE_TERMIOS is defined, there is 'q' to terminate a list of songs, so
 * no pressing need to keep up this first second SIGINT hack that was too
 * often mistaken as a bug. [dk]
 * ThOr: Yep, I deactivated the Ctrl+C hack for active control modes.
 */
#if !defined(WIN32) && !defined(GENERIC)
#ifdef HAVE_TERMIOS
	if(!param.term_ctrl)
#endif
        {
		gettimeofday (&now, NULL);
        	secdiff = (now.tv_sec - start_time.tv_sec) * 1000;
        	if (now.tv_usec >= start_time.tv_usec)
          		secdiff += (now.tv_usec - start_time.tv_usec) / 1000;
        	else
          		secdiff -= (start_time.tv_usec - now.tv_usec) / 1000;
        	if (secdiff < 1000)
          		break;
	}
#endif
        intflag = FALSE;

#ifndef NOXFERMEM
        if(param.usebuffer) buffer_resync();
#endif
      }
    } /* end of loop over input files */
    clear_icy();
    exit_id3(); /* free id3 memory */
#ifndef NOXFERMEM
    if (param.usebuffer) {
      buffer_end();
      xfermem_done_writer (buffermem);
      waitpid (buffer_pid, NULL, 0);
      xfermem_done (buffermem);
    }
    else {
#endif
      audio_flush(param.outmode, ao);
      free (pcm_sample);
#ifndef NOXFERMEM
    }
#endif

    switch(param.outmode) {
      case DECODE_AUDIO:
        ao->close(ao);
        break;
      case DECODE_WAV:
        wav_close();
        break;
      case DECODE_AU:
        au_close();
        break;
      case DECODE_CDR:
        cdr_close();
        break;
    }
   	if(!param.remote) free_playlist();
    return 0;
}

static void print_title(FILE *o)
{
	fprintf(o, "High Performance MPEG 1.0/2.0/2.5 Audio Player for Layers 1, 2 and 3\n");
	fprintf(o, "\tversion %s; written and copyright by Michael Hipp and others\n", PACKAGE_VERSION);
	fprintf(o, "\tfree software (LGPL/GPL) without any warranty but with best wishes\n");
}

static void usage(int err)  /* print syntax & exit */
{
	FILE* o = stdout;
	if(err)
	{
		o = stderr; 
		fprintf(o, "You made some mistake in program usage... let me briefly remind you:\n\n");
	}
	print_title(o);
	fprintf(o,"\nusage: %s [option(s)] [file(s) | URL(s) | -]\n", prgName);
	fprintf(o,"supported options [defaults in brackets]:\n");
	fprintf(o,"   -v    increase verbosity level       -q    quiet (don't print title)\n");
	fprintf(o,"   -t    testmode (no output)           -s    write to stdout\n");
	fprintf(o,"   -w <filename> write Output as WAV file\n");
	fprintf(o,"   -k n  skip first n frames [0]        -n n  decode only n frames [all]\n");
	fprintf(o,"   -c    check range violations         -y    DISABLE resync on errors\n");
	fprintf(o,"   -b n  output buffer: n Kbytes [0]    -f n  change scalefactor [32768]\n");
	fprintf(o,"   -r n  set/force samplerate [auto]    -g n  set audio hardware output gain\n");
	fprintf(o,"                                        -a d  set audio device\n");
	fprintf(o,"   -2    downsample 1:2 (22 kHz)        -4    downsample 1:4 (11 kHz)\n");
	fprintf(o,"   -d n  play every n'th frame only     -h n  play every frame n times\n");
	fprintf(o,"   -0    decode channel 0 (left) only   -1    decode channel 1 (right) only\n");
	fprintf(o,"   -m    mix both channels (mono)       -p p  use HTTP proxy p [$HTTP_PROXY]\n");
	#ifdef HAVE_SCHED_SETSCHEDULER
	fprintf(o,"   -@ f  read filenames/URLs from f     -T get realtime priority\n");
	#else
	fprintf(o,"   -@ f  read filenames/URLs from f\n");
	#endif
	fprintf(o,"   -z    shuffle play (with wildcards)  -Z    random play\n");
	fprintf(o,"   -u a  HTTP authentication string     -E f  Equalizer, data from file\n");
	#ifdef GAPLESS
	fprintf(o,"   -C    enable control keys            --gapless  skip junk/padding in some mp3s\n");
	#else
	fprintf(o,"   -C    enable control keys\n");
	#endif
	fprintf(o,"   -?    this help                      --version  print name + version\n");
	fprintf(o,"See the manpage %s(1) or call %s with --longhelp for more parameters and information.\n", prgName,prgName);
	safe_exit(err);
}

static void want_usage(char* arg)
{
	usage(0);
}

static void long_usage(int err)
{
	FILE* o = stdout;
	if(err)
	{
  	o = stderr; 
  	fprintf(o, "You made some mistake in program usage... let me remind you:\n\n");
	}
	print_title(o);
	fprintf(o,"\nusage: %s [option(s)] [file(s) | URL(s) | -]\n", prgName);

	fprintf(o,"\ninput options\n\n");
	fprintf(o," -k <n> --skip <n>         skip n frames at beginning\n");
	fprintf(o," -n     --frames <n>       play only <n> frames of every stream\n");
	fprintf(o," -y     --resync           DISABLES resync on error\n");
	fprintf(o," -p <f> --proxy <f>        set WWW proxy\n");
	fprintf(o," -u     --auth             set auth values for HTTP access\n");
	fprintf(o," -@ <f> --list <f>         play songs in playlist <f> (plain list, m3u, pls (shoutcast))\n");
	fprintf(o," -l <n> --listentry <n>    play nth title in playlist; show whole playlist for n < 0\n");
	fprintf(o," -z     --shuffle          shuffle song-list before playing\n");
	fprintf(o," -Z     --random           full random play\n");

	fprintf(o,"\noutput/processing options\n\n");
	fprintf(o," -o <o> --output <o>       select audio output module\n");
	fprintf(o," -a <d> --audiodevice <d>  select audio device\n");
	fprintf(o," -s     --stdout           write raw audio to stdout (host native format)\n");
	fprintf(o," -w <f> --wav <f>          write samples as WAV file in <f> (- is stdout)\n");
	fprintf(o,"        --au <f>           write samples as Sun AU file in <f> (- is stdout)\n");
	fprintf(o,"        --cdr <f>          write samples as CDR file in <f> (- is stdout)\n");
	fprintf(o,"        --reopen           force close/open on audiodevice\n");
	fprintf(o," -g     --gain             set audio hardware output gain\n");
	fprintf(o," -f <n> --scale <n>        scale output samples (soft gain, default=%li)\n", outscale);
	fprintf(o,"        --rva-mix,\n");
	fprintf(o,"        --rva-radio        use RVA2/ReplayGain values for mix/radio mode\n");
	fprintf(o,"        --rva-album,\n");
	fprintf(o,"        --rva-audiophile   use RVA2/ReplayGain values for album/audiophile mode\n");
	fprintf(o,"        --reopen           force close/open on audiodevice\n");
	fprintf(o," -0     --left --single0   play only left channel\n");
	fprintf(o," -1     --right --single1  play only right channel\n");
	fprintf(o," -m     --mono --mix       mix stereo to mono\n");
	fprintf(o,"        --stereo           duplicate mono channel\n");
	fprintf(o," -r     --rate             force a specific audio output rate\n");
	fprintf(o," -2     --2to1             2:1 downsampling\n");
	fprintf(o," -4     --4to1             4:1 downsampling\n");
	fprintf(o,"        --8bit             force 8 bit output\n");
	fprintf(o," -d     --doublespeed      play only every second frame\n");
	fprintf(o," -h     --halfspeed        play every frame twice\n");
	fprintf(o,"        --equalizer        exp.: scales freq. bands acrd. to 'equalizer.dat'\n");
	#ifdef GAPLESS
	fprintf(o,"        --gapless          remove padding/junk added by encoder/decoder\n");
	#endif
	fprintf(o,"                           (experimental, needs Lame tag, layer 3 only)\n");
	fprintf(o," -b <n> --buffer <n>       set play buffer (\"output cache\")\n");

	fprintf(o,"\nmisc options\n\n");
	fprintf(o," -t     --test             only decode, no output (benchmark)\n");
	fprintf(o," -c     --check            count and display clipped samples\n");
	fprintf(o," -v[*]  --verbose          increase verboselevel\n");
	fprintf(o," -q     --quiet            quiet mode\n");
	#ifdef HAVE_TERMIOS
	fprintf(o," -C     --control          enable terminal control keys\n");
	#endif
	#ifndef GENERIG
	fprintf(o,"        --title            set xterm/rxvt title to filename\n");
	#endif
	fprintf(o,"        --long-tag         spacy id3 display with every item on a separate line\n");
	fprintf(o," -R     --remote           generic remote interface\n");
	fprintf(o,"        --remote-err       use stderr for generic remote interface\n");
	#ifdef HAVE_SETPRIORITY
	fprintf(o,"        --aggressive       tries to get higher priority (nice)\n");
	#endif
	#ifdef HAVE_SCHED_SETSCHEDULER
	fprintf(o," -T     --realtime         tries to get realtime priority\n");
	#endif
	#ifdef OPT_MULTI
	fprintf(o,"        --cpu <string>     set cpu optimization\n");
	fprintf(o,"        --test-cpu         display result of CPU features autodetect and exit\n");
	fprintf(o,"        --list-cpu         list builtin optimizations and exit\n");
	#endif
	#ifdef OPT_3DNOW
	fprintf(o,"        --test-3dnow       display result of 3DNow! autodetect and exit\n");
	fprintf(o,"        --force-3dnow      force use of 3DNow! optimized routine\n");
	fprintf(o,"        --no-3dnow         force use of floating-pointer routine\n");
	#endif
	fprintf(o," -?     --help             give compact help\n");
	fprintf(o,"        --longhelp         give this long help listing\n");
	fprintf(o,"        --version          give name / version string\n");

	fprintf(o,"\nSee the manpage %s(1) for more information.\n", prgName);
	safe_exit(err);
}

static void want_long_usage(char* arg)
{
	long_usage(0);
}

static void give_version(char* arg)
{
	fprintf(stdout, PACKAGE_NAME" "PACKAGE_VERSION"\n");
	safe_exit(0);
}

/*
	common: misc stuff... audio flush, status display...

	copyright ?-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <fcntl.h>

#include "mpg123.h"
#include "common.h"

#ifdef WIN32
#include <winsock.h>
#endif

const char* rva_name[3] = { "off", "mix", "album" };

#define RESYNC_LIMIT 1024

void audio_flush(struct frame *fr, int outmode, struct audio_info_struct *ai)
{
	#ifdef GAPLESS
	if(param.gapless) frame_gapless_buffercheck(fr);
	#endif
	if(fr->buffer.fill)
	{
		switch(outmode)
		{
			case DECODE_FILE:
				write (OutputDescriptor, fr->buffer.data, fr->buffer.fill);
			break;
			case DECODE_AUDIO:
				audio_play_samples (ai, fr->buffer.data, fr->buffer.fill);
			break;
			case DECODE_BUFFER:
				write (buffer_fd[1], fr->buffer.data, fr->buffer.fill);
			break;
			case DECODE_WAV:
			case DECODE_CDR:
			case DECODE_AU:
				wav_write(fr->buffer.data, fr->buffer.fill);
			break;
		}
		fr->buffer.fill = 0;
	}
}

#if !defined(WIN32) && !defined(GENERIC)
void (*catchsignal(int signum, void(*handler)()))()
{
  struct sigaction new_sa;
  struct sigaction old_sa;

#ifdef DONT_CATCH_SIGNALS
  fprintf (stderr, "Not catching any signals.\n");
  return ((void (*)()) -1);
#endif

  new_sa.sa_handler = handler;
  sigemptyset(&new_sa.sa_mask);
  new_sa.sa_flags = 0;
  if (sigaction(signum, &new_sa, &old_sa) == -1)
    return ((void (*)()) -1);
  return (old_sa.sa_handler);
}
#endif

/* concurring to print_rheader... here for control_generic */
const char* remote_header_help = "S <mpeg-version> <layer> <sampling freq> <mode(stereo/mono/...)> <mode_ext> <framesize> <stereo> <copyright> <error_protected> <emphasis> <bitrate> <extension> <vbr(0/1=yes/no)>";
void make_remote_header(struct frame* fr, char *target)
{
	/* redundancy */
	static char *modes[4] = {"Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel"};
	snprintf(target, 1000, "S %s %d %ld %s %d %d %d %d %d %d %d %d %d",
		fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
		fr->lay,
		frame_freq(fr),
		modes[fr->mode],
		fr->mode_ext,
		fr->framesize+4,
		fr->stereo,
		fr->copyright ? 1 : 0,
		fr->error_protection ? 1 : 0,
		fr->emphasis,
		frame_bitrate(fr),
		fr->extension,
		fr->vbr);
}


#ifdef MPG123_REMOTE
void print_rheader(struct frame *fr)
{
	static char *modes[4] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel" };
	static char *layers[4] = { "Unknown" , "I", "II", "III" };
	static char *mpeg_type[2] = { "1.0" , "2.0" };

	/* version, layer, freq, mode, channels, bitrate, BPF, VBR*/
	fprintf(stderr,"@I %s %s %ld %s %d %d %d %i\n",
			mpeg_type[fr->lsf],layers[fr->lay], frame_freq(fr),
			modes[fr->mode],fr->stereo,
			fr->vbr == ABR ? fr->abr_rate : frame_bitrate(fr),
			fr->framesize+4,
			fr->vbr);
}
#endif

void print_header(struct frame *fr)
{
	static char *modes[4] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel" };
	static char *layers[4] = { "Unknown" , "I", "II", "III" };

	fprintf(stderr,"MPEG %s, Layer: %s, Freq: %ld, mode: %s, modext: %d, BPF : %d\n", 
		fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
		layers[fr->lay], frame_freq(fr),
		modes[fr->mode],fr->mode_ext,fr->framesize+4);
	fprintf(stderr,"Channels: %d, copyright: %s, original: %s, CRC: %s, emphasis: %d.\n",
		fr->stereo,fr->copyright?"Yes":"No",
		fr->original?"Yes":"No",fr->error_protection?"Yes":"No",
		fr->emphasis);
	fprintf(stderr,"Bitrate: ");
	switch(fr->vbr)
	{
		case CBR: fprintf(stderr, "%d kbit/s", frame_bitrate(fr)); break;
		case VBR: fprintf(stderr, "VBR"); break;
		case ABR: fprintf(stderr, "%d kbit/s ABR", fr->abr_rate); break;
		default: fprintf(stderr, "???");
	}
	fprintf(stderr, " Extension value: %d\n",	fr->extension);
}

void print_header_compact(struct frame *fr)
{
	static char *modes[4] = { "stereo", "joint-stereo", "dual-channel", "mono" };
	static char *layers[4] = { "Unknown" , "I", "II", "III" };
	
	fprintf(stderr,"MPEG %s layer %s, ",
		fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
		layers[fr->lay]);
	switch(fr->vbr)
	{
		case CBR: fprintf(stderr, "%d kbit/s", frame_bitrate(fr)); break;
		case VBR: fprintf(stderr, "VBR"); break;
		case ABR: fprintf(stderr, "%d kbit/s ABR", fr->abr_rate); break;
		default: fprintf(stderr, "???");
	}
	fprintf(stderr,", %ld Hz %s\n",
		frame_freq(fr), modes[fr->mode]);
}

#if 0
/* removed the strndup for better portability */
/*
 *   Allocate space for a new string containing the first
 *   "num" characters of "src".  The resulting string is
 *   always zero-terminated.  Returns NULL if malloc fails.
 */
char *strndup (const char *src, int num)
{
	char *dst;

	if (!(dst = (char *) malloc(num+1)))
		return (NULL);
	dst[num] = '\0';
	return (strncpy(dst, src, num));
}
#endif

/*
 *   Split "path" into directory and filename components.
 *
 *   Return value is 0 if no directory was specified (i.e.
 *   "path" does not contain a '/'), OR if the directory
 *   is the same as on the previous call to this function.
 *
 *   Return value is 1 if a directory was specified AND it
 *   is different from the previous one (if any).
 */

int split_dir_file (const char *path, char **dname, char **fname)
{
	static char *lastdir = NULL;
	char *slashpos;

	if ((slashpos = strrchr(path, '/'))) {
		*fname = slashpos + 1;
		*dname = strdup(path); /* , 1 + slashpos - path); */
		if(!(*dname)) {
			perror("failed to allocate memory for dir name");
			return 0;
		}
		(*dname)[1 + slashpos - path] = 0;
		if (lastdir && !strcmp(lastdir, *dname)) {
			/***   same as previous directory   ***/
			free (*dname);
			*dname = lastdir;
			return 0;
		}
		else {
			/***   different directory   ***/
			if (lastdir)
				free (lastdir);
			lastdir = *dname;
			return 1;
		}
	}
	else {
		/***   no directory specified   ***/
		if (lastdir) {
			free (lastdir);
			lastdir = NULL;
		};
		*dname = NULL;
		*fname = (char *)path;
		return 0;
	}
}

/*
 * Returns number of frames queued up in output buffer, i.e. 
 * offset between currently played and currently decoded frame.
 */

long compute_buffer_offset(struct frame *fr)
{
	long bufsize;
	
	/*
	 * buffermem->buf[0] holds output sampling rate,
	 * buffermem->buf[1] holds number of channels,
	 * buffermem->buf[2] holds audio format of output.
	 */
	
	if(!param.usebuffer || !(bufsize=xfermem_get_usedspace(buffermem))
		|| !buffermem->buf[0] || !buffermem->buf[1])
		return 0;

	bufsize = (long)((double) bufsize / buffermem->buf[0] / 
			buffermem->buf[1] / compute_tpf(fr));
	
	if((buffermem->buf[2] & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_16)
		return bufsize/2;
	else
		return bufsize;
}

unsigned int roundui(double val)
{
	double base = floor(val);
	return (unsigned int) ((val-base) < 0.5 ? base : base + 1 );
}

void print_stat(struct frame *fr,unsigned long no,long buffsize,struct audio_info_struct *ai)
{
	double tim1,tim2;
	unsigned long rno;
	if(!position_info(fr, no, buffsize, ai, &rno, &tim1, &tim2))
	{
		/* All these sprintf... only to avoid two writes to stderr in case of using buffer?
		   I guess we can drop that. */
		fprintf(stderr, "\rFrame# %5lu [%5lu], Time: %02lu:%02u.%02u [%02u:%02u.%02u], RVA:%6s, Vol: %3u(%3u)",
		        no,rno,
		        (unsigned long) tim1/60, (unsigned int)tim1%60, (unsigned int)(tim1*100)%100,
		        (unsigned int)tim2/60, (unsigned int)tim2%60, (unsigned int)(tim2*100)%100,
		        rva_name[param.rva], roundui((double)fr->outscale/MAXOUTBURST*100), roundui((double)fr->lastscale/MAXOUTBURST*100) );
		if(param.usebuffer) fprintf(stderr,", [%8ld] ",(long)buffsize);
	}
	if(fr->icy.changed && fr->icy.data)
	{
		fprintf(stderr, "\nICY-META: %s\n", fr->icy.data);
		fr->icy.changed = 0;
	}
}

void clear_stat()
{
	fprintf(stderr, "\r                                                                                       \r");
}

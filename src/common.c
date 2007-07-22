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

void audio_flush(int outmode, unsigned char *bytes, size_t count, struct audio_info_struct *ai)
{
	switch(outmode)
	{
		case DECODE_AUDIO:
			audio_play_samples (ai, bytes, count);
		break;
		case DECODE_FILE:
			write (OutputDescriptor, bytes, count);
		break;
		case DECODE_WAV:
		case DECODE_CDR:
		case DECODE_AU:
			wav_write(bytes, count);
		break;
		default: error1("What to do for outmode %i?", outmode);
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

unsigned int roundui(double val)
{
	double base = floor(val);
	return (unsigned int) ((val-base) < 0.5 ? base : base + 1 );
}

void print_stat(mpg123_handle *fr, long offset, long buffsize)
{
	double tim1,tim2;
	long rno, no;
	double basevol, realvol;
	char *icy;
#ifndef GENERIC
/* Only generate new stat line when stderr is ready... don't overfill... */
	{
		struct timeval t;
		fd_set serr;
		int n,errfd = fileno(stderr);

		t.tv_sec=t.tv_usec=0;

		FD_ZERO(&serr);
		FD_SET(errfd,&serr);
		n = select(errfd+1,NULL,&serr,NULL,&t);
		if(n <= 0) return;
	}
#endif
	if(    MPG123_OK == mpg123_position(fr, offset, buffsize, &no, &rno, &tim1, &tim2)
	    && MPG123_OK == mpg123_getvolume(fr, &basevol, &realvol, NULL) )
	{
		fprintf(stderr, "\rFrame# %5li [%5li], Time: %02lu:%02u.%02u [%02u:%02u.%02u], RVA:%6s, Vol: %3u(%3u)",
		        no,rno,
		        (unsigned long) tim1/60, (unsigned int)tim1%60, (unsigned int)(tim1*100)%100,
		        (unsigned int)tim2/60, (unsigned int)tim2%60, (unsigned int)(tim2*100)%100,
		        rva_name[param.rva], roundui(basevol*100), roundui(realvol*100) );
		if(param.usebuffer) fprintf(stderr,", [%8ld] ",(long)buffsize);
	}
	/* Check for changed tags here too? */
	if( mpg123_meta_check(fr) | MPG123_NEW_ICY && MPG123_OK == mpg123_icy(fr, &icy) )
	fprintf(stderr, "\nICY-META: %s\n", icy);
}

void clear_stat()
{
	fprintf(stderr, "\r                                                                                       \r");
}

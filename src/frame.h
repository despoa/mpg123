#ifndef MPG123_FRAME_H
#define MPG123_FRAME_H

#include "id3.h"

/* max = 1728 */
#define MAXFRAMESIZE 3456

struct al_table 
{
  short bits;
  short d;
};

struct frame_index
{
	off_t data[INDEX_SIZE];
	size_t fill;
	unsigned long step;
};

/* the output buffer, used to be pcm_sample, pcm_point and audiobufsize */
struct outbuffer
{
	unsigned char *data;
	int fill;
	int size; /* that's actually more like a safe size, after we have more than that, flush it */
};

struct frame
{
	int verbose;    /* 0: nothing, 1: just print chosen decoder, 2: be verbose */

	/* struct frame */
	const struct al_table *alloc;
	/* could use types from optimize.h */
	int (*synth)(real *,int,unsigned char *,int *);
	int (*synth_mono)(real *,unsigned char *,int *);
	int stereo; /* I _think_ 1 for mono and 2 for stereo */
	int jsbound;
	int single;
	int II_sblimit;
	int down_sample_sblimit;
	int lsf; /* 0: MPEG 1.0; 1: MPEG 2.0/2.5 -- both used as bool and array index! */
	int mpeg25;
	int down_sample;
	int header_change;
	int lay;
	int (*do_layer)(struct frame *fr,int,struct audio_info_struct *);
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int framesize; /* computed framesize */
#define CBR 0
#define VBR 1
#define ABR 2
	int vbr; /* 1 if variable bitrate was detected */
	unsigned long num; /* the nth frame in some stream... */

	/* bitstream info; bsi */
	int bitindex;
	unsigned char *wordpointer;
	/* temporary storage for getbits stuff */
	unsigned long ultmp;
	unsigned char uctmp;

	/* rva data, used in common.c, set in id3.c */
	
	struct
	{
		scale_t lastscale;
		scale_t outscale;
		int level[2];
		float gain[2];
		float peak[2];
	} rva;

	int do_recover;

	/* input data */
	unsigned long track_frames;
	double mean_framesize;
	unsigned long mean_frames;
	int fsizeold;
	int ssize;
	unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
	unsigned char *bsbuf;
	unsigned char *bsbufold;
	int bsnum;
	unsigned long oldhead;
	unsigned long firsthead;
	int abr_rate;
	struct frame_index index;

	/* output data */
	struct outbuffer buffer;
#ifdef GAPLESS
	unsigned long position; /* position in raw decoder bytestream */
	unsigned long begin; /* first byte to play == number to skip */
	unsigned long end; /* last byte to play */
	unsigned long ignore; /* forcedly ignore stuff in between */
	int bytified;
#endif
	unsigned int crc;
	struct taginfo tag;
};

void frame_invalid(struct frame *fr);
int frame_buffer(struct frame *fr, int s);
int frame_init(struct frame* fr);
void frame_clear(struct frame *fr);
void print_frame_index(struct frame *fr, FILE* out);
off_t frame_index_find(struct frame *fr, unsigned long want_frame, unsigned long* get_frame);

#ifdef GAPLESS
unsigned long samples_to_bytes(struct frame *fr , unsigned long s, struct audio_info_struct* ai);
/* samples per frame ...
Layer I
Layer II
Layer III
MPEG-1
384
1152
1152
MPEG-2 LSF
384
1152
576
MPEG 2.5
384
1152
576
*/
#define spf(fr) (fr->lay == 1 ? 384 : (fr->lay==2 ? 1152 : (fr->lsf || fr->mpeg25 ? 576 : 1152)))
/* still fine-tuning the "real music" window... see read_frame */
#define GAP_SHIFT -1
void frame_gapless_init(struct frame *fr, unsigned long b, unsigned long e);
void frame_gapless_position(struct frame* fr, unsigned long frames, struct audio_info_struct *ai);
void frame_gapless_bytify(struct frame *fr, struct audio_info_struct *ai);
void frame_gapless_ignore(struct frame *fr, unsigned long frames, struct audio_info_struct *ai);
void frame_gapless_buffercheck(struct frame *fr);
#endif

#endif

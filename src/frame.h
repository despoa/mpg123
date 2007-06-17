#ifndef MPG123_FRAME_H
#define MPG123_FRAME_H

#include "id3.h"

/* max = 1728 */
#define MAXFRAMESIZE 3456

/* need the definite optimization flags here */

#ifdef OPT_I486
#define OPT_I386
#define FIR_BUFFER_SIZE  128
#define FIR_SIZE 16
#endif

#ifdef OPT_I386
#define PENTIUM_FALLBACK
#define OPT_X86
#endif

#ifdef OPT_I586
#define PENTIUM_FALLBACK
#define OPT_PENTIUM
#define OPT_X86
#endif

#ifdef OPT_I586_DITHER
#define PENTIUM_FALLBACK
#define OPT_PENTIUM
#define OPT_X86
#endif

#ifdef OPT_MMX
#define OPT_MMXORSSE
#define OPT_X86
#ifndef OPT_MULTI
#define OPT_MMX_ONLY
#endif
#endif

#ifdef OPT_SSE
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#define OPT_MMX_ONLY
#endif
#endif

#ifdef OPT_3DNOWEXT
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#define OPT_MMX_ONLY
#endif
#endif

#ifdef OPT_3DNOW
#define K6_FALLBACK
#define OPT_X86
#endif

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

enum optdec { nodec=0, generic, idrei, ivier, ifuenf, ifuenf_dither, mmx, dreidnow, dreidnowext, altivec, sse };
enum optcla { nocla=0, normal, mmxsse };

struct frame
{
	real hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	/* the scratch vars for the decoders, sometimes real, sometimes short... sometimes int/long */ 
	short *short_buffs[2][2];
	real *real_buffs[2][2];
	unsigned char *rawbuffs;
	int rawbuffss;
	int bo[2]; /* i486 and dither need a second value */
	unsigned char* rawdecwin; /* the block with all decwins */
	real *decwin; /* _the_ decode table */
#ifdef OPT_MMXORSSE
	/* I am not really sure that I need both of them... used in assembler */
	float *decwin_mmx;
	float *decwins;
#endif
	int have_eq_settings;
	real equalizer[2][32];

	/* a raw buffer and a pointer into the middle for signed short conversion, only allocated on demand */
	unsigned char *conv16to8_buf;
	unsigned char *conv16to8;

	/* There's some possible memory saving for stuff that is not _really_ dynamic. */

	/* layer3 */
	int longLimit[9][23];
	int shortLimit[9][14];
	real gainpow2[256+118+4]; /* not really dynamic, just different for mmx */

	/* layer2 */
	real muls[27][64];	/* also used by layer 1 */

	/* decode_ntom */
	unsigned long ntom_val[2];
	unsigned long ntom_step;

	/* special i486 fun */
#ifdef OPT_I486
	int *int_buffs[2][2];
#endif
	/* special altivec... */
#ifdef OPT_ALTIVEC
	real *areal_buffs[4][4];
#endif
	struct
	{
#ifdef OPT_MULTI
		int (*synth_1to1)(real *,int, struct frame *,int );
		int (*synth_1to1_mono)(real *, struct frame *);
		int (*synth_1to1_mono2stereo)(real *, struct frame *);
		int (*synth_1to1_8bit)(real *,int, struct frame *,int );
		int (*synth_1to1_8bit_mono)(real *, struct frame *);
		int (*synth_1to1_8bit_mono2stereo)(real *, struct frame *);
#ifdef OPT_PENTIUM
		int (*synth_1to1_i586_asm)(real *,int,unsigned char *, unsigned char *, int *, real *decwin);
#endif
#ifdef OPT_MMXORSSE
		void (*make_decode_tables)(struct frame *fr);
		real (*init_layer3_gainpow2)(int);
		real* (*init_layer2_table)(real*, double);
#endif
#ifdef OPT_3DNOW
		void (*dct36)(real *,real *,real *,real *,real *);
#endif
		void (*dct64)(real *,real *,real *);
#ifdef OPT_MPLAYER
		void (*mpl_dct64)(real *,real *,real *);
#endif
#endif
		enum optdec type;
		enum optcla class;
	} cpu_opts;

	int verbose;    /* 0: nothing, 1: just print chosen decoder, 2: be verbose */

	/* struct frame */
	const struct al_table *alloc;
	/* could use types from optimize.h */
	int (*synth)(real *,int, struct frame*, int);
	int (*synth_mono)(real *, struct frame*);
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

void frame_preinit(struct frame *fr);
int frame_outbuffer(struct frame *fr, int s);
int frame_buffers(struct frame *fr);
int frame_init(struct frame* fr);
void frame_clear(struct frame *fr);
void print_frame_index(struct frame *fr, FILE* out);
off_t frame_index_find(struct frame *fr, unsigned long want_frame, unsigned long* get_frame);
int frame_cpu_opt(struct frame *fr);
int set_synth_functions(struct frame *fr, struct audio_info_struct *ai);

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

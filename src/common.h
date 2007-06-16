/*
	common: anything can happen here... frame reading, output, messages

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#ifndef _MPG123_COMMON_H_
#define _MPG123_COMMON_H_

/*
	AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
	A: sync
	B: mpeg version
	C: layer
	D: CRC
	E: bitrate
	F:sampling rate
	G: padding
	H: private
	I: channel mode
	J: mode ext
	K: copyright
	L: original
	M: emphasis

	old compare mask 0xfffffd00:
	11111111 11111111 11111101 00000000

	means: everything must match excluding padding and channel mode, ext mode, ...
	But a vbr stream's headers will differ in bitrate!
	We are already strict in allowing only frames of same type in stream, we should at least watch out for VBR while being strict.

	So a better mask is:
	11111111 11111111 00001101 00000000

	Even more, I'll allow varying crc bit.
	11111111 11111110 00001101 00000000

	(still unsure about this private bit)
*/
#define HDRCMPMASK 0xfffe0d00

extern double compute_tpf(struct frame *fr);
extern double compute_bpf(struct frame *fr);
extern long compute_buffer_offset(struct frame *fr);

/* well, I take that one for granted... at least layer3 */
#define DECODER_DELAY 529


/* for control_generic */
extern const char* remote_header_help;
void make_remote_header(struct frame* fr, char *target);

int position_info(struct frame* fr, unsigned long no, long buffsize, struct audio_info_struct* ai,
                   unsigned long* frames_left, double* current_seconds, double* seconds_left);

int read_frame_recover(struct frame* fr);

#endif

void print_stat(struct frame *fr,unsigned long no,long buffsize,struct audio_info_struct *ai);
void clear_stat();

/* adjust volume to current outscale and rva values if wanted */
#define RVA_OFF 0
#define RVA_MIX 1
#define RVA_ALBUM 2
#define RVA_MAX RVA_ALBUM
extern const char* rva_name[3];
void do_rva(struct frame *fr);
/* wrap over do_rva that prepares outscale */
void do_volume(struct frame *fr, double factor);

/* positive and negative for offsets... I guess I'll drop the unsigned frame position type anyway */
long time_to_frame(struct frame *fr, double seconds);

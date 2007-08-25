/*
	common: anything can happen here... frame reading, output, messages

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#ifndef _MPG123_COMMON_H_
#define _MPG123_COMMON_H_

#include "libmpg123.h"

void audio_flush(int outmode, unsigned char *bytes, size_t count, struct audio_info_struct *ai);
void (*catchsignal(int signum, void(*handler)()))();

void print_header(mpg123_handle *);
void print_header_compact(mpg123_handle *);
void print_stat(mpg123_handle *fr, long offset, long buffsize);
void clear_stat();
/* for control_generic */
extern const char* remote_header_help;
void make_remote_header(mpg123_handle *, char *target);

int split_dir_file(const char *path, char **dname, char **fname);

extern const char* rva_name[3];
#endif


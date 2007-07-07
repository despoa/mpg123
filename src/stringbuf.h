/*
	stringbuf: mimicking a bit of C++ to more safely handle strings

	copyright 2006-7 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/
#ifndef MPG123_STRINGBUF_H
#define MPG123_STRINGBUF_H

#include "mpg123lib.h" /* the mpg123_string */

void init_stringbuf(mpg123_string* sb);
void free_stringbuf(mpg123_string* sb);
/* returning 0 on error, 1 on success */
int resize_stringbuf(mpg123_string* sb, size_t new);
int copy_stringbuf  (mpg123_string* from, mpg123_string* to);
int add_to_stringbuf(mpg123_string* sb, char* stuff);
int set_stringbuf   (mpg123_string* sb, char* stuff);

#endif

/*
	icy: support for SHOUTcast ICY meta info, an attempt to keep it organized

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis and modelled after patch by Honza
*/
#ifndef MPG123_ICY_H
#define MPG123_ICY_H

#include <sys/types.h>
#include "stringbuf.h"

struct icy_meta
{
	struct stringbuf name;
	struct stringbuf url;
	char* data;
	off_t interval;
	off_t next;
	int changed;
};

void init_icy(struct icy_meta *);
void clear_icy(struct icy_meta *);
void reset_icy(struct icy_meta *);

#endif

#ifndef MPG123_H_HTTP
#define MPG123_H_HTTP

/* ------ Declarations from "httpget.c" ------ */
#include "mpg123lib.h"

struct httpdata
{
	mpg123_string content_type;
	mpg123_string icy_name;
	mpg123_string icy_url;
	off_t icy_interval;
	char *proxyurl;
	unsigned long proxyip;
};

void httpdata_init(struct httpdata *e);
void httpdata_reset(struct httpdata *e);

/* takes url and content type string address, opens resource, returns fd for data, allocates and sets content type */
extern int http_open (char* url, struct httpdata *hd);
extern char *httpauth;

#endif

#ifndef MPG123_H_HTTP
#define MPG123_H_HTTP

/* ------ Declarations from "httpget.c" ------ */

extern char *proxyurl;
extern unsigned long proxyip;
/* takes url and content type string address, opens resource, returns fd for data, allocates and sets content type */
extern int http_open (struct frame *fr, char* url, char** content_type);
extern char *httpauth;

#endif

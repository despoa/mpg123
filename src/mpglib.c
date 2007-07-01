#include "mpg123lib.h"
#include <unistd.h>
#include <stdio.h>

#define INBUFF 16777216

unsigned char buf[INBUFF];

int main(void)
{
	int size;
	unsigned char out[8192];
	long len;
	int ret;
	long in = 0, outc = 0;
	mpg123_handle *m;

	mpg123_init();
	m = mpg123_new();
	mpg123_param(m, MPG123_VERBOSE, 2);
	/* mpg123_param(m, MPG123_ADD_FLAGS, MPG123_GAPLESS); */
	mpg123_param(m, MPG123_START_FRAME, 2300);
	mpg123_open_feed(m);
	if(m == NULL) return -1;
	while(1) {
		len = read(0,buf,INBUFF);
		if(len <= 0)
			break;
		in += len;
		fprintf(stderr, ">> %li KiB in\n", in>>10);
		ret = mpg123_decode(m,buf,len,out,8192,&size);
		write(1,out,size);
		fprintf(stderr, "<< size=%i out, ret=%i\n", size, ret);
		while(ret == MPG123_OK || ret == MPG123_NEW_FORMAT) {
			outc += size;
			ret = mpg123_decode(m,NULL,0,out,8192,&size);
			write(1,out,size);
			fprintf(stderr, "<< %li B out, ret=%i\n", outc, ret);
		}
		if(ret != MPG123_NEED_MORE) break;
	}
	mpg123_delete(m);
	mpg123_exit();
	return 0;
}


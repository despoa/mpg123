#include "mpg123lib.h"
#include "unistd.h"

unsigned char buf[16384];

int main(void)
{
	int size;
	unsigned char out[8192];
	int len,ret;
	mpg123_handle *m;

	mpg123_init();
	m = mpg123_new();

	while(1) {
		len = read(0,buf,16384);
		if(len <= 0)
			break;
		ret = mpg123_decode(m,buf,len,out,8192,&size);
		while(ret == MPG123_OK) {
			write(1,out,size);
			ret = mpg123_decode(m,NULL,0,out,8192,&size);
		}
	}
	mpg123_delete(m);
	mpg123_exit();
	return 0;
}


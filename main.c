
#include "mpg123.h"
#include "mpglib.h"

char buf[16384];
struct mpstr mp;

int main(void)
{
	int size;
	real out[8192/2];
	int len,ret;
	size_t count = 0;

	InitMP3(&mp);

	while(1) {
		len = read(0,buf,16384);
		if(len <= 0)
			break;
		ret = decodeMP3(&mp,buf,len,out,8192/2,&size);
		while(ret == MP3_OK) {
			int k;
			fprintf(stderr, "got %i numbers\n", size);
			if(++count > 20) return 0;
			for(k = 0; k < size; ++k)
			{
				printf("%g\n", out[k]); 
			}
			/* write(1,out,sizeof(real)*size);*/
			ret = decodeMP3(&mp,NULL,0,out,8192/2,&size);
		}
	}
	return 0;
}


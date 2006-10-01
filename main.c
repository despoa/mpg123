// This is a simple test to work on SHARC 21369, using EZ-KIT-LITE
//
// note that the file IO is super slow
// A lot of SHARC optimization needs to be done, since this is basically still the bare generic mpglib here...


#include "mpg123.h"
#include "mpglib.h"
#include <stdlib.h>

extern void initPLL_SDRAM(void);    //Initialize PLL and set up AMI and SDRAM controller

extern int sdram_heap_start;
#define SDRAM_HEAP_START_DM &sdram_heap_start
#define SDRAM_HEAP_ID 3

#ifdef SHARC
#define SHARC_SHIFT 2
#else
#define SHARC_SHIFT 0
#endif

#define MP3_BUFFERIN_LEN_WORDS (256)
#define MP3_BUFFERIN_LEN_BYTES (MP3_BUFFERIN_LEN_WORDS<<SHARC_SHIFT)

//char SDRAM songBuffer[3*1024*1024];
char SDRAM buf[MP3_BUFFERIN_LEN_BYTES];
real SDRAM out[8192/2];
struct mpstr mp;

static unsigned char* _vMP3Song;


int main(void)
{
  FILE* fp;
  int size;
  int len,ret;
  int iIndexMP3 = 0;
  int iMP3Size;
  int iTotalDecode = 0;

#ifdef SHARC
  initPLL_SDRAM();
#endif

  fp = fopen("../song.mp3","rb");
  if (!fp)
    {
      printf("File not found\r\n");
      return 1;
    }
  
  fseek(fp,0,SEEK_END);
  iMP3Size = ftell(fp) << SHARC_SHIFT;
  printf("File Size = %d bytes\r\n",iMP3Size);
  fseek(fp,0,SEEK_SET);

#ifdef SHARC
  // Install a big 3*4 = 12 MByte heap in SDRAM, SDRAM size is 16 MByte on the EZ-Kit-Lite
  if (heap_install((void*)SDRAM_HEAP_START_DM,1024*1024*3,SDRAM_HEAP_ID,-1) == -1)
    {
      printf("Error installing heap in SDRAM\r\n");
      return 1;
    }

  _vMP3Song = heap_calloc(SDRAM_HEAP_ID, iMP3Size, sizeof(char));
#else
  _vMP3Song = (unsigned char*)malloc(iMP3Size);
#endif

  if (_vMP3Song == NULL)
    {
      printf("Can't allocate %d 32bits words in SDRAM heap\r\n",iMP3Size>>2);
      return 1;
    }

  fread(_vMP3Song, iMP3Size, sizeof(char), fp);
  fclose(fp);

  
  fp = fopen("result.raw","wb");
  if (!fp)
    {
      printf("Can't create file\r\n");
      return 1;
    }

  InitMP3(&mp);

  len = MP3_BUFFERIN_LEN_BYTES;
  printf("Done reading\r\n");

  while(1) 
    {
      int i;

      if (len < MP3_BUFFERIN_LEN_BYTES)
        break;

      if (((iIndexMP3+MP3_BUFFERIN_LEN_WORDS)<<SHARC_SHIFT) >= iMP3Size)
        len = iMP3Size - (iIndexMP3<<SHARC_SHIFT);

      if (!len)
        break;

      for (i = 0; i < len; iIndexMP3++)
        {
          buf[i++] = (_vMP3Song[iIndexMP3] >> 0 )&0xFF;
#ifdef SHARC
          buf[i++] = (_vMP3Song[iIndexMP3] >> 8 )&0xFF;
          buf[i++] = (_vMP3Song[iIndexMP3] >> 16)&0xFF;
          buf[i++] = (_vMP3Song[iIndexMP3] >> 24)&0xFF;
#endif
        }

      ret = decodeMP3(&mp,buf,len,out,8192/2,&size);
      
      while(ret == MP3_OK) 
        {
          for (ret = 0; ret < size; ret++)
            ((short*)out)[ret] = (short)out[ret];

          fwrite(out,sizeof(short),size,fp);

          iTotalDecode += size;

          ret = decodeMP3(&mp,NULL,0,out,8192/2,&size);
        }
    }

  printf("Done Decoding %d samples\r\n",iTotalDecode);
  fclose(fp);

  return 0;
}


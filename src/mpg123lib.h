

/* not decided... how anonymous should the handle be? */
struct frame;
typedef struct frame mpg123_handle;

mpg123_handle *mpg123_new()

/* replacement for mpglib's decodeMP3, same usage */
#define MP3_ERR -1
#define MP3_OK  0
#define MP3_NEED_MORE 1
#define decodeMP3(a,b,c,d,e,f) mpg123_decode((a),(b),(c),(d),(e),(f))
int mpg123_decode(mpglib_handle *mh,char *inmemory,int inmemsize, char *outmemory,int outmemsize,int *done);

/* cleanup, free memory */
void mpg123_delete(mpglib_handle *mh);


/* missing various functions to change properties: RVA, equalizer */
/* also: functions to access properties: ID3, RVA, ... */

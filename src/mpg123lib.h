
#define MPG123_FORMAT_MASK 0x100
#define MPG123_FORMAT_16   0x100
#define MPG123_FORMAT_8    0x000

#define MPG123_FORMAT_SIGNED_16    0x110
#define MPG123_FORMAT_UNSIGNED_16  0x120
#define MPG123_FORMAT_UNSIGNED_8   0x1
#define MPG123_FORMAT_SIGNED_8     0x2
#define MPG123_FORMAT_ULAW_8       0x4
#define MPG123_FORMAT_ALAW_8       0x8

/* not decided... how anonymous should the handle be? */
struct frame;
typedef struct frame mpg123_handle;

/* non-threadsafe init/exit, call _once_ */
void mpg123_init(void);
void mpg123_exit(void);
/* threadsafely create and delete handles */
mpg123_handle* mpg123_new(void)
void mpg123_delete(mpg123_handle *mh);

/* set/get output parameters, default 16bit stereo */
int  mpg123_output    (mpg123_handle *mh, int  format, int  channels);
int  mpg123_get_output(mpg123_handle *mh, int *format, int *channels);

int mpg123_open  (mpg123_handle *mh, char *url);
int mpg123_fdopen(mpg123_handle *mh, int fd);

void mpg123_close(mpg123_handle *mh);


/* replacement for mpglib's decodeMP3, same usage */
#define MPG123_ERR -1
#define MPG123_OK  0
#define MPG123_NEED_MORE 1
int mpg123_decode(mpg123_handle *mh,char *inmemory,int inmemsize, char *outmemory,int outmemsize,int *done);
void mpg123_track(mpg123_handle *mh); /* clear out buffers/state for track change */



/* missing various functions to change properties: RVA, equalizer */
/* also: functions to access properties: ID3, RVA, ... */

/*
	pinknoise: pink noise generator for libsyn123

	based on:
	patest_pink.c

	Generate Pink Noise using Gardner method.
	Optimization suggested by James McCartney uses a tree
	to select which random value to replace.

	    x x x x x x x x x x x x x x x x 
	     x   x   x   x   x   x   x   x   
	       x       x       x       x       
	           x               x               
	                   x                               

	Tree is generated by counting trailing zeros in an increasing index.
	When the index is zero, no random number is selected.

	This program uses the Portable Audio library which is under development.
	For more information see:   http://www.audiomulch.com/portaudio/

	Author: Phil Burk, http://www.softsynth.com
	
	Revision History:

	Copyleft 1999 Phil Burk - No rights reserved.
*/

#include "syn123_int.h"
#include <math.h>

// Calculate pseudo-random 32 bit number based on linear congruential method.
// ... or via xor after all? That looks a lot better when not
// faking out with 64 bit long.
static int32_t GenerateRandomNumber(uint32_t *seed)
{
#if 0
	*seed = *seed * 196314165 + 907633515;
#else
	*seed ^= (*seed<<13);
	*seed ^= (*seed>>17);
	*seed ^= (*seed<<5);
#endif
	return (int32_t)*seed; // Implementaton-defined ...
}

#define PINK_MAX_RANDOM_ROWS   (30)
#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(int32_t)*8)-PINK_RANDOM_BITS-1)

typedef struct
{
	int32_t  pink_Rows[PINK_MAX_RANDOM_ROWS];
	int32_t  pink_RunningSum;   /* Used to optimize summing of generators. */
	int       pink_Index;        /* Incremented each sample. */
	int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
	float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
	uint32_t  rand_value;        /* The state of the random number generator. */
} PinkNoise;

/* Setup PinkNoise structure for N rows of generators. */
static void InitializePinkNoise( PinkNoise *pink, int numRows )
{
	int i;
	int32_t pmax;
	pink->pink_Index = 0;
	pink->pink_IndexMask = (1<<numRows) - 1;
	/* Calculate maximum possible random value. Extra 1 for white noise always added. */
	pmax = (numRows + 1) * (1<<(PINK_RANDOM_BITS-1));
	pink->pink_Scalar = 1.0f / pmax;
/* Initialize rows. */
	for( i=0; i<numRows; i++ ) pink->pink_Rows[i] = 0;
	pink->pink_RunningSum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
static float GeneratePinkNoise( PinkNoise *pink )
{
	int32_t newRandom;
	int32_t sum;
	float output;

/* Increment and mask index. */
	pink->pink_Index = (pink->pink_Index + 1) & pink->pink_IndexMask;

/* If index is zero, don't update any random values. */
	if( pink->pink_Index != 0 )
	{
	/* Determine how many trailing zeros in PinkIndex. */
	/* This algorithm will hang if n==0 so test first. */
		int numZeros = 0;
		int n = pink->pink_Index;
		while( (n & 1) == 0 )
		{
			n = n >> 1;
			numZeros++;
		}

	/* Replace the indexed ROWS random value.
	 * Subtract and add back to RunningSum instead of adding all the random
	 * values together. Only one changes each time.
	 */
		pink->pink_RunningSum -= pink->pink_Rows[numZeros];
		newRandom = GenerateRandomNumber(&pink->rand_value) >> PINK_RANDOM_SHIFT;
		pink->pink_RunningSum += newRandom;
		pink->pink_Rows[numZeros] = newRandom;
	}
	
/* Add extra white noise value. */
	newRandom = GenerateRandomNumber(&pink->rand_value) >> PINK_RANDOM_SHIFT;
	sum = pink->pink_RunningSum + newRandom;

/* Scale to range of -1.0 to 0.9999. */
	output = pink->pink_Scalar * sum;

	return output;
}

static void pink_generator(syn123_handle *sh, int samples)
{
	for(int i=0; i<samples; ++i)
		sh->workbuf[1][i] = GeneratePinkNoise(sh->handle);
}

int attribute_align_arg
syn123_setup_pink(syn123_handle *sh, int rows, unsigned long seed, size_t *period)
{
	int ret = SYN123_OK;
	if(!sh)
		return SYN123_BAD_HANDLE;
	syn123_setup_silence(sh);
	if(rows < 1)
		rows = 22;
	if(rows > PINK_MAX_RANDOM_ROWS)
		rows = PINK_MAX_RANDOM_ROWS;
	PinkNoise *handle = malloc(sizeof(PinkNoise));
	if(!handle)
		return SYN123_DOOM;
	handle->rand_value = seed;
	InitializePinkNoise(handle, rows);
	sh->handle = handle;
	sh->generator = pink_generator;
	if(sh->maxbuf)
	{
		size_t samplesize = MPG123_SAMPLESIZE(sh->fmt.encoding);
		size_t buffer_samples = sh->maxbuf/samplesize;
		grow_buf(sh, buffer_samples*samplesize);
		if(buffer_samples > sh->bufs/samplesize)
		{
			ret = SYN123_DOOM;
			goto setup_pink_end;
		}
		int outchannels = sh->fmt.channels;
		sh->fmt.channels = 1;
		size_t buffer_bytes = syn123_read(sh, sh->buf, buffer_samples*samplesize);
		sh->fmt.channels = outchannels;
		InitializePinkNoise(handle, rows);
		if(buffer_bytes != buffer_samples*samplesize)
		{
			ret = SYN123_WEIRD;
			goto setup_pink_end;
		}
		sh->samples = buffer_samples;
	}
	if(sh->samples)
	{
		handle->rand_value = seed;
		InitializePinkNoise(handle, rows);
	}
setup_pink_end:
	if(ret != SYN123_OK)
		syn123_setup_silence(sh);
	if(period)
		*period = sh->samples;
	return ret;
}


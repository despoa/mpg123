/*
	optimize: get a grip on the different optimizations

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, inspired by 3DNow stuff in mpg123.[hc]

	Currently, this file contains the struct and function to choose an optimization variant and works only when OPT_MULTI is in effect.
*/

#include "mpg123.h" /* includes optimize.h */

#ifdef OPT_MULTI

#include "getcpuflags.h"
struct cpuflags cpu_flags;

void list_cpu_opt()
{
	printf("CPU options:");
	#ifdef OPT_3DNOWEXT
	printf(" 3DNowExt");
	#endif
	#ifdef OPT_SSE
	printf(" SSE");
	#endif
	#ifdef OPT_3DNOW
	printf(" 3DNow");
	#endif
	#ifdef OPT_MMX
	printf(" MMX");
	#endif
	#ifdef OPT_I586
	printf(" i586");
	#endif
	#ifdef OPT_I586_DITHER
	printf(" i586_dither");
	#endif
	#ifdef OPT_I486
	printf(" i486");
	#endif
	#ifdef OPT_I386
	printf(" i386");
	#endif
	#ifdef OPT_ALTIVEC
	printf(" AltiVec");
	#endif
	#ifdef OPT_GENERIC
	printf(" generic");
	#endif
	printf("\n");
}

void test_cpu_flags()
{
	#ifdef OPT_X86
	struct cpuflags cf;
	getcpuflags(&cf);
	if(cpu_i586(cf))
	{
		printf("Supported decoders:");
		/* not yet: if(cpu_sse2(cf)) printf(" SSE2");
		if(cpu_sse3(cf)) printf(" SSE3"); */
#ifdef OPT_3DNOWEXT
		if(cpu_3dnowext(cf)) printf(" 3DNowExt");
#endif
#ifdef OPT_SSE
		if(cpu_sse(cf)) printf(" SSE");
#endif
#ifdef OPT_3DNOW
		if(cpu_3dnow(cf)) printf(" 3DNow");
#endif
#ifdef OPT_MMX
		if(cpu_mmx(cf)) printf(" MMX");
#endif
#ifdef OPT_I586
		printf(" i586");
#endif
#ifdef OPT_I586_DITHER
		printf(" i586_dither");
#endif
#ifdef OPT_I386
		printf(" i386");
#endif
#ifdef OPT_GENERIC
		printf(" generic");
#endif
		printf("\n");
	}
	else
	{
		printf("You have an i386 or i486... or perhaps a non-x86-32bit CPU?\n");
	}
	#else
	printf("I only know x86 cpus...\n");
	#endif
}

#endif

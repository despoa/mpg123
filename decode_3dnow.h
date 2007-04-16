/*
 * Mpeg Layer-1,2,3 audio decoder
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 *
 * slighlty optimized for machines without autoincrement/decrement.
 * The performance is highly compiler dependend. Maybe
 * the decode.c version for 'normal' processor may be faster
 * even for Intel processors.
 *
 * 3DNow! and 3DNow!Ex optimizations by Nick Kurshev <nickols_k@mail.ru>
 */
#ifdef HAVE_3DNOWEX
int synth_1to1_3dnowex16(real *bandPtr,int channel,short *samples)
#else
int synth_1to1_3dnow16(real *bandPtr,int channel,short *samples)
#endif
{
  static real buffs[2][2][0x110];
  static const int step = 2;
  static int bo = 1;
#if 0
  short *samples = (short *) (out + *pnt);
#endif
  real *b0,(*buf)[0x110];
  int clip = 0;
  int bo1;
  if(!channel) {     /* channel=0 */
    bo--;
    bo &= 0xf;
    buf = buffs[0];
  }
  else {
    samples++;
    buf = buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
#ifdef HAVE_3DNOWEX
    dct64_3dnowex(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
#else
    dct64_3dnow(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
#endif
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
#ifdef HAVE_3DNOWEX
    dct64_3dnowex(buf[0]+bo,buf[1]+bo+1,bandPtr);
#else
    dct64_3dnow(buf[0]+bo,buf[1]+bo+1,bandPtr);
#endif
  }

  {
    register int j;
    real _tmp[2];
    register int res;
    real *window = decwin + 16 - bo1;
__asm __volatile("femms"::
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
    for (j=16;j;j--,b0+=0x10,window+=0x20,samples+=step)
    {
	__asm __volatile(
	"movq	(%2),	%%mm0\n\t"
	"movq	(%3),	%%mm1\n\t"
	"pfmul	%%mm1,	%%mm0\n\t"
	"movq	8(%2),	%%mm2\n\t"
	"movq	8(%3),	%%mm3\n\t"
	"pfmul	%%mm3,	%%mm2\n\t"
	"movq	16(%2),	%%mm4\n\t"
	"movq	16(%3),	%%mm5\n\t"
	"pfmul	%%mm5,	%%mm4\n\t"
	"movq	24(%2),	%%mm6\n\t"
	"movq	24(%3),	%%mm7\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"movq	%%mm0,	(%1)\n\t"
	"movq	32(%2),	%%mm0\n\t"
	"movq	32(%3),	%%mm1\n\t"
	"pfmul	%%mm1,	%%mm0\n\t"
	"movq	40(%2),	%%mm2\n\t"
	"movq	40(%3),	%%mm3\n\t"
	"pfmul	%%mm3,	%%mm2\n\t"
	"movq	48(%2),	%%mm4\n\t"
	"movq	48(%3),	%%mm5\n\t"
	"pfmul	%%mm5,	%%mm4\n\t"
	"movq	56(%2),	%%mm6\n\t"
	"movq	56(%3),	%%mm7\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"pfadd	(%1),	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
	"pfpnacc %%mm0,	%%mm0\n\t"
#else
	"movq	%%mm0,	%%mm1\n\t"
	"psrlq	$32,	%%mm1\n\t"
	"pfsub	%%mm1,	%%mm0\n\t"
#endif
	"movd	%4,	%%mm6\n\t"
	"pfmul	%%mm6,	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
	"pf2iw	%%mm0,	%%mm0\n\t"
#else
	"pf2id	%%mm0,	%%mm0\n\t"
	"packssdw %%mm0, %%mm0\n\t"
#endif
	"movd	%%mm0,	%0"
	:"=&r"(res)
	:"r"(_tmp), "r"(window), "r"(b0), "m"(mp3_scaler)
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
	*samples = res;
    }
    __asm __volatile(
	"movd	(%1),	%%mm0\n\t"
	"movd	(%2),	%%mm1\n\t"
	"punpckldq 8(%1), %%mm0\n\t"
	"punpckldq 8(%2), %%mm1\n\t"
	"pfmul	%%mm1,	%%mm0\n\t"
	"movd	16(%1),	%%mm2\n\t"
	"movd	16(%2),	%%mm3\n\t"
	"punpckldq 24(%1), %%mm2\n\t"
	"punpckldq 24(%2), %%mm3\n\t"
	"pfmul	%%mm3,	%%mm2\n\t"
	"movd	32(%1),	%%mm4\n\t"
	"movd	32(%2),	%%mm5\n\t"
	"punpckldq 40(%1), %%mm4\n\t"
	"punpckldq 40(%2), %%mm5\n\t"
	"pfmul	%%mm5,	%%mm4\n\t"
	"movd	48(%1),	%%mm6\n\t"
	"movd	48(%2),	%%mm7\n\t"
	"punpckldq 56(%1), %%mm6\n\t"
	"punpckldq 56(%2), %%mm7\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"movd	%3,	%%mm7\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"pfacc	%%mm0,	%%mm0\n\t"
	"pfmul	%%mm7,	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
	"pf2iw	%%mm0,	%%mm0\n\t"
#else
	"pf2id	%%mm0,	%%mm0\n\t"
	"packssdw %%mm0,%%mm0\n\t"
#endif
	"movd	%%mm0,	%0\n\t"
	:"=&r"(res)
	:"g"(window),"g"(b0),"m"(mp3_scaler)
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
    );
    *samples = res;
    b0-=0x10,window-=0x20,samples+=step;
    window += bo1<<1;

    for (j=15;j;j--,b0-=0x10,window-=0x20,samples+=step)
    {
	__asm __volatile(
#ifdef HAVE_3DNOWEX
        "pswapd	-8(%2),	%%mm0\n\t"
	"movq	(%3),	%%mm1\n\t"
#else
        "movd	-4(%2),	%%mm0\n\t"
	"movq	(%3),	%%mm1\n\t"
	"punpckldq -8(%2), %%mm0\n\t"
#endif
	"pfmul	%%mm1,	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-16(%2),%%mm2\n\t"
	"movq	8(%3),	%%mm3\n\t"
#else
        "movd	-12(%2),%%mm2\n\t"
	"movq	8(%3),	%%mm3\n\t"
	"punpckldq -16(%2),%%mm2\n\t"
#endif
	"pfmul	%%mm3,	%%mm2\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-24(%2),%%mm4\n\t"
	"movq	16(%3),	%%mm5\n\t"
#else
        "movd	-20(%2),%%mm4\n\t"
	"movq	16(%3),	%%mm5\n\t"
	"punpckldq -24(%2),%%mm4\n\t"
#endif
	"pfmul	%%mm5,	%%mm4\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-32(%2),%%mm6\n\t"
	"movq	24(%3),	%%mm7\n\t"
#else
        "movd	-28(%2),%%mm6\n\t"
	"movq	24(%3),	%%mm7\n\t"
	"punpckldq -32(%2),%%mm6\n\t"
#endif
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"movq	%%mm0,	(%1)\n\t"

#ifdef HAVE_3DNOWEX
        "pswapd	-40(%2),%%mm0\n\t"
	"movq	32(%3),	%%mm1\n\t"
#else
        "movd	-36(%2),%%mm0\n\t"
	"movq	32(%3),	%%mm1\n\t"
	"punpckldq -40(%2), %%mm0\n\t"
#endif
	"pfmul	%%mm1,	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-48(%2),%%mm2\n\t"
	"movq	40(%3),	%%mm3\n\t"
#else
        "movd	-44(%2),%%mm2\n\t"
	"movq	40(%3),	%%mm3\n\t"
	"punpckldq -48(%2),%%mm2\n\t"
#endif
	"pfmul	%%mm3,	%%mm2\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-56(%2),%%mm4\n\t"
	"movq	48(%3),	%%mm5\n\t"
#else
        "movd	-52(%2),%%mm4\n\t"
	"movq	48(%3),	%%mm5\n\t"
	"punpckldq -56(%2),%%mm4\n\t"
#endif
	"pfmul	%%mm5,	%%mm4\n\t"
        "movd	-60(%2),%%mm6\n\t"
	"movq	56(%3),	%%mm7\n\t"
	"punpckldq (%2),%%mm6\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"pfadd	(%1),	%%mm0\n\t"
	"pxor	%%mm7,	%%mm7\n\t"
	"pfsub	%%mm0,	%%mm7\n\t"
	"movd	%4,	%%mm6\n\t"
	"pfacc	%%mm7,	%%mm7\n\t"
	"pfmul	%%mm6,	%%mm7\n\t"
#ifdef HAVE_3DNOWEX
	"pf2iw	%%mm7,	%%mm7\n\t"
#else
	"pf2id	%%mm7,	%%mm7\n\t"
	"packssdw %%mm7,%%mm7\n\t"
#endif
	"movd	%%mm7, %0"
	:"=&r"(res)
	:"r"(_tmp), "r"(window), "r"(b0), "m"(mp3_scaler)
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
	*samples=res;
    }
  }
  __asm __volatile("femms"::
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
  return clip;

}

#ifdef HAVE_3DNOWEX
int synth_1to1_3dnowex32(real *bandPtr,int channel,float *samples)
#else
int synth_1to1_3dnow32(real *bandPtr,int channel,float *samples)
#endif
{
  static real buffs[2][2][0x110];
  static const int step = 2;
  static int bo = 1;
  real *b0,(*buf)[0x110];
  int clip = 0;
  int bo1;
  if(!channel) {     /* channel=0 */
    bo--;
    bo &= 0xf;
    buf = buffs[0];
  }
  else {
    samples++;
    buf = buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
#ifdef HAVE_3DNOWEX
    dct64_3dnowex(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
#else
    dct64_3dnow(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
#endif
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
#ifdef HAVE_3DNOWEX
    dct64_3dnowex(buf[0]+bo,buf[1]+bo+1,bandPtr);
#else
    dct64_3dnow(buf[0]+bo,buf[1]+bo+1,bandPtr);
#endif
  }

  {
    register int j;
    real _tmp[2];
    register float res;
    real *window = decwin + 16 - bo1;
__asm __volatile("femms"::
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
    for (j=16;j;j--,b0+=0x10,window+=0x20,samples+=step)
    {
	__asm __volatile(
	"movq	(%2),	%%mm0\n\t"
	"movq	(%3),	%%mm1\n\t"
	"pfmul	%%mm1,	%%mm0\n\t"
	"movq	8(%2),	%%mm2\n\t"
	"movq	8(%3),	%%mm3\n\t"
	"pfmul	%%mm3,	%%mm2\n\t"
	"movq	16(%2),	%%mm4\n\t"
	"movq	16(%3),	%%mm5\n\t"
	"pfmul	%%mm5,	%%mm4\n\t"
	"movq	24(%2),	%%mm6\n\t"
	"movq	24(%3),	%%mm7\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"movq	%%mm0,	(%1)\n\t"
	"movq	32(%2),	%%mm0\n\t"
	"movq	32(%3),	%%mm1\n\t"
	"pfmul	%%mm1,	%%mm0\n\t"
	"movq	40(%2),	%%mm2\n\t"
	"movq	40(%3),	%%mm3\n\t"
	"pfmul	%%mm3,	%%mm2\n\t"
	"movq	48(%2),	%%mm4\n\t"
	"movq	48(%3),	%%mm5\n\t"
	"pfmul	%%mm5,	%%mm4\n\t"
	"movq	56(%2),	%%mm6\n\t"
	"movq	56(%3),	%%mm7\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"pfadd	(%1),	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
	"pfpnacc %%mm0,	%%mm0\n\t"
#else
	"movq	%%mm0,	%%mm1\n\t"
	"psrlq	$32,	%%mm1\n\t"
	"pfsub	%%mm1,	%%mm0\n\t"
#endif
	"movd	%4,	%%mm6\n\t"
	"pfmul	%%mm6,	%%mm0\n\t"
	"movd	%%mm0,	%0"
	:"=&r"(res)
	:"r"(_tmp), "r"(window), "r"(b0), "m"(mp3_scaler)
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
	*samples = res;
    }
    __asm __volatile(
	"movd	(%1),	%%mm0\n\t"
	"movd	(%2),	%%mm1\n\t"
	"punpckldq 8(%1), %%mm0\n\t"
	"punpckldq 8(%2), %%mm1\n\t"
	"pfmul	%%mm1,	%%mm0\n\t"
	"movd	16(%1),	%%mm2\n\t"
	"movd	16(%2),	%%mm3\n\t"
	"punpckldq 24(%1), %%mm2\n\t"
	"punpckldq 24(%2), %%mm3\n\t"
	"pfmul	%%mm3,	%%mm2\n\t"
	"movd	32(%1),	%%mm4\n\t"
	"movd	32(%2),	%%mm5\n\t"
	"punpckldq 40(%1), %%mm4\n\t"
	"punpckldq 40(%2), %%mm5\n\t"
	"pfmul	%%mm5,	%%mm4\n\t"
	"movd	48(%1),	%%mm6\n\t"
	"movd	48(%2),	%%mm7\n\t"
	"punpckldq 56(%1), %%mm6\n\t"
	"punpckldq 56(%2), %%mm7\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"movd	%3,	%%mm7\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"pfacc	%%mm0,	%%mm0\n\t"
	"pfmul	%%mm7,	%%mm0\n\t"
	"movd	%%mm0,	%0\n\t"
	:"=&r"(res)
	:"g"(window),"g"(b0),"m"(mp3_scaler)
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
    );
    *samples = res;
    b0-=0x10,window-=0x20,samples+=step;
    window += bo1<<1;

    for (j=15;j;j--,b0-=0x10,window-=0x20,samples+=step)
    {
	__asm __volatile(
#ifdef HAVE_3DNOWEX
        "pswapd	-8(%2),	%%mm0\n\t"
	"movq	(%3),	%%mm1\n\t"
#else
        "movd	-4(%2),	%%mm0\n\t"
	"movq	(%3),	%%mm1\n\t"
	"punpckldq -8(%2), %%mm0\n\t"
#endif
	"pfmul	%%mm1,	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-16(%2),%%mm2\n\t"
	"movq	8(%3),	%%mm3\n\t"
#else
        "movd	-12(%2),%%mm2\n\t"
	"movq	8(%3),	%%mm3\n\t"
	"punpckldq -16(%2),%%mm2\n\t"
#endif
	"pfmul	%%mm3,	%%mm2\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-24(%2),%%mm4\n\t"
	"movq	16(%3),	%%mm5\n\t"
#else
        "movd	-20(%2),%%mm4\n\t"
	"movq	16(%3),	%%mm5\n\t"
	"punpckldq -24(%2),%%mm4\n\t"
#endif
	"pfmul	%%mm5,	%%mm4\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-32(%2),%%mm6\n\t"
	"movq	24(%3),	%%mm7\n\t"
#else
        "movd	-28(%2),%%mm6\n\t"
	"movq	24(%3),	%%mm7\n\t"
	"punpckldq -32(%2),%%mm6\n\t"
#endif
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"movq	%%mm0,	(%1)\n\t"

#ifdef HAVE_3DNOWEX
        "pswapd	-40(%2),%%mm0\n\t"
	"movq	32(%3),	%%mm1\n\t"
#else
        "movd	-36(%2),%%mm0\n\t"
	"movq	32(%3),	%%mm1\n\t"
	"punpckldq -40(%2), %%mm0\n\t"
#endif
	"pfmul	%%mm1,	%%mm0\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-48(%2),%%mm2\n\t"
	"movq	40(%3),	%%mm3\n\t"
#else
        "movd	-44(%2),%%mm2\n\t"
	"movq	40(%3),	%%mm3\n\t"
	"punpckldq -48(%2),%%mm2\n\t"
#endif
	"pfmul	%%mm3,	%%mm2\n\t"
#ifdef HAVE_3DNOWEX
        "pswapd	-56(%2),%%mm4\n\t"
	"movq	48(%3),	%%mm5\n\t"
#else
        "movd	-52(%2),%%mm4\n\t"
	"movq	48(%3),	%%mm5\n\t"
	"punpckldq -56(%2),%%mm4\n\t"
#endif
	"pfmul	%%mm5,	%%mm4\n\t"
        "movd	-60(%2),%%mm6\n\t"
	"movq	56(%3),	%%mm7\n\t"
	"punpckldq (%2),%%mm6\n\t"
	"pfmul	%%mm7,	%%mm6\n\t"
	"pfadd	%%mm2,	%%mm0\n\t"
	"pfadd	%%mm6,	%%mm4\n\t"
	"pfadd	%%mm4,	%%mm0\n\t"
	"pfadd	(%1),	%%mm0\n\t"
	"pxor	%%mm7,	%%mm7\n\t"
	"pfsub	%%mm0,	%%mm7\n\t"
	"movd	%4,	%%mm6\n\t"
	"pfacc	%%mm7,	%%mm7\n\t"
	"pfmul	%%mm6,	%%mm7\n\t"
	"movd	%%mm7, %0"
	:"=&r"(res)
	:"r"(_tmp), "r"(window), "r"(b0), "m"(mp3_scaler)
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
	*samples=res;
    }
  }
  __asm __volatile("femms"::
	:"memory"
#ifdef FPU_CLOBBERED
	,FPU_CLOBBERED
#endif
#ifdef MMX_CLOBBERED
	,MMX_CLOBBERED
#endif
	);
  return clip;

}

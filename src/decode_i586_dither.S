/*
	synth_1to1 works the same way as the c version of this
	file.  only two types of changes have been made:
	- reordered floating point instructions to
	 prevent pipline stalls
	- made WRITE_SAMPLE use integer instead of
	 (slower) floating point
	all kinds of x86 processors should benefit from these
	modifications.

	useful sources of information on optimizing x86 code include:

	   Intel Architecture Optimization Manual
	   http:#/www.intel.com/design/pentium/manuals/242816.htm

	   Cyrix 6x86 Instruction Set Summary
	   ftp:#/ftp.cyrix.com/6x86/6x-dbch6.pdf

	   AMD-K5 Processor Software Development
	   http:#/www.amd.com/products/cpg/techdocs/appnotes/20007e.pdf

	Stefan Bieschewski <stb@acm.org>

	You can use this part under GPL.

	This version uses "circular" 64k dither noise.
	(Patch by Adrian <adrian.bacon@xs4all.nl>)
*/

#include "mangle.h"

.data
#ifndef __APPLE__
		.section	.rodata
#endif
	ALIGN8
.LC0:
	.long 0x0,0x40dfffc0
	ALIGN8
.LC1:
	.long 0x0,0xc0e00000
	ALIGN8
.text
/* int synth_1to1_i586_asm_dither(real *bandPtr, int channel, unsigned char *out, unsigned char *buffs, int bo_and_ditherindex[2]); */
.globl ASM_NAME(synth_1to1_i586_asm_dither)
ASM_NAME(synth_1to1_i586_asm_dither):
	subl $16,%esp
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %ebx
/* The stack: 0(%esp)=%ebx 4=esi 8=edi 12=ebp 16,20,24=local 28=back 32=bandptr 36=channel 40=out 44=buffs 48=bo */
/* new stack: 0(%esp)=%ebx 4=esi 8=edi 12=ebp 16,20,24,28=local 32=back 36=bandptr 40=channel 44=out 48=buffs 52=bo */
#define BANDPTR 36(%esp)
#define CHANNEL 40(%esp)
#define OUT     44(%esp)
#define BUFFS   48(%esp)
#define BO      52(%esp)
#define LOC0    16(%esp)
#define LOC1    20(%esp)
#define LOC2    24(%esp)
#define DITHERINDEX  28(%esp)
	movl BANDPTR,%eax
	movl OUT,%esi
	movl BO, %ebx
	movl (%ebx),%ebp    /* get bo value */
	movl 4(%ebx),%edi;  /* get the ditherindex behind bo */
	movl %edi,DITHERINDEX
	xorl %edi,%edi
	cmpl %edi,CHANNEL
	jne .L48
	decl %ebp
	andl $15,%ebp
	movl %ebp,(%ebx)   /* save bo back */
	movl BUFFS,%ecx
	jmp .L49
.L48:
/*       In stereo mode , "rewind" dither pointer 32 samples , so 2nd channel */
/*       has same dither values. Tested OK for mono and stereo MP2 and MP3 */
	subl $128,DITHERINDEX /* better move to %edi for the two calculations? */
	andl $0x0003fffc,DITHERINDEX
	addl $2,%esi
	movl BUFFS,%ecx
	addl $2176,%ecx
.L49:
	testl $1,%ebp
	je .L50
	movl %ecx,%ebx
	movl %ebp,LOC0
	pushl %eax
	movl LOC1,%edx
	leal (%ebx,%edx,4),%eax
	pushl %eax
	movl LOC2,%eax
	incl %eax
	andl $15,%eax
	leal 1088(,%eax,4),%eax
	addl %ebx,%eax
	jmp .L74
.L50:
	leal 1088(%ecx),%ebx
	leal 1(%ebp),%edx
	movl %edx,LOC0
	pushl %eax
	leal 1092(%ecx,%ebp,4),%eax
	pushl %eax
	leal (%ecx,%ebp,4),%eax
/* The stack after 2 pushes: 8(%esp)=%ebx 12=esi 16=edi 20=ebp 24,28,32=local 36=back 40=bandptr 44=channel 48=out 52=buffs 56=bo */
.L74:
	pushl %eax
/* The stack after 1 push: 12(%esp)=%ebx 16=esi 20=edi 24=ebp 28,32,36=local 40=back 44=bandptr 48=channel 52=out 56=buffs 60=bo */
	call ASM_NAME(dct64_i386)
	addl $12,%esp
/* Now removed the parameters.
   The stack: 0(%esp)=%ebx 4=esi 8=edi 12=ebp 16,20,24=local 28=back 32=bandptr 36=channel 40=out 44=buffs 48=bo
   new stack: 0(%esp)=%ebx 4=esi 8=edi 12=ebp 16,20,24,28=local 32=back 36=bandptr 40=channel 44=out 48=buffs 52=bo */
	movl LOC0,%edx
	leal 0(,%edx,4),%edx
	movl $ASM_NAME(decwin)+64,%eax
	movl %eax,%ecx
	subl %edx,%ecx
	movl $16,%ebp
.L55:
	flds (%ecx)
	fmuls (%ebx)
	flds 4(%ecx)
	fmuls 4(%ebx)
	fxch %st(1)
	flds 8(%ecx)
	fmuls 8(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds 12(%ecx)
	fmuls 12(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 16(%ecx)
	fmuls 16(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds 20(%ecx)
	fmuls 20(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 24(%ecx)
	fmuls 24(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds 28(%ecx)
	fmuls 28(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 32(%ecx)
	fmuls 32(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds 36(%ecx)
	fmuls 36(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 40(%ecx)
	fmuls 40(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds 44(%ecx)
	fmuls 44(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 48(%ecx)
	fmuls 48(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds 52(%ecx)
	fmuls 52(%ebx)
	fxch %st(2)         
	faddp %st,%st(1)
	flds 56(%ecx)
	fmuls 56(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds 60(%ecx)
	fmuls 60(%ebx)
	fxch %st(2)
	subl $4,%esp
	faddp %st,%st(1)
	fxch %st(1)
	fsubrp %st,%st(1)

	addl $4,DITHERINDEX
	andl $0x0003fffc,DITHERINDEX
	movl $ASM_NAME(dithernoise),%edi
	addl DITHERINDEX,%edi	

	fadd (%edi)

/* fistpl and popl as a unit keep the stack unchanged */
	fistpl (%esp)
	popl %eax
	cmpl $32767,%eax
	jg 1f
	cmpl $-32768,%eax
	jl 2f
	movw %ax,(%esi)
	jmp 4f
1:	movw $32767,(%esi)
	jmp 3f
2:	movw $-32768,(%esi)
3:
/*	incl %edi */
4:
.L54:
	addl $64,%ebx
	subl $-128,%ecx
	addl $4,%esi
	decl %ebp
	jnz .L55
	flds (%ecx)
	fmuls (%ebx)
	flds 8(%ecx)
	fmuls 8(%ebx)
	flds 16(%ecx)
	fmuls 16(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 24(%ecx)
	fmuls 24(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 32(%ecx)
	fmuls 32(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 40(%ecx)
	fmuls 40(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 48(%ecx)
	fmuls 48(%ebx)
	fxch %st(2)
	faddp %st,%st(1)
	flds 56(%ecx)
	fmuls 56(%ebx)
	fxch %st(2)
	subl $4,%esp
	faddp %st,%st(1)
	fxch %st(1)
	faddp %st,%st(1)

	addl $4,DITHERINDEX
	andl $0x0003fffc,DITHERINDEX
	movl $ASM_NAME(dithernoise),%edi
	addl DITHERINDEX,%edi	

	fadd (%edi)
/* fistpl and popl as a unit keep the stack unchanged */
	fistpl (%esp)
	popl %eax
	cmpl $32767,%eax
	jg 1f
	cmpl $-32768,%eax
	jl 2f
	movw %ax,(%esi)
	jmp 4f
1:	movw $32767,(%esi)
	jmp 3f
2:	movw $-32768,(%esi)
3:
/*	incl %edi */
4:
.L62:
	addl $-64,%ebx
	addl $4,%esi
	movl LOC0,%edx
	leal -128(%ecx,%edx,8),%ecx
	movl $15,%ebp
.L68:
	flds -4(%ecx)
	fchs
	fmuls (%ebx)
	flds -8(%ecx)
	fmuls 4(%ebx)
	fxch %st(1)
	flds -12(%ecx)
	fmuls 8(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -16(%ecx)
	fmuls 12(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -20(%ecx)
	fmuls 16(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -24(%ecx)
	fmuls 20(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -28(%ecx)
	fmuls 24(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -32(%ecx)
	fmuls 28(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -36(%ecx)
	fmuls 32(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -40(%ecx)
	fmuls 36(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -44(%ecx)
	fmuls 40(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -48(%ecx)
	fmuls 44(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -52(%ecx)
	fmuls 48(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -56(%ecx)
	fmuls 52(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds -60(%ecx)
	fmuls 56(%ebx)
	fxch %st(2)
	fsubrp %st,%st(1)
	flds (%ecx)
	fmuls 60(%ebx)
	fxch %st(2)
	subl $4,%esp
	fsubrp %st,%st(1)
	fxch %st(1)
	fsubrp %st,%st(1)

	addl $4,DITHERINDEX
	andl $0x0003fffc,DITHERINDEX
	movl $ASM_NAME(dithernoise),%edi
	addl DITHERINDEX,%edi	

	fadd (%edi)
/* fistpl and popl as a unit keep the stack unchanged */
	fistpl (%esp)
	popl %eax
	cmpl $32767,%eax
	jg 1f
	cmpl $-32768,%eax
	jl 2f
	movw %ax,(%esi)
	jmp 4f
1:	movw $32767,(%esi)
	jmp 3f
2:	movw $-32768,(%esi)
3:
/*	incl %edi */
4:
.L67:
	addl $-64,%ebx
	addl $-128,%ecx
	addl $4,%esi
	decl %ebp
	jnz .L68
/* return ipv edi 0 in eax */
	movl $0,%eax
/* save ditherindex */
	movl BO,%ebx
	movl DITHERINDEX,%esi
	movl %esi,4(%ebx);
/* 0=ebx 4=esi 8=edi 12=ebp 16,20,24,28=local 32=back 36=bandptr 40=channel 44=out 48=buffs 52=bo */
	popl %ebx
	popl %esi
	popl %edi
	popl %ebp
/*    0,4,8=local 12=back 16=bandptr 20=channel 24=out 28=buffs 32=bo
  new 0,4,8,12=local 16=back 20=bandptr 24=channel 28=out 32=buffs 36=bo */
	addl $16,%esp
/* The stack must be now: 0=back 4=bandptr 8=channel 12=out 16=buffs 20=bo */
	ret


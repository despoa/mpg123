#include "../config.h"
#include "mpg123.h"

#ifndef CAN_COMPILE_X86_ASM
#if defined( ARCH_X86 ) || defined(ARCH_X86_64)
#define CAN_COMPILE_X86_ASM
#endif
#endif

#undef HAVE_3DNOW
#undef HAVE_3DNOWEX

#ifdef CAN_COMPILE_3DNOW
#define HAVE_3DNOW
#include "decode_3dnow.h"
#endif
#ifdef CAN_COMPILE_3DNOW2
#define HAVE_3DNOWEX
#include "decode_3dnow.h"
#endif

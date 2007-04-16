# a BSD compatible 'install' program
INSTALL = install
include ../config.mak
LIBNAME = libMP3$(SLIBSUF)
BINDIR = $(prefix)/lib/$(PROGNAME)/codecs
SRCS = sr1.c
OBJS = sr1.o
# OBJS = $(SRCS:.c,.s=.o)
ifeq ($(TARGET_ARCH_SGI_MIPS),yes)
OPTFLAGS := $(OPTFLAGS:-O4=-O0)
endif
CFLAGS  = $(OPTFLAGS) $(EXTRA_INC)
ifneq ($(TARGET_ARCH_X86),yes)
CFLAGS += -fPIC
endif
ifeq ($(TARGET_ARCH_X86),yes)
SRCS += decode_MMX.c dct64_MMX.c tabinit_MMX.c
OBJS += decode_MMX.o dct64_MMX.o tabinit_MMX.o
SRCS += dct36_3dnow.c dct64_3dnow.c decode_3dnow.c
OBJS += dct36_3dnow.o dct64_3dnow.o decode_3dnow.o
SRCS += dct64_k7.c
OBJS += dct64_k7.o
endif

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

.s.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBNAME):	$(OBJS)
ifeq ($(TARGET_OS),WIN32)
	$(CC) --shared -o $@ $(OBJS) -lm -lc
ifeq ($(TARGET_OS),OpenBSD)
#	./mp3lib_objfix.sh
endif
else
	$(LD) --shared -soname $@ -o $@ $(OBJS) -lm -lc
endif

test1:	$(LIBNAME) test.c
	$(CC) $(CFLAGS) test.c ../../mplayerxp/my_profile.c -o test1 -I.. -L. -lMP3 -lm -lc -Xlinker -rpath ./

test2:	$(LIBNAME) test2.c
	$(CC) $(CFLAGS) test2.c -o test2 -I.. -L. -lMP3 -lm -lc -Xlinker -rpath ./

all:	$(LIBNAME)

install:
	$(INSTALL) -D -m 755 -s -p $(LIBNAME) $(BINDIR)/$(LIBNAME)
uninstall:
	rm -f $(BINDIR)/$(LIBNAME)
	rmdir -p --ignore-fail-on-non-empty $(BINDIR)
clean:
	rm -f *~ *.o *$(SLIBSUF)

distclean:
	rm -f *~ *.o *$(SLIBSUF) Makefile.bak .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif

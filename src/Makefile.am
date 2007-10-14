## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey

AM_CFLAGS = -Ilibmpg123 @AUDIO_CFLAGS@
#AM_LDFLAGS = 
mpg123_LDADD = libmpg123/libmpg123.la @AUDIO_OBJ@ @AUDIO_LIBS@
mpg123_DEPENDENCIES = @AUDIO_OBJ@ libmpg123/libmpg123.la

EXTRA_DIST = Makefile.legacy config.h.legacy
SUBDIRS = libmpg123
CLEANFILES = *.a

bin_PROGRAMS = mpg123
#EXTRA_PROGRAMS = testcpu mpglib
EXTRA_PROGRAMS = mpglib

mpglib_LDADD = libmpg123/libmpg123.la
mpglib_DEPENDENCIES = libmpg123/libmpg123.la


mpglib_SOURCES = \
	mpglib.c \
	libmpg123/mpg123.h

#testcpu_SOURCES = \
#	testcpu.c \
#	getcpuflags.S

mpg123_SOURCES = \
	audio.c \
	audio.h \
	buffer.c \
	buffer.h \
	common.c \
	common.h \
	control_generic.c \
	libmpg123/debug.h \
	getlopt.c \
	getlopt.h \
	httpget.c \
	id3print.c \
	id3print.h \
	genre.h \
	genre.c \
	libmpg123/mpg123.h \
	mpg123.c \
	mpg123app.h \
	true.h \
	playlist.c \
	playlist.h \
	sfifo.c \
	sfifo.h \
	term.c \
	term.h \
	wav.c \
	xfermem.c \
	xfermem.h

EXTRA_mpg123_SOURCES = \
	audio_aix.c \
	audio_alib.c \
	audio_alsa.c \
	audio_dummy.c \
	audio_esd.c \
	audio_hp.c \
	audio_jack.c \
	audio_coreaudio.c \
	audio_mint.c \
	audio_nas.c \
	audio_os2.c \
	audio_oss.c \
	audio_portaudio.c \
	audio_sdl.c \
	audio_sgi.c \
	audio_sun.c \
	audio_win32.c
# \
#	getcpuflags.h

## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey

AM_CFLAGS = @AUDIO_CFLAGS@
#AM_LDFLAGS = 
mpg123_LDADD = libmpg123.la @AUDIO_OBJ@ @AUDIO_LIBS@
mpg123_DEPENDENCIES = @AUDIO_OBJ@ libmpg123.la

EXTRA_DIST = Makefile.legacy config.h.legacy

CLEANFILES = *.a

bin_PROGRAMS = mpg123
#EXTRA_PROGRAMS = testcpu mpglib
EXTRA_PROGRAMS = mpglib

mpglib_LDADD = libmpg123.la
mpglib_DEPENDENCIES = libmpg123.la

#lib_LIBRARIES = libmpg123.a
lib_LTLIBRARIES = libmpg123.la
include_HEADERS = libmpg123.h

#libmpg123_a_LIBADD = @DECODER_OBJ@
#libmpg123_a_DEPENDENCIES = @DECODER_OBJ@

libmpg123_la_LDFLAGS = -version-info @LIBMPG123_VERSION@ -export-symbols libmpg123.sym
libmpg123_la_LIBADD = @DECODER_LOBJ@
libmpg123_la_DEPENDENCIES = @DECODER_LOBJ@

mpglib_SOURCES = \
	mpglib.c \
	libmpg123.h

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
	debug.h \
	getlopt.c \
	getlopt.h \
	httpget.c \
	id3print.c \
	id3print.h \
	genre.h \
	genre.c \
	mpg123.c \
	mpg123.h \
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
	audio_win32.c \
	getcpuflags.h

libmpg123_la_SOURCES = \
	parse.c \
	parse.h \
	frame.c \
	format.c \
	frame.h \
	reader.h \
	debug.h \
	decode.h \
	decode_2to1.c \
	decode_4to1.c \
	decode_ntom.c \
	equalizer.c \
	huffman.h \
	icy.c \
	icy.h \
	id3.c \
	id3.h \
	genre.h \
	l2tables.h \
	layer1.c \
	layer2.c \
	layer3.c \
	layer3.h \
	optimize.h \
	optimize.c \
	readers.c \
	tabinit.c \
	stringbuf.c \
	stringbuf.h \
	libmpg123.c \
	libmpg123.h \
	getcpuflags.h

EXTRA_libmpg123_la_SOURCES = \
	dct36_3dnowext.S \
	dct36_3dnow.S \
	dct64_3dnowext.S \
	dct64_3dnow.S \
	dct64_altivec.c \
	dct64.c \
	dct64_i386.c \
	dct64_mmx.S \
	dct64_sse.S \
	decode_3dnowext.S \
	decode_3dnow.S \
	decode_altivec.c \
	decode.c \
	decode_i386.c \
	decode_i586_dither.S \
	decode_i586.S \
	decode_mmx.S \
	decode_sse3d.h \
	decode_sse.S \
	dnoise.c \
	equalizer_3dnow.S \
	tabinit_mmx.S \
	getcpuflags.S

# explicit preprocessing since mingw32 does not honor the big .S
.S.o:
	$(CPP) $(CPPFLAGS) $< > $<.s
	$(CCAS) $(CCASFLAGS) -c -o $@ $<.s && rm $<.s

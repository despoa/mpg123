## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://www.mpg123.org
## initially written by Nicholas J. Humfrey

pkglib_LTLIBRARIES = output_alsa.la
output_alsa_la_SOURCES = output_alsa.c
output_alsa_la_LDFLAGS = -module -avoid-version
output_alsa_la_LIBADD  = -lasound

pkglib_LTLIBRARIES = output_coreaudio.la
output_coreaudio_la_SOURCES = output_coreaudio.c
output_coreaudio_la_LDFLAGS = -module -avoid-version \
	-framework AudioToolbox \
	-framework AudioUnit \
	-framework CoreServices

pkglib_LTLIBRARIES = output_dummy.la
output_dummy_la_SOURCES = output_dummy.c
output_dummy_la_LDFLAGS = -module -avoid-version

pkglib_LTLIBRARIES = output_esound.la
output_esound_la_SOURCES = output_esound.c
output_esound_la_CFLAGS  = @ESOUND_CFLAGS@
output_esound_la_LDFLAGS = -module -avoid-version @ESOUND_LDFLAGS@
output_esound_la_LIBADD  = @ESOUND_LIBS@

pkglib_LTLIBRARIES = output_jack.la
output_jack_la_SOURCES = output_jack.c
output_jack_la_CFLAGS  = @JACK_CFLAGS@
output_jack_la_LDFLAGS = -module -avoid-version @JACK_LDFLAGS@
output_jack_la_LIBADD  = @JACK_LIBS@

pkglib_LTLIBRARIES = output_oss.la
output_oss_la_SOURCES = output_oss.c
output_oss_la_LDFLAGS = -module -avoid-version

pkglib_LTLIBRARIES = output_nas.la
output_nas_la_SOURCES = output_nas.c
output_nas_la_LDFLAGS = -module -avoid-version
output_nas_la_LIBADD = -laudio

pkglib_LTLIBRARIES = output_portaudio.la
output_portaudio_la_SOURCES = output_portaudio.c
output_portaudio_la_LDFLAGS = -module -avoid-version
output_portaudio_la_LIBADD  = -lportaudio

pkglib_LTLIBRARIES = output_pulse.la
output_pulse_la_SOURCES = output_pulse.c
output_pulse_la_CFLAGS  = @PULSE_CFLAGS@
output_pulse_la_LDFLAGS = -module -avoid-version @PULSE_LDFLAGS@
output_pulse_la_LIBADD  = @PULSE_LIBS@

pkglib_LTLIBRARIES = output_sdl.la
output_sdl_la_SOURCES = output_sdl.c
output_sdl_la_CFLAGS = @SDL_CFLAGS@
output_sdl_la_LDFLAGS = -module -avoid-version @SDL_LDFLAGS@
output_sdl_la_LIBADD = @SDL_LIBS@

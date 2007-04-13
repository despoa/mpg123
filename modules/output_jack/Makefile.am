## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://www.mpg123.org
## initially written by Nicholas J. Humfrey

pkglib_LTLIBRARIES = output_jack.la
output_jack_la_SOURCES = jack.c
output_jack_la_CFLAGS  = @JACK_CFLAGS@
output_jack_la_LDFLAGS = -module -avoid-version @JACK_LDFLAGS@
output_jack_la_LIBADD  = @JACK_LIBS@

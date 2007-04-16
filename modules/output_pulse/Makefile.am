## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://www.mpg123.org
## initially written by Nicholas J. Humfrey

pkglib_LTLIBRARIES = output_pulse.la
output_pulse_la_SOURCES = pulse.c
output_pulse_la_CFLAGS  = @PULSE_CFLAGS@
output_pulse_la_LDFLAGS = -module -avoid-version @PULSE_LDFLAGS@
output_pulse_la_LIBADD  = @PULSE_LIBS@

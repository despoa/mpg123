## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://www.mpg123.org
## initially written by Nicholas J. Humfrey

pkglib_LTLIBRARIES = output_sdl.la
output_sdl_la_SOURCES = sdl.c
output_sdl_la_CFLAGS = @SDL_CFLAGS@
output_sdl_la_LDFLAGS = -module -avoid-version @SDL_LDFLAGS@
output_sdl_la_LIBADD = @SDL_LIBS@

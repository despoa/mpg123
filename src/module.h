/*
	module: module loading and listing interface

	copyright ?-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
*/

#include "audio.h"

#ifndef _MPG123_MODULE_H_
#define _MPG123_MODULE_H_

#include <ltdl.h>


#define MPG123_MODULE_API_VERSION		(1)


typedef struct mpg123_module_struct {
	const int api_version;						/* module API version number */

	const char* name;							/* short name of the module */
	const char* description;					/* description of what the module does */
	const char* revision;						/* source code revision */
	
	lt_dlhandle handle;							/* ltdl handle - set by open_module */

	
	/* Initialisers - set to NULL if unsupported by module */
	int (*init_output)(audio_output_t* ao);		/* audio output - returns 0 on success */

} mpg123_module_t;



/* ------ Declarations from "module.c" ------ */

mpg123_module_t* open_module( const char* name );
void close_module( mpg123_module_t* module );
void list_modules();

#endif

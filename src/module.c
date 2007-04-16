/*
	mpg123: main code of the program (not of the decoder...)

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Nicholas J Humfrey
*/

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "debug.h"
#include "module.h"


#define MPG123_MODULE_PREFIX		"output_"
#define MPG123_MODULE_SUFFIX		".la"
#define MPG123_MODULE_SYMBOL 		"mpg123_module_info"


/* Open a module */
mpg123_module_t*
open_module( const char* name )
{
	lt_dlhandle handle = NULL;
	mpg123_module_t *module = NULL;
	char* module_path = NULL;
	int module_path_len = 0;
/*	int i; */

	/* Initialize libltdl */
	if (lt_dlinit()) error( "Failed to initialise libltdl" );
	

	/* Work out the path of the module to open */
	module_path_len = strlen( PKGLIBDIR ) + 1 + 
					  strlen( MPG123_MODULE_PREFIX ) + strlen( name ) +
					  strlen( MPG123_MODULE_SUFFIX ) + 1;
	module_path = malloc( module_path_len );
	if (module_path == NULL) {
		error1( "Failed to allocate memory for module name: %s", strerror(errno) );
		return NULL;
	}
	snprintf( module_path, module_path_len, "%s/%s%s%s", PKGLIBDIR, MPG123_MODULE_PREFIX, name, MPG123_MODULE_SUFFIX );
	
	
	/* FIXME: clean up the module name to prevent loading random files */
	/*for(i=strlen(PKGLIBDIR) + strlen(MODULE_PREFIX) + 1; i<(module_path_len-1); i++) {
		if (!isalnum(module_path[i])) module_path[i] = '_';
	}*/
	debug1( "Opening output module: '%s'", module_path );


	/* Open the module */
	handle = lt_dlopenext( module_path );
	free( module_path );
	if (handle==NULL) {
		error1( "Failed to open module: %s", lt_dlerror() );
		return NULL;
	}
	
	/* Get the init function from the module */
	module = (mpg123_module_t*)lt_dlsym(handle, MPG123_MODULE_SYMBOL);
	if (module==NULL) {
		error1( "Failed to get module symbol: %s", lt_dlerror() );
		lt_dlclose( handle );
		return NULL;
	}

	debug1("Module name: %s", module->name );
	debug1("Module description: %s", module->description );

	return module;
}


void close_module( mpg123_module_t* module )
{


}

void list_modules()
{
	DIR* dir = NULL;
	struct dirent *dp = NULL;

	
	/* Open the module directory */
	dir = opendir( PKGLIBDIR );
	if (dir==NULL) {
		error2("Failed to open the module directory (%s): %s\n", PKGLIBDIR, strerror(errno));
		exit(-1);
	}
	
	/* Display the program title */
	/* print_title(stderr); */

	/* List the output modules */
	printf("\n");
	printf("Output modules\n");
	printf("--------------\n");
	
	while( (dp = readdir(dir)) != NULL ) {
		if (dp->d_type == DT_REG) {
			printf("Found file: %s\n", dp->d_name );
		}
	}

	closedir( dir );
	
	exit(0);
}



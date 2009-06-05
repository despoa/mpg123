/*
	module.c: modular code loader

	copyright 1995-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J Humfrey
*/

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <ltdl.h>

#include "mpg123app.h"
#include "debug.h"
#include "mpg123.h"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winnls.h>
#include <direct.h>
#endif

#ifndef HAVE_LTDL
#error Cannot build without LTDL library support
#endif

#define MODULE_FILE_SUFFIX		__T(".la")
#define MODULE_SYMBOL_PREFIX 	"mpg123_"
#define MODULE_SYMBOL_SUFFIX 	"_module_info"

/* It's nasty to hardcode that here...
   also it does need hacking around libtool's hardcoded .la paths:
   When the .la file is in the same dir as .so file, you need libdir='.' in there. */
static const TCHAR* modulesearch[] =
{
	 __T("../lib/mpg123")
	,__T("plugins")
};

static TCHAR *get_the_cwd(); /* further down... */
static TCHAR *get_module_dir()
{
	/* Either PKGLIBDIR is accessible right away or we search for some possible plugin dirs relative to binary path. */
	_TDIR* dir = NULL;
	TCHAR *moddir = NULL;
	TCHAR *defaultdir;
	/* Compiled-in default module dir or environment variable MPG123_MODDIR. */
	defaultdir = _tgetenv(__T("MPG123_MODDIR"));
	if(defaultdir == NULL)
    {
       #if (!(_UNICODE && WIN32))
	   defaultdir=PKGLIBDIR;
       #else
       char *PKGLIBDIR_handle = PKGLIBDIR;
       defaultdir=(TCHAR *) malloc (( strlen (PKGLIBDIR_handle)+1) * sizeof (TCHAR));
       if ((MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, PKGLIBDIR_handle, -1, defaultdir, strlen (PKGLIBDIR_handle) + 1)) > 0)
       debug("PKGLIBDIR converted to wchar_t and attached to defaultdir.");
       else
       error("PKGLIBDIR conversion failed!");
       #endif
    }

	dir = _topendir(defaultdir);
	if(dir != NULL)
	{
		size_t l = _tcslen(defaultdir);
		moddir = malloc((l+1)*sizeof(TCHAR));
		if(moddir != NULL)
		{
			_tcscpy(moddir, defaultdir);
			moddir[l] = 0;
		}
		_tclosedir(dir);
	}
	else /* Search relative to binary. */
	{
		size_t i;
		for(i=0; i<sizeof(modulesearch)/sizeof(TCHAR*); ++i)
		{
			const TCHAR *testpath = modulesearch[i];
			size_t l;
			if(binpath != NULL) l = _tcslen(binpath) + _tcslen(testpath) + 1;
			else l = _tcslen(testpath);

			moddir = malloc((l+1)*sizeof(TCHAR));
			if(moddir != NULL)
			{
				if(binpath==NULL) /* a copy of testpath, when there is no prefix */
				_sntprintf(moddir, l+1, __T("%"strz), testpath);
				else
				_sntprintf(moddir, l+1, __T("%"strz"/%"strz), binpath, testpath);

				moddir[l] = 0;
				debug1("Looking for module dir: %s", moddir);

				dir = _topendir(moddir);
				_tclosedir(dir);

				if(dir != NULL) break; /* found it! */
				else{ free(moddir); moddir=NULL; }
			}
		}
	}
	debug1("module dir: %s", moddir != NULL ? moddir : __T("<nil>"));
    #if (_UNICODE && WIN32)
    free (defaultdir);
    #endif
	return moddir;
}

/* Open a module */
mpg123_module_t*
open_module( const TCHAR* type, const TCHAR* name )
{
	lt_dlhandle handle = NULL;
	mpg123_module_t *module = NULL;
	TCHAR* module_path = NULL;
	size_t module_path_len = 0;
	char* module_symbol = NULL;
	size_t module_symbol_len = 0;
	TCHAR *workdir = NULL;
	TCHAR *moddir  = NULL;
    char *char_module_path = NULL;  /**< Translate module_path to char for lt_dlopen */
    char *char_type = NULL; /**< Translate type For lt_dlsym to understand module_symbol */
	workdir = get_the_cwd();
	moddir  = get_module_dir();
	if(workdir == NULL || moddir == NULL)
	{
		error("Failure getting workdir or moddir!");
		if(workdir != NULL) free(workdir);
		if(moddir  != NULL) free(moddir);
		return NULL;
	}

	/* Initialize libltdl */
	if (lt_dlinit()) error( "Failed to initialise libltdl" );

	if(_tchdir(moddir) != 0)
	{
		error2("Failed to enter module directory %"strz": %s", moddir, strerror(errno));
		goto om_bad;
	}
	/* Work out the path of the module to open */
	module_path_len = _tcslen(type) + 1 + _tcslen(name) + _tcslen(MODULE_FILE_SUFFIX) + 1;
	module_path = malloc( module_path_len * sizeof(TCHAR) );
	if (module_path == NULL) {
		error1( "Failed to allocate memory for module name: %s", strerror(errno) );
		goto om_bad;
	}
	_sntprintf( module_path, module_path_len, __T("%"strz"_%s%"strz), type, name, MODULE_FILE_SUFFIX );

	/* Display the path of the module created */
	debug1( "Module path: %"strz, module_path );
    
    #if (WIN32 && _UNICODE) /**Translate so libtool lt_dlopen can understand */
    char_module_path = (char *)malloc (module_path_len);
    char_type = (char *)malloc (_tcslen(type)+1);
    if (!(char_module_path && char_type))
    error("Path conversion failed!");
    else
    {
      WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, module_path, -1, char_module_path, module_path_len, NULL, NULL);
      WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, type, -1, char_type, _tcslen(type)+1, NULL, NULL);
    }
    #else
    char_module_path = (char *)module_path;
    char_type = (char *)type;
    #endif

	/* Open the module */
	handle = lt_dlopen( char_module_path );
	free( module_path );
	if (handle==NULL) {
		error2( "Failed to open module %"strz": %s", name, lt_dlerror() );
		goto om_bad;
	}
	
	/* Work out the symbol name */
	module_symbol_len = strlen ( MODULE_SYMBOL_PREFIX ) +
						strlen( char_type )  +
						strlen( MODULE_SYMBOL_SUFFIX ) + 1;
	module_symbol = malloc(module_symbol_len);
	snprintf( module_symbol, module_symbol_len, "%s%s%s", MODULE_SYMBOL_PREFIX, char_type, MODULE_SYMBOL_SUFFIX );
	debug1( "Module symbol: %s", module_symbol );
	
	/* Get the information structure from the module */
	module = (mpg123_module_t*)lt_dlsym(handle, module_symbol );
	free( module_symbol );
	if (module==NULL) {
		error1( "Failed to get module symbol: %s", lt_dlerror() );
		goto om_latebad;
	}
	
	/* Check the API version */
	if (MPG123_MODULE_API_VERSION > module->api_version) {
		error( "API version of module is too old" );
		goto om_latebad;
	} else if (MPG123_MODULE_API_VERSION > module->api_version) {
		error( "API version of module is too new" );
		goto om_latebad;
	}

	/* Store handle in the data structure */
	module->handle = handle;

	goto om_end;
om_latebad:
	lt_dlclose( handle );
om_bad:
	module = NULL;
om_end:
	_tchdir(workdir);
	free(moddir);
	free(workdir);
    #if (WIN32 && _UNICODE)
    free(char_module_path);
    free(char_type);
    #endif
	return module;
}


void close_module( mpg123_module_t* module )
{
	lt_dlhandle handle = module->handle;
	int err = lt_dlclose( handle );
	
	if (err) error1("Failed to close module: %s", lt_dlerror() );

}

#define PATH_STEP 50
static TCHAR *get_the_cwd()
{
	size_t bs = PATH_STEP;
	TCHAR *buf = malloc(bs * sizeof(TCHAR));
	while((buf != NULL) && _tgetcwd(buf, bs) == NULL)
	{
		TCHAR *buf2;
		buf2 = realloc(buf, bs+=PATH_STEP);
		if(buf2 == NULL){ free(buf); buf = NULL; }
		else debug1("pwd: increased buffer to %lu", (unsigned long)bs);

		buf = buf2;
	}
	return buf;
}

void list_modules()
{
	_TDIR* dir = NULL;
	struct _tdirent *dp = NULL;
	TCHAR *workdir = NULL;
	TCHAR *moddir  = NULL;

	moddir = get_module_dir();
	/* Open the module directory */
	dir = _topendir(moddir);
	if (dir==NULL) {
		error2("Failed to open the module directory (%s): %s\n", PKGLIBDIR, strerror(errno));
		free(moddir);
		exit(-1);
	}
	
	workdir = get_the_cwd();
	if(_tchdir(moddir) != 0)
	{
		error2("Failed to enter module directory (%s): %s\n", PKGLIBDIR, strerror(errno));
		_tclosedir( dir );
		free(workdir);
		free(moddir);
		exit(-1);
	}
	/* Display the program title */
	/* print_title(stderr); */

	/* List the output modules */
	printf("\n");
	printf("Available modules\n");
	printf("-----------------\n");
	
	while( (dp = _treaddir(dir)) != NULL ) {
		struct _stat64i32 fst;
		if(_tstat(dp->d_name, &fst) != 0) continue;
		if(S_ISREG(fst.st_mode)) /* Allow links? */
		{
			TCHAR* ext = dp->d_name + _tcslen( dp->d_name ) - _tcslen( MODULE_FILE_SUFFIX );
			if (_tcscmp(ext, MODULE_FILE_SUFFIX) == 0)
			{
				TCHAR *module_name = NULL;
				TCHAR *module_type = NULL;
				TCHAR *uscore_pos = NULL;
				mpg123_module_t *module = NULL;
				
				/* Extract the module type */
				module_type = _tcsdup( dp->d_name );
				uscore_pos = _tcschr( module_type, '_' );
				if (uscore_pos==NULL) continue;
				if (uscore_pos>=module_type+_tcslen(module_type)+1) continue;
				*uscore_pos = '\0';
				
				/* Extract the short name of the module */
				module_name = _tcsdup( dp->d_name + _tcslen( module_type ) + 1 );
				module_name[ _tcslen( module_name ) - _tcslen( MODULE_FILE_SUFFIX ) ] = __T('\0');
				
				/* Open the module */
				module = open_module( module_type, module_name );
				if (module) {
					_tprintf(__T("%-15"strz"%"strz"  %"strz"\n"), module->name, module_type, module->description );
				
					/* Close the module again */
					close_module( module );
				}
				
				free( module_name );
			}
		}
	}

	_tchdir(workdir);
	free(workdir);
	_tclosedir( dir );
	free(moddir);
	exit(0);
}



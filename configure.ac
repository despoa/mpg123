dnl configure.ac: source for the configure script

dnl copyright by the mpg123 project - free software under the terms of the LGPL 2.1
dnl see COPYING and AUTHORS files in distribution or http://mpg123.de
dnl initially written by Nicholas J. Humfrey

dnl Require autoconf version >= 2.57
AC_PREREQ(2.57)


dnl ############# Initialisation

AC_INIT([mpg123], [0.66], [mpg123-devel@lists.sourceforge.net])

AC_CONFIG_SRCDIR(src/mpg123.c)
AC_CONFIG_AUX_DIR(build)
AC_CANONICAL_TARGET

dnl Version 1.7 of automake is recommended
AM_INIT_AUTOMAKE( 1.7 )
AM_CONFIG_HEADER(src/config.h)



dnl ############# Compiler and tools Checks

AM_PROG_AS
AC_PROG_CC
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S

AM_DISABLE_STATIC
AC_LIBLTDL_INSTALLABLE
AM_PROG_LIBTOOL

AC_C_CONST
AC_C_BIGENDIAN
AC_INLINE

# Check if system supports termios
AC_SYS_POSIX_TERMIOS
if test "x$ac_cv_sys_posix_termios" = "xyes"; then
  AC_DEFINE_UNQUOTED([HAVE_TERMIOS], 1, 
  [Define this if you have the POSIX termios library])
fi



dnl ############## Configurable Options

AC_ARG_ENABLE(debug,
              [  --enable-debug=[no/yes] turn on debugging],
              [
                if test "x$enableval" = xyes
                then
                  debugging="enabled"
                  AC_DEFINE(DEBUG, 1, [ Define if debugging is enabled. ])
                else
                  debugging="disabled"
                fi
              ],
              [ debugging="disabled" ]
)

AC_ARG_ENABLE(gapless,
              [  --enable-gapless=[no/yes] turn on gapless (enabled per default)],
              [
                if test "x$enableval" = xyes
                then
                  gapless="enabled"
                  AC_DEFINE(GAPLESS, 1, [ Define if gapless is enabled. ])
                else
                  gapless="disabled"
                fi
              ],
              [
                gapless="enabled"
                AC_DEFINE(GAPLESS, 1, [ Define if gapless is enabled. ])
              ]
)

AC_ARG_WITH([cpu], [
  --with-cpu=generic[_fpu]      Use generic processor code with floating point arithmetic
  --with-cpu=generic_nofpu      Use generic processor code with fixed point arithmetic (p.ex. ARM, experimental)
  --with-cpu=i386[_fpu]         Use code optimized for i386 processors with floating point arithmetic
  --with-cpu=i386_nofpu         Use code optimized for i386 processors with fixed point arithmetic (experimental)
  --with-cpu=i486         Use code optimized for i486 processors
  --with-cpu=i586         Use code optimized for i586 processors
  --with-cpu=i586_dither  Use code optimized for i586 processors with dithering (noise shaping)
  --with-cpu=3dnow        Use code optimized for 3DNow processors
  --with-cpu=3dnowext     Use code optimized for 3DNowExt processors (K6-3+, Athlon)
  --with-cpu=mmx          Use code optimized for MMX processors
  --with-cpu=sse          Use code optimized for SSE processors
  --with-cpu=x86          Pack all x86 opts into one binary (excluding i486)
  --with-cpu=altivec      Use code optimized for Altivec processors (PowerPC G4 and G5)
]) 

AC_ARG_WITH([audio], [
  --with-audio=aix        Use AIX audio output sub-system
  --with-audio=alib       Use Alib audio output sub-system (for HPUX)
  --with-audio=alsa       Use ALSA audio output sub-system
  --with-audio=dummy      Use dummy audio (when no sound card is available)
  --with-audio=esd        Use ESoundD audio output sub-system
  --with-audio=hp         Use HP audio output sub-system
  --with-audio=jack       Use JACK low-latency audio server
  --with-audio=macosx     Use Mac OS X audio output sub-system (CoreAudio)
  --with-audio=mint       Use MinT audio output sub-system (Atari)
  --with-audio=nas        Use NAS audio output (Network Audio System)
  --with-audio=os2        Use OS2 audio output sub-system
  --with-audio=oss        Use OSS audio output sub-system (/dev/dsp)
  --with-audio=portaudio  Use PortAudio audio output sub-system
  --with-audio=sdl        Use SDL audio output sub-system (Simple DirectMedia Layer)
  --with-audio=sgi        Use SGI audio output sub-system (IRIX)
  --with-audio=sun        Use Sun audio output sub-system (/dev/audio)
])

AC_ARG_WITH([optimization], [
  --with-optimization=0   No Optimization
  --with-optimization=1   Limited Optimization (-O) (for gcc)
  --with-optimization=2   Default Optimization (-O2 ...) (for gcc)
  --with-optimization=3   More Optimize than default (-O3 ...) (for gcc)
  --with-optimization=4   Optimize yet more (-O4 ...) (for gcc)
])

AC_ARG_WITH([seektable], [
  --with-seektable=<size> choose size of seek index table (0 disables it), default 1000
])



dnl ############## Plug-ins

# Dummy audio output plug-in is alway supported
output_plugins="dummy"

# Set the default plug-in to dummy for now
default_output_plugin="dummy"
AC_DEFINE_UNQUOTED( DEFAULT_OUTPUT_PLUGIN, "$default_output_plugin", [The default audio output plug-in to use] )

# Location that plug-ins are installed to
if test x$libdir = 'x${exec_prefix}/lib'; then
    if test x$prefix = xNONE; then
        pkglibdir="$ac_default_prefix/lib/$PACKAGE_NAME"
    else
        pkglibdir="$prefix/lib/$PACKAGE_NAME"
    fi
else
    pkglibdir="$libdir/$PACKAGE_NAME"
fi
AC_DEFINE_UNQUOTED([PKGLIBDIR], "$pkglibdir", [Path to installed plug-ins] ) 
AC_SUBST([pkglibdir])



dnl ############## Function Checks

AC_FUNC_MMAP

# Check for sched_setscheduler
AC_CHECK_FUNCS( sched_setscheduler )

# Check for setpriority
AC_CHECK_FUNCS( setpriority )



dnl ############## Library Checks


# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([mx], [powf])

# Check for JACK
PKG_CHECK_MODULES(JACK, jack, output_plugins="$output_plugins jack", HAVE_JACK="no")
AC_SUBST( JACK_CFLAGS )
AC_SUBST( JACK_LDFLAGS )
AC_SUBST( JACK_LIBS )

# Check for EsounD
PKG_CHECK_MODULES(ESOUND, esound, output_plugins="$output_plugins esound", HAVE_ESOUND="no")
AC_SUBST( ESOUND_CFLAGS )
AC_SUBST( ESOUND_LDFLAGS )
AC_SUBST( ESOUND_LIBS )

# Check for SDL
PKG_CHECK_MODULES(SDL, sdl, output_plugins="$output_plugins sdl", HAVE_SDL="no")
AC_SUBST( SDL_CFLAGS )
AC_SUBST( SDL_LDFLAGS )
AC_SUBST( SDL_LIBS )


# Check for NAS
AC_CHECK_LIB( [audio], [AuOpenServer],
	[ AC_CHECK_HEADER( [audio/audiolib.h],
		[ output_plugins="$output_plugins nas" ],
		[ AC_MSG_WARN([Found NAS library but NOT header files on your system]) ] )
	]
)

# Check for PortAudio
AC_CHECK_LIB( [portaudio], [Pa_Initialize],
	[ AC_CHECK_HEADER( [portaudio.h],
		[ output_plugins="$output_plugins portaudio" ],
		[ AC_MSG_WARN([Found PortAudio library but NOT header files on your system]) ] )
	]
)

# Check for ALSA
AC_CHECK_LIB( [asound], [snd_pcm_open],
	[ AC_CHECK_HEADER( [alsa/asoundlib.h],
		[ output_plugins="$output_plugins alsa" ],
		[ AC_MSG_WARN([Found ALSA library but NOT header files on your system]) ] )
	]
)


dnl ############## Header Checks

AC_HEADER_STDC
dnl Is it too paranoid to specifically check for stdint.h and limits.h?
AC_CHECK_HEADERS([stdlib.h string.h unistd.h sched.h sys/ioctl.h stdint.h limits.h])


# OSS Header files
AC_CHECK_HEADERS([sys/soundcard.h linux/soundcard.h machine/soundcard.h])
if test "x${ac_cv_header_sys_soundcard_h}" = "xyes" \
	 -o "x${ac_cv_header_linux_soundcard_h}" = "xyes" \
	 -o "x${ac_cv_header_machine_soundcard_h}" = "xyes";
then
	output_plugins="$output_plugins oss"
fi


# Sun Audio headers
AC_CHECK_HEADERS([sun/audioio.h sys/audioio.h asm/audioio.h sys/audio.h])
if test "x${ac_cv_header_sun_audioio_h}" = "xyes" \
	 -o "x${ac_cv_header_sys_audioio_h}" = "xyes" \
	 -o "x${ac_cv_header_asm_audioio_h}" = "xyes";
then
	output_plugins="$output_plugins sun"
fi


# Mac OS X CoreAudio
AC_CHECK_HEADERS([AudioUnit/AudioUnit.h CoreServices/CoreServices.h AudioToolbox/AudioToolbox.h])
if test "x${ac_cv_header_AudioUnit_AudioUnit_h}" = "xyes" \
	 -a "x${ac_cv_header_CoreServices_CoreServices_h}" = "xyes" \
	 -a "x${ac_cv_header_AudioToolbox_AudioToolbox_h}" = "xyes";
then
	output_plugins="$output_plugins coreaudio"
fi




dnl ############## Choose compiler flags and CPU

# do not assume gcc here, so no flags by default
CFLAGS="$CFLAGS"
LIBS="$LIBS"

# If debugging is enabled then make warnings errors
if test "$debugging" = "enabled"; then
	CFLAGS="$CFLAGS -g"
	# gcc specific...
	if test "$GCC" = "yes"; then
		CFLAGS="$CFLAGS -Wall -pedantic -fno-strict-aliasing"
	fi
fi


case $target in
  arm-*-linux*)
    # check that... perhaps we are better off on arm with kernel math emulation
    cpu_type="generic_nofpu"
  ;;
  i386-*-linux*|i386-*-kfreebsd*-gnu)
    cpu_type="i386_fpu"
  ;;
  i486-*-linux*|i486-*-kfreebsd*-gnu)
    cpu_type="i486"
  ;;
  i586-*-linux*|i586-*-kfreebsd*-gnu)
    cpu_type="x86"
  ;;
  i686-*-linux*|i686-*-kfreebsd*-gnu)
    cpu_type="x86"
  ;;
  *-*-linux*|*-*-kfreebsd*-gnu)
    cpu_type="generic_fpu"
  ;;
  *-apple-darwin*)
    AC_MSG_CHECKING([if CPU type supports AltiVec])
    case `machine` in
      ppc7400 | ppc7450 | ppc970)
        AC_MSG_RESULT([yes])
        cpu_type="altivec"
      ;;
      *)
        AC_MSG_RESULT([no])
        cpu_type="generic_fpu"
      ;;
    esac
  ;;
  i386-*-freebsd*)
    cpu_type="i386_fpu"
  ;;
  *-*-freebsd*)
    cpu_type="generic_fpu"
  ;;
  *-*-solaris*)
    cpu_type="generic_fpu"
    LIBS="$LIBS -lsocket -lnsl"
  ;;
  i686-pc-cygwin*)
    cpu_type="x86"
  ;;
  i586-pc-cygwin*)
    cpu_type="x86"
  ;;
  i486-pc-cygwin*)
    cpu_type="i486"
  ;;
  i386-pc-cygwin*)
    cpu_type="i386"
  ;;
  *-pc-cygwin*)
    cpu_type="generic_fpu"
  ;;
  *)
  	AC_MSG_WARN([Unknown target operating system])
    cpu_type="generic_fpu"
    CFLAGS="$CFLAGS -DGENERIC"
  ;;
esac


dnl Did user choose other CPU type ?
if test "x$with_cpu" != "x"; then
	cpu_type=$with_cpu
fi


dnl CPU specific compiler flags
case $cpu_type in
  generic)
    CFLAGS="$CFLAGS -DOPT_GENERIC -DREAL_IS_FLOAT"
  ;;
  generic_fpu)
    CFLAGS="$CFLAGS -DOPT_GENERIC -DREAL_IS_FLOAT"
  ;;
  generic_nofpu)
    CFLAGS="$CFLAGS -DOPT_GENERIC -DREAL_IS_FIXED"
  ;;
  altivec)
    CFLAGS="$CFLAGS -maltivec -faltivec -DREAL_IS_FLOAT -DOPT_ALTIVEC"
  ;;
  i386) 
    CFLAGS="$CFLAGS -DOPT_I386 -DREAL_IS_FLOAT"
  ;;
  i386_fpu) 
    CFLAGS="$CFLAGS -DOPT_I386 -DREAL_IS_FLOAT"
  ;;
  i386_nofpu) 
    CFLAGS="$CFLAGS -DOPT_I386 -DREAL_IS_FIXED"
  ;;
  i486) 
    CFLAGS="$CFLAGS -DOPT_I486 -DREAL_IS_FLOAT"
  ;;
  i586) 
    CFLAGS="$CFLAGS -DOPT_I586 -DREAL_IS_FLOAT"
  ;;
  i586_dither) 
    CFLAGS="$CFLAGS -DOPT_I586_DITHER -DREAL_IS_FLOAT"
  ;;
  3dnow)
    # legacy 3dnow had the 3dnow optional... keeping that
    CFLAGS="$CFLAGS -DOPT_MULTI -DOPT_I586 -DOPT_3DNOW -DREAL_IS_FLOAT"
  ;;
  3dnowext) 
    CFLAGS="$CFLAGS -DOPT_3DNOWEXT -DREAL_IS_FLOAT"
  ;;
  mmx) 
    CFLAGS="$CFLAGS -DOPT_MMX -DREAL_IS_FLOAT"
  ;;
  sse) 
    CFLAGS="$CFLAGS -DOPT_SSE -DREAL_IS_FLOAT"
  ;;
  x86) 
    CFLAGS="$CFLAGS -DOPT_MULTI -DOPT_GENERIC -DOPT_I386 -DOPT_I586 -DOPT_I586_DITHER -DOPT_MMX -DOPT_3DNOW -DOPT_3DNOWEXT -DOPT_SSE -DREAL_IS_FLOAT"
  ;;
  *)
  	AC_MSG_ERROR([Unknown CPU type '$cpu_type'])
  ;;
esac

CPU_TYPE_LIB="libcpu_$cpu_type.a"
AC_SUBST( CPU_TYPE_LIB )


dnl ############## Create list of plugins/modules to build

MODULE_LIST=`echo "$output_plugins" | $AWK 'BEGIN{RS=" "}{printf("output_%s ",$1)}'`
AC_SUBST(MODULE_LIST)


dnl ############## Compiler Optimizations

# None chosen?
if test "x$with_optimization" = "x"; then
	if test "$debugging" = "enabled"; then
		with_optimization="0"
	else
		# enable (gcc specific) default opts only with gcc
		test "$GCC" = yes && with_optimization="2"
	fi
fi

case $with_optimization in
  0)
    # No Optimizations
    CFLAGS="$CFLAGS"
  ;;
  1)
    CFLAGS="$CFLAGS -O"
  ;;
  2)
    CFLAGS="$CFLAGS -O2"
    CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-all-loops"
    CFLAGS="$CFLAGS -finline-functions -ffast-math"
  ;;
  3)
    CFLAGS="$CFLAGS -O3"
    CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-all-loops"
    CFLAGS="$CFLAGS -finline-functions -ffast-math"
  ;;
  4)
    CFLAGS="$CFLAGS -O4"
    CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-all-loops"
    CFLAGS="$CFLAGS -finline-functions -ffast-math"
  ;;
  *)
  	AC_MSG_ERROR([Unknown optimizations level '$with_optimization'])
  ;;
esac

dnl ############## Seektable size

if test "x$with_seektable" = "x"; then
	seektable=1000
else
	seektable=$with_seektable
fi

AC_DEFINE_UNQUOTED(INDEX_SIZE, $seektable, [size of the frame index seek table])




dnl ############## Final Output

AC_CONFIG_FILES([
	Makefile
	src/Makefile 
	modules/Makefile
	modules/output_dummy/Makefile
	modules/output_coreaudio/Makefile
	modules/output_sdl/Makefile
])

AC_OUTPUT



dnl ############## Display Message

echo "
  $PACKAGE_NAME $PACKAGE_VERSION

  Install path ............ $prefix
  CPU Optimisation......... $cpu_type
  Compiler Optimization ... $with_optimization
  Gapless Support ......... $gapless
  Debugging ............... $debugging
  Seek table size ......... $seektable

  Plugin Directory ........ $pkglibdir
  Audio output plugins .... $output_plugins
  Default output plugin ... $default_output_plugin
"
 
if test "$debugging" = "enabled"; then
	echo ""
	echo "CFLAGS='$CFLAGS'"
	echo "LIBS='$LIBS'"
	echo "MODULE_LIST='$MODULE_LIST'"
else
    echo "Next type 'make' and then 'make install'."
fi



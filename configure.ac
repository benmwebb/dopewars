dnl Process this file with autoconf to produce a configure script.
AC_INIT([dopewars], [SVN])
AC_CONFIG_SRCDIR([src/dopewars.c])
AC_CONFIG_AUX_DIR(auxbuild)

AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET
AC_USE_SYSTEM_EXTENSIONS

dnl Initialize automake
dnl 'foreign' because we have README.md instead of README
AM_INIT_AUTOMAKE([foreign])

dnl Write configuration defines into config.h
AM_CONFIG_HEADER([src/config.h])

dnl We need this for the Darwin test, plus gettext uses it anyway
AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_ISC_POSIX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h stdlib.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_TIME
AC_STRUCT_TM

dnl Fix Apple's stupid C preprocessor
case "$host_os" in darwin*)
   CPPFLAGS="$CPPFLAGS -no-cpp-precomp"
esac

dnl Process client options
AC_ARG_ENABLE(gui-client,
[  --enable-gui-client     include graphical client (GTK+/Win32)],
[ GUI_CLIENT="$enableval" ],[ GUI_CLIENT="probe" ])

AC_ARG_ENABLE(curses-client,
[  --enable-curses-client  include curses client],
[ CURSES_CLIENT="$enableval" ],[ CURSES_CLIENT="probe" ])

AC_ARG_ENABLE(gui-server,
[  --enable-gui-server     use a simple GTK+/Win32 GUI for the server],
[ GUI_SERVER="$enableval" ],[ GUI_SERVER="probe" ])

AC_ARG_ENABLE(gtk4,
[  --enable-gtk4           use GTK 4 if available (experimental)],
[ USE_GTK4="$enableval" ],[ USE_GTK4="no" ])

AC_ARG_ENABLE(gtk3,
[  --disable-gtk3          use GTK+ 2.x, even if 3.x is found],
[ USE_GTK3="$enableval" ],[ USE_GTK3="yes" ])

AC_ARG_WITH(esd,
[  --without-esd           do not support ESD sound output],
[ USE_ESD="$withval" ], [ USE_ESD="probe" ])

AC_ARG_WITH(sdl,
[  --without-sdl           do not support SDL sound output],
[ USE_SDL="$withval" ], [ USE_SDL="probe" ])

if test "$host_vendor" = "apple" ; then
  APPLE=yes
  AC_DEFINE(APPLE, 1, [Are we building on an Apple Mac?])
  default_cocoa="yes"
else
  APPLE=no
  default_cocoa="no"
fi
AM_CONDITIONAL(APPLE, test "$APPLE" = "yes")

AC_ARG_WITH(cocoa,
[  --without-cocoa         do not support Cocoa (Mac) sound output],
[ USE_COCOA="$withval" ], [ USE_COCOA="${default_cocoa}" ])

ESD=no
SDL=no
COCOA=no

dnl Test for Cygwin environment
AC_CYGWIN
dnl Let the user override this with the --enable-nativewin32 option
AC_ARG_ENABLE(nativewin32,
[  --enable-nativewin32    build a native Win32 binary under Cygwin],
[ CYGWIN="$enableval" ])

if test "$CYGWIN" = "yes" ; then
   AC_MSG_RESULT([Configuring for native Win32 binary under Cygwin])
   AC_DEFINE(CYGWIN, 1, [Define if building under the Cygwin environment])

   dnl Otherwise we get the error
   dnl 'conditional "am__fastdepOBJC" was never defined'
   am__fastdepOBJC_TRUE='#'
   am__fastdepOBJC_FALSE=

   AC_CHECK_TOOL([WINDRES], [windres], [windres])
   dnl This flag allows linking with MSVC-generated DLLs. -fnative-struct was
   dnl used by gcc 2, and -mms-bitfields by gcc 3, so it is tested for here.
   bkp_CFLAGS="$CFLAGS"
   AC_MSG_CHECKING(for compiler MSVC compatibility flag)
   CFLAGS="$CFLAGS -mms-bitfields"
   AC_COMPILE_IFELSE([ AC_LANG_PROGRAM() ], [ mscompat="-mms-bitfields" ],
                     [ mscompat="-fnative-struct" ])
   AC_MSG_RESULT($mscompat)
   CFLAGS="$bkp_CFLAGS"

   dnl If using older Cygwin -mno-cygwin is included in CPPFLAGS so the
   dnl right headers are detected (in /usr/include/mingw/ rather than
   dnl /usr/include) later on - e.g. sys/param.h isn't in mingw.
   bkp_CFLAGS="$CFLAGS"
   AC_MSG_CHECKING(for no-Cygwin flag)
   CFLAGS="$CFLAGS -no-cygwin"
   AC_COMPILE_IFELSE([ AC_LANG_PROGRAM() ], [ nocyg="-no-cygwin" ],
                     [ nocyg="" ])
   AC_MSG_RESULT($nocyg)
   CFLAGS="$bkp_CFLAGS"

   dnl Libraries and flags
   CFLAGS="$CFLAGS -mwindows $mscompat $nocyg"
   CPPFLAGS="$CPPFLAGS $nocyg"
   LIBS="$LIBS -lwsock32 -lcomctl32 -luxtheme -lmpr"
   LDFLAGS="$LDFLAGS $nocyg"

   AM_PATH_GLIB_2_0(2.0.0, , [AC_MSG_ERROR(GLib is required)])

   dnl Find libcurl for metaserver support
   dnl 7.17.0 or later is needed as prior versions did not copy input strings
   LIBCURL_CHECK_CONFIG(7.17.0)

   dnl We need to compile and then link in the Windows resources
   WNDRES="dopewars.res"
   AC_SUBST(WNDRES)

   dnl Add support for the Windows multimedia system
   SOUND_LIBS="$SOUND_LIBS -lwinmm"
   PLUGOBJS="$PLUGOBJS plugins/sound_winmm.o"
   AC_DEFINE(HAVE_WINMM, 1, [Do we have the Windows multimedia system?])

   dnl Use graphical server by default
   if test "$GUI_SERVER" = "probe"; then
     GUI_SERVER="yes"
   fi

   dnl Read high score files, docs and locale files from current directory
   datadir="."
   localstatedir="."
else
   AC_MSG_RESULT([Configuring for Unix binary])

   dnl On true Unix systems, test for valid curses-like libraries
   if test "$CURSES_CLIENT" != "no"; then
      AX_WITH_CURSES
      if test "$ax_cv_curses" != "yes"; then
         AC_CHECK_LIB(cur_colr,newterm)
         if test "$ac_cv_lib_cur_colr_newterm" != "yes"; then
            AC_MSG_WARN(Cannot find any curses-type library)
            CURSES_CLIENT="no"
         fi
      fi
   fi

   gtk2_found="probe"
   if test "$GUI_CLIENT" != "no" -o "$GUI_SERVER" != "no"; then
      PKG_PROG_PKG_CONFIG()
      gtk4_found="no"
      if test "$USE_GTK4" = "yes" ; then
        PKG_CHECK_MODULES([GTK], [gtk4], gtk4_found="yes", gtk4_found="no")
      fi
      if test "$gtk4_found" = "no" ; then
        gtk3_found="no"
        if test "$USE_GTK3" = "yes" ; then
          PKG_CHECK_MODULES([GTK], [gtk+-3.0], gtk3_found="yes", gtk3_found="no")
        fi
        if test "$gtk3_found" = "no" ; then
          AM_PATH_GTK_2_0(2.0.0,gtk2_found="yes",gtk2_found="no")
          if test "$gtk2_found" = "no" ; then
            if test "$GUI_CLIENT" = "yes" -o "$GUI_SERVER" = "yes" ; then
               AC_MSG_ERROR(Cannot find GTK+)
            else
               AC_MSG_WARN(Cannot find GTK+; not building GUI client or server)
               GUI_CLIENT="no"
               GUI_SERVER="no"
            fi
          fi
        fi
      fi
   fi

   dnl We NEED glib
   AM_PATH_GLIB_2_0(2.0.0, , [AC_MSG_ERROR(GLib is required)])

   dnl Find libcurl for metaserver support
   dnl 7.17.0 or later is needed as prior versions did not copy input strings
   LIBCURL_CHECK_CONFIG(7.17.0)

   dnl Add esound support if available
   if test "$USE_ESD" != "no"; then
     AM_PATH_ESD(0.0.20, ESD=yes)
     if test "$ESD" = "yes"; then
       SOUND_CFLAGS="$SOUND_CFLAGS $ESD_CFLAGS"
       SOUND_LIBS="$SOUND_LIBS $ESD_LIBS"
       PLUGOBJS="$PLUGOBJS plugins/sound_esd.o"
       AC_SUBST(ESD_LIBS)
       AC_DEFINE(HAVE_ESD, 1, [Do we have the ESD sound library?])
     elif test "$USE_ESD" = "yes"; then
       AC_MSG_ERROR(Cannot find ESD library)
     fi
   fi

   dnl Add SDL_mixer sound support if available
   if test "$USE_SDL" != "no"; then
     SDL_ALL=no
     SDL2=yes
     AM_PATH_SDL2(2.0.0, SDL=yes)
     if test "$SDL" != "yes"; then
       SDL2=no
       AM_PATH_SDL(1.0.0, SDL=yes)
     fi
     if test "$SDL" = "yes"; then
       headers=no
       libs=no
       ORIG_CPPFLAGS="$CPPFLAGS"
       ORIG_LDFLAGS="$LDFLAGS"
       CPPFLAGS="$ORIG_CPPFLAGS $SDL_CFLAGS"
       LDLAGS="$ORIG_LDFLAGS $SDL_LIBS"
       AC_CHECK_HEADER(SDL_mixer.h, headers=yes)
       if test "$SDL2" = "yes"; then
         AC_CHECK_LIB(SDL2_mixer, Mix_OpenAudio, libs=yes)
       else
         AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio, libs=yes)
       fi
       CPPFLAGS="$ORIG_CPPFLAGS"
       LDFLAGS="$ORIG_LDFLAGS"
       if test "$libs" = "yes"; then
         if test "$headers" = "yes"; then
           SOUND_CFLAGS="$SOUND_CFLAGS $SDL_CFLAGS"
           if test "$SDL2" = "yes"; then
             SDL_LIBS="$SDL_LIBS -lSDL2_mixer"
           else
             SDL_LIBS="$SDL_LIBS -lSDL_mixer"
           fi
           SOUND_LIBS="$SOUND_LIBS $SDL_LIBS"
           PLUGOBJS="$PLUGOBJS plugins/sound_sdl.o"
           AC_SUBST(SDL_LIBS)
           AC_DEFINE(HAVE_SDL_MIXER, 1,
                     [Do we have the SDL_mixer sound library?])
           SDL_ALL=yes
           if test "$APPLE" = "yes"; then
             SDL_LIBS="$SDL_LIBS -module"
	   fi
         fi
       fi
     fi
     if test "$USE_SDL" = "yes" -a "$SDL_ALL" = "no"; then
       AC_MSG_ERROR(Cannot find SDL library)
     fi
   fi

   dnl Add Cocoa (Mac) sound support if on Mac and so requested
   if test "$USE_COCOA" != "no"; then
     AC_PROG_OBJC
     COCOA="yes"
     PLUGOBJS="$PLUGOBJS plugins/sound_cocoa.o"
     AC_DEFINE(HAVE_COCOA, 1, [Do we have the Cocoa sound library?])
   else
     dnl Otherwise we get the error
     dnl 'conditional "am__fastdepOBJC" was never defined'
     am__fastdepOBJC_TRUE='#'
     am__fastdepOBJC_FALSE=
   fi

   dnl Use console server by default
   if test "$GUI_SERVER" = "probe"; then
     GUI_SERVER="no"
   fi

   dnl Some systems use int rather than socklen_t as an argument to getsockopt
   AC_MSG_CHECKING([for socklen_t data type])
   AC_TRY_COMPILE([#include <sys/types.h>
                   #include <sys/socket.h>],[socklen_t val],
                  [AC_MSG_RESULT([yes])
                   AC_DEFINE(HAVE_SOCKLEN_T, 1,
                             [Do we have the socklen_t data type?])],
                  [AC_MSG_RESULT([no])])
fi

AM_CONDITIONAL(ESD, test "$ESD" = "yes")
AM_CONDITIONAL(SDL, test "$SDL" = "yes")
AM_CONDITIONAL(COCOA, test "$COCOA" = "yes")

dnl If probing was unsuccessful, these will be set to "no"; therefore,
dnl if still set to "probe" then everything worked, so set to "yes"
if test "$GUI_CLIENT" = "probe"; then
  GUI_CLIENT="yes"
fi
if test "$CURSES_CLIENT" = "probe"; then
  CURSES_CLIENT="yes"
  LIBS="$LIBS $CURSES_LIBS"
  CFLAGS="$CFLAGS $CURSES_CFLAGS"
fi

dnl Do i18n stuff
ALL_LINGUAS="de pl pt_BR fr fr_CA nn es es_ES en_GB"
AM_GNU_GETTEXT([external])
if test "$gt_cv_func_gettext_libintl" = "yes"; then
    LIBS="-lintl $LIBS"
fi

if test "$GUI_CLIENT" = "yes" ; then
   AC_DEFINE(GUI_CLIENT, 1, [Use the graphical client?])
fi

if test "$CURSES_CLIENT" = "yes" ; then
   AC_DEFINE(CURSES_CLIENT, 1, [Use the (n)curses client?])
fi

if test "$GUI_SERVER" = "yes" ; then
   AC_DEFINE(GUI_SERVER, 1, [Use a graphical server?])
fi

dnl Can we use a long long datatype for price_t ?
AC_CHECK_SIZEOF(long long)

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(strdup strstr getopt getopt_long fork issetugid localtime_r gmtime_r)

dnl Enable plugins only if we can find the dlopen function, and
dnl the user does not disable them with --disable-plugins or --disable-shared
AC_ARG_ENABLE(plugins,
[  --enable-plugins        use dynamically-loaded sound modules],
[ plugins="$enableval" ],[ plugins="probe" ])

if test "$enable_shared" = "no" ; then
  plugins="no"
fi

if test "$plugins" != "no" ; then
  AC_SEARCH_LIBS(dlopen, dl)
  AC_CHECK_FUNC(dlopen, [plugins="yes"], [plugins="no"])
fi

if test "$plugins" = "yes" ; then
  AC_DEFINE(PLUGINS, 1, [Define if using dynamically-loaded sound modules])
  plugindir="${libdir}/dopewars"
  AC_SUBST(plugindir)
  DP_EXPAND_DIR(PLUGINDIR, '${plugindir}')
  AC_DEFINE_UNQUOTED(PLUGINDIR, "$PLUGINDIR",
                   [The directory containing the plugins])
  PLUGOBJS=""
else
  PLUGLIBS="$SOUND_LIBS"
  AC_SUBST(PLUGLIBS)
fi
AC_SUBST(PLUGOBJS)
AM_CONDITIONAL(PLUGINS, test "$plugins" = "yes")

dnl Enable networking by default under Win32, but on Unix systems
dnl make it dependent on the availability of select and socket
network="no"
if test "$CYGWIN" = "yes" ; then
   network="yes"
else
   dnl Check for socket and select even if networking gets manually
   dnl disabled below, since select is used if available for
   dnl millisecond sleeping
   AC_SEARCH_LIBS(socket,socket network)
   AC_SEARCH_LIBS(gethostbyname,nsl socket)
   AC_CHECK_FUNCS(socket gethostbyname select)
   if test "$ac_cv_func_select" = "yes" ; then
      if test "$ac_cv_func_socket" = "yes" ; then
         if test "$ac_cv_func_gethostbyname" = "yes" ; then
            network="yes"
         fi
      fi
   fi
fi

dnl Let the user override this with the --enable-networking option
AC_ARG_ENABLE(networking,
[  --enable-networking     dopewars will use TCP/IP to connect to servers],
[ network="$enableval" ])

dnl Inform the user of the status of networking
if test "$network" = "yes" ; then
   AC_DEFINE(NETWORKING, 1,
       [Define if dopewars should use TCP/IP networking to connect to servers])
fi

AC_ARG_ENABLE(strict,
[  --enable-strict         if using gcc, enable extra warnings above -Wall],
[ extrawarnings="$enableval" ])

dnl Enable full warnings if using gcc
if test -n "$GCC"; then
   if test "$extrawarnings" = "yes" ; then
      CFLAGS="$CFLAGS -Wall -Wpointer-arith -Wcast-qual -Wcast-align -Wsign-compare -Waggregate-return -Wredundant-decls -Wnested-externs -Wunused"
   else
      CFLAGS="$CFLAGS -Wall"
   fi
fi

dnl Tell dopewars where the high scores, docs and locale files are
DP_EXPAND_DIR(DPSCOREDIR, '${localstatedir}')
AC_DEFINE_UNQUOTED(DPSCOREDIR, "$DPSCOREDIR",
                   [The directory containing the high score file])
AC_SUBST(DPSCOREDIR)

DP_EXPAND_DIR(DPDATADIR, '${datadir}')
AC_DEFINE_UNQUOTED(DPDATADIR, "$DPDATADIR",
                   [The directory containing the sounds])
AC_SUBST(DPDATADIR)

DP_EXPAND_DIR(DPDOCDIR, '${docdir}')
AC_DEFINE_UNQUOTED(DPDOCDIR, "$DPDOCDIR",
                   [The directory containing the docs])

localedir=${datadir}/locale
AC_SUBST(localedir)
DP_EXPAND_DIR(LOCALEDIR, '${localedir}')
AC_DEFINE_UNQUOTED(LOCALEDIR, "$LOCALEDIR",
                   [The directory containing locale files])

dnl Add in the required clients
AM_CONDITIONAL(GUI_CLIENT, test "$GUI_CLIENT" = "yes")
if test "$GUI_CLIENT" = "yes" ; then
  GUILIB="gui_client/libguiclient.a"
  AC_SUBST(GUILIB)
fi
AM_CONDITIONAL(CURSES_CLIENT, test "$CURSES_CLIENT" = "yes")
if test "$CURSES_CLIENT" = "yes" ; then
  CURSESLIB="curses_client/libcursesclient.a"
  AC_SUBST(CURSESLIB)
fi

dnl Compile in the gtkport stuff for any kind of GUI
AM_CONDITIONAL(GTKPORT, test "$GUI_CLIENT" = "yes" -o "$GUI_SERVER" = "yes")
if test "$GUI_CLIENT" = "yes" -o "$GUI_SERVER" = "yes" ; then
  GTKPORTLIB="gtkport/libgtkport.a"
  AC_SUBST(GTKPORTLIB)
fi

dnl Compile in the cursesport stuff for the curses client only
AM_CONDITIONAL(CURSESPORT, test "$CURSES_CLIENT" = "yes")
if test "$CURSES_CLIENT" = "yes" ; then
  CURSESPORTLIB="cursesport/libcursesport.a"
  AC_SUBST(CURSESPORTLIB)
fi

AC_SUBST(SOUND_CFLAGS)
AC_SUBST(SOUND_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
src/gui_client/Makefile
src/curses_client/Makefile
src/gtkport/Makefile
src/cursesport/Makefile
src/plugins/Makefile
sounds/Makefile
sounds/19.5degs/Makefile
doc/Makefile
doc/help/Makefile
rpm/dopewars.spec
doc/dopewars.6
win32/install.nsi
po/Makefile.in],
[])

echo
echo "dopewars has been configured as follows:"
echo
if test "$CYGWIN" = "yes" ; then
   echo "Building native Windows (Win32) version"
else
   echo "Building Unix version"
fi
echo
echo "CLIENTS"
if test "$CURSES_CLIENT" = "no" -a "$GUI_CLIENT" = "no" ; then
   echo " - No clients will be compiled (binary will be server/AI only)"
else
   if test "$CURSES_CLIENT" = "yes" ; then
      echo " - Text-mode (curses) client"
   fi
   if test "$GUI_CLIENT" = "yes" ; then
      echo " - Graphical (GTK+ or Win32) client"
   fi
fi

echo
if test "$network" = "yes" ; then
   echo "TCP/IP networking support enabled for multi-player games"
   echo
   echo "SERVER"
   if test "$GUI_SERVER" = "yes" ; then
      echo " - Graphical server"
   else
      echo " - Text-mode server"
   fi
else
   echo "Networking support DISABLED; single-player mode only"
fi

echo

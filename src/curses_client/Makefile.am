noinst_LIBRARIES = libcursesclient.a
libcursesclient_a_SOURCES = curses_client.c curses_client.h
libcursesclient_a_DEPENDENCIES = @INTLLIBS@
INCLUDES   = -I../../intl -I${srcdir} -I${srcdir}/.. -I../..
LDADD      = @INTLLIBS@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\"

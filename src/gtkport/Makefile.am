noinst_LIBRARIES = libgtkport.a
libgtkport_a_SOURCES = gtkport.c gtkport.h
libgtkport_a_DEPENDENCIES = @INTLLIBS@
INCLUDES   = -I../../intl -I${srcdir} -I${srcdir}/.. -I../.. @GTK_CFLAGS@
LDADD      = @GTK_LIBS@ @INTLLIBS@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\"

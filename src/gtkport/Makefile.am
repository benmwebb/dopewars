noinst_LIBRARIES = libgtkport.a
libgtkport_a_SOURCES = gtkport.c gtkport.h
libgtkport_a_DEPENDENCIES = @INTLLIBS@
INCLUDES   = @GTK_CFLAGS@ -I.. -I../.. -I.
LDADD      = @GTK_LIBS@ @INTLLIBS@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\"

noinst_LIBRARIES = libguiclient.a
libguiclient_a_SOURCES = gtk_client.c gtk_client.h \
                         optdialog.c optdialog.h \
                         newgamedia.c newgamedia.h dopewars-pill.xpm
libguiclient_a_DEPENDENCIES = @INTLLIBS@
INCLUDES   = -I../../intl -I${srcdir} -I${srcdir}/.. -I../.. @GTK_CFLAGS@
LDADD      = @GTK_LIBS@ @INTLLIBS@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\"

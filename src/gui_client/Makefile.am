noinst_LIBRARIES = libguiclient.a
libguiclient_a_SOURCES = gtk_client.c optdialog.c
libguiclient_a_DEPENDENCIES = @INTLLIBS@
INCLUDES   = @GTK_CFLAGS@ -I.. -I../.. -I.
LDADD      = @GTK_LIBS@ @INTLLIBS@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\"

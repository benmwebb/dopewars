bin_PROGRAMS = dopewars
dopewars_SOURCES = AIPlayer.c serverside.c dopewars.c message.c \
                   curses_client.c gtk_client.c winmain.c \
		   dopeos.c tstring.c gtkport.c
dopewars_DEPENDENCIES = @INTLLIBS@
SUFFIXES = .rc .res
INCLUDES = @GTK_CFLAGS@ -I.. -I.
LDADD = @GTK_LIBS@ @INTLLIBS@
DEFS = @DEFS@ -DLOCALEDIR=\"${localedir}\"

DOCPATH=/usr/doc/${PACKAGE}-${VERSION}/
DOCS= aiplayer.html configfile.html index.html server.html clientplay.html \
      credits.html installation.html servercommands.html commandline.html \
      developer.html metaserver.html windows.html README

install-exec-hook:
	chown root.games ${DESTDIR}${bindir}/dopewars
	chmod 2755 ${DESTDIR}${bindir}/dopewars


bin_PROGRAMS = dopewars
dopewars_SOURCES = AIPlayer.c serverside.c dopewars.c message.c \
                   curses_client.c gtk_client.c win32_client.c \
		   dopeos.c tstring.c
SUFFIXES = .rc .res
INCLUDES = @GTK_CFLAGS@ -I.. -I.
LDADD = @GTK_LIBS@
DEFS = @DEFS@ -DLOCALEDIR=\"${localedir}\"

DOCPATH=/usr/doc/${PACKAGE}-${VERSION}/
DOCS= aiplayer.html configfile.html index.html server.html clientplay.html \
      credits.html installation.html servercommands.html commandline.html \
      developer.html metaserver.html windows.html README

install-exec-hook:
	chown root.games ${bindir}/dopewars
	chmod 2755 ${bindir}/dopewars


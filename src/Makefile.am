bin_PROGRAMS = dopewars
dopewars_SOURCES = AIPlayer.c serverside.c dopewars.c message.c \
                   curses_client.c gtk_client.c win32_client.c \
		   dopeos.c tstring.c @WIN_RC@
dopewars_DEPENDENCIES = @WIN_RES@
SUFFIXES = .rc .res
INCLUDES = @GTK_CFLAGS@ -I.. -I.
LDADD = @GTK_LIBS@ @WIN_RES@
DEFS = @DEFS@ -DLOCALEDIR=\"${localedir}\"

DOCPATH=/usr/doc/${PACKAGE}-${VERSION}/
DOCS= aiplayer.html configfile.html index.html server.html clientplay.html \
      credits.html installation.html servercommands.html commandline.html \
      developer.html metaserver.html windows.html README

@WIN_MAKE_RES@

install-exec-hook:
	chown root.games ${bindir}/dopewars
	chmod 2755 ${bindir}/dopewars


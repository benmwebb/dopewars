bin_PROGRAMS = dopewars
dopewars_SOURCES = AIPlayer.c serverside.c dopewars.c message.c \
                   curses_client.c gtk_client.c winmain.c \
		   dopeos.c tstring.c @GTKPORT_C@
dopewars_DEPENDENCIES = @INTLLIBS@ @GTKPORT_O@ @WNDRES@
INCLUDES   = @GTK_CFLAGS@ -I.. -I.
LDADD      = @GTKPORT_O@ @GTK_LIBS@ @INTLLIBS@ @WNDRES@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\"
PIXDIR     = ${DESTDIR}${datadir}/pixmaps
PIXMAPS    = dopewars-pill.png dopewars-shot.png dopewars-weed.png

install-exec-hook:
	chown root:games ${DESTDIR}${bindir}/dopewars || \
	    chown root:wheel ${DESTDIR}${bindir}/dopewars
	chmod 2755 ${DESTDIR}${bindir}/dopewars

install-data-local:
	${mkinstalldirs} ${PIXDIR}
	${INSTALL} -o root -g 0 -m 0644 ${PIXMAPS} ${PIXDIR}

%.res: %.rc
	windres -O coff -o $@ $<

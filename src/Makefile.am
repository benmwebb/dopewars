bin_PROGRAMS = dopewars
dopewars_SOURCES = AIPlayer.c curses_client.c dopeos.c dopewars.c \
                   error.c gtk_client.c message.c network.c serverside.c \
                   tstring.c winmain.c @GTKPORT_C@
dopewars_DEPENDENCIES = @INTLLIBS@ @GTKPORT_O@ @WNDRES@
INCLUDES   = @GTK_CFLAGS@ -I.. -I.
LDADD      = @GTKPORT_O@ @GTK_LIBS@ @INTLLIBS@ @WNDRES@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\"
PIXDIR     = ${DESTDIR}${datadir}/pixmaps
PIXMAPS    = dopewars-pill.png dopewars-shot.png dopewars-weed.png
DOPEDIR    = ${DESTDIR}${bindir}
DOPEBIN    = ${DOPEDIR}/dopewars

install-exec-hook:
	@chgrp games ${DOPEBIN} || chgrp wheel ${DOPEBIN} || \
	  ( echo "WARNING: Cannot change group of dopewars binary - the high"; \
	    echo "score file may be unreadable or unwriteable by some users" )
	chmod 2755 ${DOPEBIN}

install-data-local:
	${mkinstalldirs} ${PIXDIR}
	${INSTALL} -m 0644 ${PIXMAPS} ${PIXDIR}

%.res: %.rc
	windres -O coff -o $@ $<

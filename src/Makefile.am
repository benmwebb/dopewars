if GUI_CLIENT
GUISUBDIR = gui_client
GUIDEP = gui_client/libguiclient.a
endif

if CURSES_CLIENT
CURSESSUBDIR = curses_client
CURSESDEP = curses_client/libcursesclient.a
endif

if GTKPORT
GTKPORTSUBDIR = gtkport
GTKPORTDEP = gtkport/libgtkport.a
endif

SUBDIRS = $(GUISUBDIR) $(CURSESSUBDIR) $(GTKPORTSUBDIR)
dopewars_DEPENDENCIES = @INTLLIBS@ @WNDRES@ $(GUIDEP) $(CURSESDEP) $(GTKPORTDEP)
dopewars_LDADD = $(GUIDEP) $(CURSESDEP) $(GTKPORTDEP) @GTK_LIBS@ @INTLLIBS@ @WNDRES@

bin_PROGRAMS = dopewars
dopewars_SOURCES = admin.c AIPlayer.c dopeos.c dopewars.c \
                   error.c message.c network.c serverside.c \
                   tstring.c winmain.c

INCLUDES   = -I.. -I. @GTK_CFLAGS@
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

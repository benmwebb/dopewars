# Nasty hack; there seems to be no way of disabling libtool for the link
# of the main executable...
if PLUGINS
LIBTOOL = $(SHELL) $(top_builddir)/libtool
else
LIBTOOL = ./fakelibtool.sh
endif

if GUI_CLIENT
GUISUBDIR = gui_client
endif
if CURSES_CLIENT
CURSESSUBDIR = curses_client
endif
if GTKPORT
GTKPORTSUBDIR = gtkport
endif
if CURSESPORT
CURSESPORTSUBDIR = cursesport
endif

SUBDIRS = $(GUISUBDIR) $(CURSESSUBDIR) $(GTKPORTSUBDIR) $(CURSESPORTSUBDIR) plugins
dopewars_LDADD = @GUILIB@ @CURSESLIB@ @GTKPORTLIB@ @CURSESPORTLIB@ @GTK_LIBS@ @INTLLIBS@ @WNDRES@ @PLUGOBJS@ @PLUGLIBS@
dopewars_DEPENDENCIES = @GUILIB@ @CURSESLIB@ @GTKPORTLIB@ @CURSESPORTLIB@ @INTLLIBS@ @WNDRES@ @PLUGOBJS@

bin_PROGRAMS = dopewars
dopewars_SOURCES = admin.c admin.h AIPlayer.c AIPlayer.h util.c util.h \
                   dopewars.c dopewars.h error.c error.h message.c message.h \
                   network.c network.h nls.h serverside.c serverside.h \
                   sound.c sound.h \
                   tstring.c tstring.h winmain.c winmain.h
INCLUDES   = -I../intl -I${srcdir} -I.. @GTK_CFLAGS@
DEFS       = @DEFS@ -DLOCALEDIR=\"${localedir}\" @PLUGINDEF@
PIXDIR     = ${DESTDIR}${datadir}/pixmaps
DOPEDIR    = ${DESTDIR}${bindir}
DOPEBIN    = ${DOPEDIR}/dopewars
PIXMAPS    = dopewars-pill.png dopewars-shot.png dopewars-weed.png
EXTRA_DIST = ${PIXMAPS} pill.ico magic dopewars.rc
CLEANFILES = dopewars.res dopewars.exe

install-exec-hook:
	@chgrp games ${DOPEBIN} || chgrp wheel ${DOPEBIN} || \
	  ( echo "WARNING: Cannot change group of dopewars binary - the high"; \
	    echo "score file may be unreadable or unwriteable by some users" )
	chmod 2755 ${DOPEBIN}

install-data-local:
	${mkinstalldirs} ${PIXDIR}
	for pix in ${PIXMAPS}; do \
	  ${INSTALL} -m 0644 ${srcdir}/$${pix} ${PIXDIR}; \
	done

%.res: %.rc
	windres -O coff -o $@ $<

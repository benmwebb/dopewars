# Nasty hack; there seems to be no other way of disabling libtool for the
# link of the main executable...
if PLUGINS
MYLINK = $(LIBTOOL) --mode=link $(CCLD)
else
MYLINK = $(CCLD)
endif
LINK = $(MYLINK) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

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
dopewars_LDADD = @GUILIB@ @CURSESLIB@ @GTKPORTLIB@ @CURSESPORTLIB@ @GTK_LIBS@ @INTLLIBS@ @WNDRES@ @PLUGOBJS@ @PLUGLIBS@ @GLIB_LIBS@
dopewars_DEPENDENCIES = @GUILIB@ @CURSESLIB@ @GTKPORTLIB@ @CURSESPORTLIB@ @INTLLIBS@ @WNDRES@ @PLUGOBJS@

bin_PROGRAMS = dopewars
dopewars_SOURCES = admin.c admin.h AIPlayer.c AIPlayer.h util.c util.h \
                   configfile.c configfile.h convert.c convert.h \
                   dopewars.c dopewars.h error.c error.h log.c log.h \
                   message.c message.h network.c network.h nls.h \
                   serverside.c serverside.h sound.c sound.h \
                   tstring.c tstring.h winmain.c winmain.h
INCLUDES   = -I../intl -I${srcdir} -I.. @GLIB_CFLAGS@ @GTK_CFLAGS@
DEFS       = @DEFS@
PIXDIR     = ${DESTDIR}${datadir}/pixmaps
DOPEDIR    = ${DESTDIR}${bindir}
DOPEBIN    = ${DOPEDIR}/dopewars
PIXMAPS    = dopewars-pill.png dopewars-shot.png dopewars-weed.png
EXTRA_DIST = ${PIXMAPS} pill.ico magic dopewars.rc dopewars.manifest
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

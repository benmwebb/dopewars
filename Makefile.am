SUBDIRS    = intl src doc po
DESKTOPDIR = ${DESTDIR}${datadir}/gnome/apps/Games
DESKTOP    = dopewars.desktop
SCOREDIR   = ${DESTDIR}${datadir}
SCORE      = ${SCOREDIR}/dopewars.sco
EXTRA_DIST = ABOUT-NLS LICENCE dopewars.desktop dopewars.spec \
             runindent.sh win32/Makefile win32/README win32/contid.h \
             win32/dialogs.rc win32/filelist win32/guifunc.c win32/guifunc.h \
             win32/makeinstall.c win32/setup.c win32/setup.ico win32/setup.rc \
             win32/uninstall.c win32/uninstall.rc win32/util.c win32/util.h \
             win32/zlib/Makefile.nocygwin win32/zlib/README.zlib
CLEANFILES = dopewars.sco dopewars-log.txt dopewars-config.txt

install-data-local:
	${mkinstalldirs} ${SCOREDIR}
	touch ${SCORE}
	@chgrp games ${SCORE} || chgrp wheel ${SCORE} || \
	  echo "Unable to change group ownership of the high score file"
	chmod 0660 ${SCORE}
	src/dopewars -C ${SCORE}
	${mkinstalldirs} ${DESKTOPDIR}
	${INSTALL} -m 0644 ${srcdir}/${DESKTOP} ${DESKTOPDIR}

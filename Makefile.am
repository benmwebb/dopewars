SUBDIRS    = intl src doc po
DESKTOPDIR = ${DESTDIR}${datadir}/gnome/apps/Games
DESKTOP    = dopewars.desktop
SCOREDIR   = ${DESTDIR}${datadir}
SCORE      = ${SCOREDIR}/dopewars.sco
EXTRA_DIST = ABOUT-NLS LICENCE dopewars.desktop dopewars.spec \
             runindent.sh win32

install-data-local:
	${mkinstalldirs} ${SCOREDIR}
	touch ${SCORE}
	@chgrp games ${SCORE} || chgrp wheel ${SCORE} || \
	  echo "Unable to change group ownership of the high score file"
	chmod 0660 ${SCORE}
	src/dopewars -C ${SCORE}
	${mkinstalldirs} ${DESKTOPDIR}
	${INSTALL} -m 0644 ${srcdir}/${DESKTOP} ${DESKTOPDIR}

SUBDIRS    = intl src doc po
DISTFILES  = ABOUT-NLS
DESKTOPDIR = ${DESTDIR}${datadir}/gnome/apps/Games
DESKTOP    = dopewars.desktop
SCOREDIR   = ${DESTDIR}${datadir}
SCORE      = ${SCOREDIR}/dopewars.sco

install-data-local:
	${mkinstalldirs} ${SCOREDIR}
	touch ${SCORE}
	@chgrp games ${SCORE} || chgrp wheel ${SCORE} || \
	  echo "Unable to change group ownership of the high score file"
	chmod 0660 ${SCORE}
	src/dopewars -C ${SCORE}
	${mkinstalldirs} ${DESKTOPDIR}
	${INSTALL} -m 0644 ${DESKTOP} ${DESKTOPDIR}

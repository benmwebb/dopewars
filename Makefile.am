SUBDIRS    = intl src doc po
DISTFILES  = ABOUT-NLS
DESKTOPDIR = ${DESTDIR}${datadir}/gnome/apps/Games
DESKTOP    = dopewars.desktop
SCOREDIR   = ${DESTDIR}${datadir}
SCORE      = ${SCOREDIR}/dopewars.sco

install-data-local:
	${mkinstalldirs} ${SCOREDIR}
	touch ${SCORE}
	chown root:games ${SCORE} || \
	   chown root:wheel ${SCORE}
	chmod 0660 ${SCORE}
	${mkinstalldirs} ${DESKTOPDIR}
	${INSTALL} -o root -g 0 -m 0644 ${DESKTOP} ${DESKTOPDIR}

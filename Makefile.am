SUBDIRS    = intl src doc po
DISTFILES  = ABOUT-NLS
DESKTOPDIR = ${DESTDIR}${datadir}/gnome/apps/Games
DESKTOP    = dopewars.desktop

install-data-local:
	${mkinstalldirs} ${DESTDIR}${datadir}
	touch ${DESTDIR}${datadir}/dopewars.sco
	chown root:games ${DESTDIR}${datadir}/dopewars.sco || \
	   chown root:wheel ${DESTDIR}${datadir}/dopewars.sco
	chmod 0660 ${DESTDIR}${datadir}/dopewars.sco
	${mkinstalldirs} ${DESKTOPDIR}
	${INSTALL} -o root -g 0 -m 0644 ${DESKTOP} ${DESKTOPDIR}

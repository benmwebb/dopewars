SUBDIRS = src doc po intl

DISTFILES = ABOUT-NLS

install-data-local:
	${mkinstalldirs} ${DESTDIR}${datadir}
	touch ${DESTDIR}${datadir}/dopewars.sco
	chown root.games ${DESTDIR}${datadir}/dopewars.sco
	chmod 0660 ${DESTDIR}${datadir}/dopewars.sco


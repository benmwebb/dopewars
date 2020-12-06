SUBDIRS        = src doc po sounds
DESKTOPDIR     = ${DESTDIR}${datadir}/gnome/apps/Games
DESKTOP        = dopewars.desktop
SCOREDIR       = ${DESTDIR}${localstatedir}
SCORE          = ${SCOREDIR}/dopewars.sco
EXTRA_DIST     = ABOUT-NLS LICENCE dopewars.desktop rpm/dopewars.spec.in \
                 runindent.sh win32/README.md win32/install.nsi.in README.md \
		 win32/mingw/Dockerfile
CLEANFILES     = dopewars.sco dopewars-log.txt dopewars-config.txt
DISTCLEANFILES = rpm/dopewars.spec
ACLOCAL_AMFLAGS= -I m4

install-data-local:
	${mkinstalldirs} ${SCOREDIR}
	touch ${SCORE}
	@chgrp games ${SCORE} || chgrp wheel ${SCORE} || \
	  echo "Unable to change group ownership of the high score file"
	chmod 0660 ${SCORE}
	${mkinstalldirs} ${DESKTOPDIR}
	${INSTALL} -m 0644 ${srcdir}/${DESKTOP} ${DESKTOPDIR}

uninstall-local:
	/bin/rm -f ${SCORE} ${DESKTOPDIR}/${DESKTOP}

SUBDIRS        = intl src doc po sounds
DESKTOPDIR     = ${DESTDIR}${datadir}/gnome/apps/Games
DESKTOP        = dopewars.desktop
SCOREDIR       = ${DESTDIR}${localstatedir}
SCORE          = ${SCOREDIR}/dopewars.sco
EXTRA_DIST     = ABOUT-NLS LICENCE dopewars.desktop dopewars.spec.in depcomp \
                 runindent.sh win32/Makefile win32/README win32/contid.h \
                 win32/dialogs.rc win32/filelist win32/guifunc.c \
                 win32/guifunc.h win32/makeinstall.c win32/setup.c \
                 win32/setup.ico win32/setup.rc win32/uninstall.c \
                 win32/uninstall.rc win32/util.c win32/util.h \
                 win32/zlib/Makefile.nocygwin win32/zlib/README.zlib \
                 win32/setup.manifest win32/uninstall.manifest \
                 win32/bzlib/Makefile.nocygwin win32/bzlib/README.bzlib
CLEANFILES     = dopewars.sco dopewars-log.txt dopewars-config.txt
DISTCLEANFILES = dopewars.spec
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

DOCPATH        = ${DESTDIR}${datadir}/doc/${PACKAGE}-${VERSION}/
DOCS           = aiplayer.html configfile.html index.html i18n.html \
                 server.html clientplay.html credits.html example-cfg \
                 installation.html servercommands.html commandline.html \
                 contribute.html developer.html metaserver.html \
                 protocol.html windows.html ../LICENCE
man_MANS       = dopewars.6
SUBDIRS        = help
EXTRA_DIST     = dopewars.6.in ${DOCS}
DISTCLEANFILES = dopewars.6

install-data-local:
	${INSTALL} -d -m 0755 ${DOCPATH}
	for doc in ${DOCS}; do \
	  ${INSTALL} -m 0644 ${srcdir}/$${doc} ${DOCPATH}; \
	done

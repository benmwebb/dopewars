DOCPATH    = ${DESTDIR}${datadir}/doc/${PACKAGE}-${VERSION}/
DOCS       = aiplayer.html configfile.html index.html i18n.html server.html \
             clientplay.html credits.html example-cfg installation.html \
             servercommands.html commandline.html developer.html \
             metaserver.html protocol.html windows.html
man_MANS   = dopewars.6
EXTRA_DIST = ${man_MANS} ${DOCS}

install-data-local:
	${INSTALL} -d -m 0755 ${DOCPATH}
	for doc in ${DOCS}; do \
	  ${INSTALL} -m 0644 ${srcdir}/$${doc} ${DOCPATH}; \
	done


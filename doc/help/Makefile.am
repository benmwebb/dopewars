DOCPATH    = ${DESTDIR}${datadir}/doc/${PACKAGE}-${VERSION}/help
DOCS       = cops.html general.html locations.html sounds.html \
             drugs.html guns.html server.html
EXTRA_DIST = ${DOCS}

install-data-local:
	${INSTALL} -d -m 0755 ${DOCPATH}
	for doc in ${DOCS}; do \
	  ${INSTALL} -m 0644 ${srcdir}/$${doc} ${DOCPATH}; \
	done

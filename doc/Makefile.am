DOCPATH=${DESTDIR}${datadir}/doc/${PACKAGE}-${VERSION}/
DOCS= aiplayer.html configfile.html index.html i18n.html server.html \
      clientplay.html credits.html installation.html \
      servercommands.html commandline.html developer.html \
      metaserver.html windows.html
man_MANS = dopewars.6
EXTRA_DIST = ${man_MANS}

install-data-local:
	${INSTALL} -d -m 0755 ${DOCPATH}
	${INSTALL} -m 0644 ${DOCS} ${DOCPATH}


DOCPATH=${datadir}/doc/${PACKAGE}-${VERSION}/
DOCS= aiplayer.html configfile.html index.html i18n.html server.html \
      clientplay.html credits.html installation.html \
      servercommands.html commandline.html developer.html \
      metaserver.html windows.html

install-data-local:
	${INSTALL} -d -o root -g root -m 0755 ${DESTDIR}${DOCPATH}
	${INSTALL} -o root -g root -m 0644 ${DOCS} ${DESTDIR}${DOCPATH}


# Use these flags to use libbz2 for compression
#LIBZ      = bzlib/libbz2.a
#COMPFLAGS = -DLIBBZ2

# Use these flags to use libz for compression
LIBZ      = zlib/libz.a
COMPFLAGS =

CFLAGS = -Wall -mno-cygwin ${COMPFLAGS}

all: setup makeinstall uninstall

clean:
	/bin/rm -f *.o *.res uninstall.exe setup.exe makeinstall.exe core *~
	/bin/rm -f installfiles.z manifest

setup: setup.o util.o guifunc.o setup.res
	gcc ${CFLAGS} -mwindows -o setup setup.o util.o guifunc.o setup.res ${LIBZ} -lcomctl32 -lole32 -luuid
	strip setup.exe

uninstall: uninstall.o util.o guifunc.o uninstall.res
	gcc ${CFLAGS} -mwindows -o uninstall uninstall.o util.o guifunc.o uninstall.res 
	strip uninstall.exe

uninstall.o: uninstall.c util.h guifunc.h
	gcc ${CFLAGS} -c uninstall.c

uninstall.res: uninstall.rc
	windres -O coff -o uninstall.res uninstall.rc

setup.o: setup.c contid.h util.h guifunc.h
	gcc ${CFLAGS} -c setup.c

util.o: util.c util.h
	gcc ${CFLAGS} -c util.c

guifunc.o: guifunc.c guifunc.h
	gcc ${CFLAGS} -c guifunc.c

manifest installfiles.z: filelist makeinstall uninstall
	sed -e 's/LICENCE/licence.txt/' < ../doc/index.html > index.html
	sed -e 's/LICENCE/licence.txt/' < ../doc/developer.html > developer.html
	sed -e 's/example-cfg/example-cfg.txt/' < ../doc/configfile.html > configfile.html
	awk '{gsub(/\f/, ""); print $$0."\r"}' < ../LICENCE > licence.txt
	awk '{print $$0."\r"}' < ../README > readme.txt
	awk '{print $$0."\r"}' < ../doc/example-cfg > example-cfg.txt
	./makeinstall
	/bin/rm -f index.html configfile.html developer.html licence.txt
	/bin/rm -f example-cfg.txt readme.txt

setup.res: setup.rc dialogs.rc contid.h manifest installfiles.z
	windres -O coff -o setup.res setup.rc

makeinstall: makeinstall.o util.o
	gcc ${CFLAGS} -o makeinstall makeinstall.o util.o ${LIBZ}

makeinstall.o: makeinstall.c util.h
	gcc ${CFLAGS} -c makeinstall.c

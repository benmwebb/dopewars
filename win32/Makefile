all: setup makeinstall uninstall

clean:
	/bin/rm -f *.o *.res uninstall.exe setup.exe makeinstall.exe core *~
	/bin/rm -f installfiles.gz manifest

setup: setup.o util.o guifunc.o setup.res
	gcc -Wall -mno-cygwin -mwindows -o setup setup.o util.o guifunc.o setup.res zlib/libz.a -lcomctl32 -lole32 -luuid
	strip setup.exe

uninstall: uninstall.o util.o guifunc.o uninstall.res
	gcc -Wall -mno-cygwin -mwindows -o uninstall uninstall.o util.o guifunc.o uninstall.res 
	strip uninstall.exe

uninstall.o: uninstall.c util.h guifunc.h
	gcc -Wall -mno-cygwin -c uninstall.c

uninstall.res: uninstall.rc
	windres -O coff -o uninstall.res uninstall.rc

setup.o: setup.c contid.h util.h guifunc.h
	gcc -Wall -mno-cygwin -c setup.c

util.o: util.c util.h
	gcc -Wall -mno-cygwin -c util.c

guifunc.o: guifunc.c guifunc.h
	gcc -Wall -mno-cygwin -c guifunc.c

manifest installfiles.gz: filelist makeinstall uninstall
	./makeinstall

setup.res: setup.rc dialogs.rc contid.h manifest installfiles.gz
	windres -O coff -o setup.res setup.rc

makeinstall: makeinstall.o util.o
	gcc -Wall -mno-cygwin -o makeinstall makeinstall.o util.o zlib/libz.a

makeinstall.o: makeinstall.c util.h
	gcc -Wall -mno-cygwin -c makeinstall.c

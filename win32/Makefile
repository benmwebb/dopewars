all: setup makeinstall uninstall

setup: setup.o setup.res
	gcc -Wall -mno-cygwin -mwindows -o setup setup.o setup.res zlib/libz.a -lcomctl32 -lole32 -luuid

uninstall: uninstall.o uninstall.res
	gcc -Wall -mno-cygwin -o uninstall uninstall.o uninstall.res 

uninstall.o: uninstall.c
	gcc -Wall -mno-cygwin -c uninstall.c

uninstall.res: uninstall.rc
	windres -O coff -o uninstall.res uninstall.rc

setup.o: setup.c contid.h
	gcc -Wall -mno-cygwin -c setup.c

setup.res: setup.rc dialogs.rc contid.h manifest installfiles.gz
	windres -O coff -o setup.res setup.rc

makeinstall: makeinstall.c
	gcc -Wall -mno-cygwin -o makeinstall makeinstall.c zlib/libz.a

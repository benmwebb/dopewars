# Unit tests for the game code, built and run by "make check". Each test
# program uses the GLib testing framework and links against the game
# code in src/libdopewars.a.
TESTS = test-message test-serverside
check_PROGRAMS = ${TESTS}

COMMON = common.c common.h
test_message_SOURCES = test-message.c ${COMMON}
test_serverside_SOURCES = test-serverside.c ${COMMON}

AM_CPPFLAGS = -I${top_srcdir}/src -I${top_builddir}/src \
              @GLIB_CFLAGS@ @GTK_CFLAGS@ @LIBCURL_CPPFLAGS@

# The library is listed twice because the GUI server code in it uses
# gtkport, which in turn uses the library.
DOPEWARSLIB = ${top_builddir}/src/libdopewars.a
LDADD = @PLUGOBJS@ ${DOPEWARSLIB} @GTKPORTLIB@ ${DOPEWARSLIB} \
        @GTK_LIBS@ @LTLIBINTL@ @PLUGLIBS@ @GLIB_LIBS@ @LIBCURL@
# Link without libtool, which would drop the repeated library
LINK = ${CCLD} ${AM_CFLAGS} ${CFLAGS} ${AM_LDFLAGS} ${LDFLAGS} -o $@
if APPLE
AM_LDFLAGS = -framework AppKit
endif

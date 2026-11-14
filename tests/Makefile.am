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

# End-to-end tests of the server and clients, in e2e/. These need Python 3
# and pytest (plus pyte, and on Unix pexpect, for the curses client tests,
# which are skipped without them), so aren't run by "make check"; run
# "make check-e2e" instead. The GUI client tests need a
# build with --enable-test-hooks, and a display. Extra pytest arguments
# can be given in PYTEST_ARGS.
PYTEST = python3 -m pytest
E2E_FILES = e2e/conftest.py e2e/cursesclient.py e2e/dopeproto.py \
            e2e/fakeserver.py e2e/guiclient.py e2e/pytest.ini \
            e2e/terminal.py e2e/test_admin.py e2e/test_ai.py \
            e2e/test_ai_client.py e2e/test_curses.py \
            e2e/test_curses_fake.py e2e/test_gui.py e2e/test_gui_fake.py \
            e2e/test_hostile.py e2e/test_players.py e2e/test_terminal.py \
            e2e/test_trading.py
EXTRA_DIST = ${E2E_FILES}

check-e2e:
	cd ${srcdir}/e2e && \
	  DOPEWARS_BINARY=${abs_top_builddir}/src/dopewars${EXEEXT} \
	  ${PYTEST} ${PYTEST_ARGS}

.PHONY: check-e2e

lib_LTLIBRARIES = libsound_esd.la libsound_sdl.la libsound_winmm.la
libsound_esd_la_SOURCES = sound_esd.c sound_esd.h
libsound_esd_la_LDFLAGS = @ESD_LIBS@
libsound_sdl_la_SOURCES = sound_sdl.c sound_sdl.h
libsound_sdl_la_LDFLAGS = @SDL_LIBS@
libsound_winmm_la_SOURCES = sound_winmm.c sound_winmm.h
LIBS = @GLIB_LIBS@
INCLUDES = @SOUND_CFLAGS@ @GLIB_CFLAGS@

if PLUGINS
PLUGINDIR = ${DESTDIR}${plugindir}
PLUGINS   = .libs/libsound_sdl.so .libs/libsound_esd.so .libs/libsound_winmm.so

install-exec-local:
	${mkinstalldirs} ${PLUGINDIR}
	for plug in ${PLUGINS}; do \
	  ${INSTALL} ${srcdir}/$${plug} ${PLUGINDIR}; \
	done
endif

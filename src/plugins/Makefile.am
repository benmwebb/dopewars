noinst_LTLIBRARIES = libsound_esd.la libsound_sdl.la libsound_winmm.la
libsound_esd_la_SOURCES = sound_esd.c sound_esd.h
libsound_esd_la_LDFLAGS = @ESD_LIBS@
libsound_sdl_la_SOURCES = sound_sdl.c sound_sdl.h
libsound_sdl_la_LDFLAGS = @SDL_LIBS@
libsound_winmm_la_SOURCES = sound_winmm.c sound_winmm.h
LIBS = @GLIB_LIBS@
INCLUDES = @SOUND_CFLAGS@ @GLIB_CFLAGS@
LINKNOO = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS)

if PLUGINS
PLUGINDIR = ${DESTDIR}${plugindir}
if ESD
ESD_SO = .libs/libsound_esd.so
endif
if SDL
SDL_SO = .libs/libsound_sdl.so
endif
PLUGINS   = $(ESD_SO) $(SDL_SO)

all-local: ${PLUGINS}

.libs/libsound_esd.so: $(libsound_esd_la_OBJECTS)
	$(LINKNOO) -o libsound_esd.la -rpath $(libdir) $(libsound_esd_la_LDFLAGS) $(libsound_esd_la_OBJECTS) $(libsound_esd_la_LIBADD) $(LIBS)

.libs/libsound_sdl.so: $(libsound_sdl_la_OBJECTS)
	$(LINKNOO) -o libsound_sdl.la -rpath $(libdir) $(libsound_sdl_la_LDFLAGS) $(libsound_sdl_la_OBJECTS) $(libsound_sdl_la_LIBADD) $(LIBS)

.libs/libsound_winmm.so: $(libsound_winmm_la_OBJECTS)
	$(LINKNOO) -o libsound_winmm.la -rpath $(libdir) $(libsound_winmm_la_LDFLAGS) $(libsound_winmm_la_OBJECTS) $(libsound_winmm_la_LIBADD) $(LIBS)

install-exec-am:
	${mkinstalldirs} ${PLUGINDIR}
	for plug in ${PLUGINS}; do \
	  ${INSTALL} ${srcdir}/$${plug} ${PLUGINDIR}; \
	done
endif

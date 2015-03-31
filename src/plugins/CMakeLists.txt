if (HAVE_ESD)
  add_library(libsound_esd MODULE sound_esd.c)
endif()

if (HAVE_SDL_MIXER)
  add_library(libsound_sdl MODULE sound_sdl.c)
endif()

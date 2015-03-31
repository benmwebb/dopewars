cmake_minimum_required(VERSION 2.8)

project(dopewars)
set(VERSION "SVN")

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake_modules)
include(${PROJECT_SOURCE_DIR}/cmake_modules/FindGLIB.cmake)
include(${PROJECT_SOURCE_DIR}/cmake_modules/FindDL.cmake)

# Find source headers
include_directories("${PROJECT_SOURCE_DIR}/src")

# Find generated config.h
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CONFIG_H")
include_directories("${PROJECT_BINARY_DIR}")

check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(stdlib.h HAVE_STDLIB_H)

# Check for socket and select even if networking gets manually
# disabled below, since select is used if available for
# millisecond sleeping
check_function_exists(socket HAVE_SOCKET)
check_function_exists(gethostbyname HAVE_GETHOSTBYNAME)
check_function_exists(select HAVE_SELECT)
if (${HAVE_SOCKET} AND ${HAVE_GETHOSTBYNAME} AND ${HAVE_SELECT})
  set(HAVE_NETWORK ON)
else()
  set(HAVE_NETWORK OFF)
endif()

# Process client options
option(GUI_CLIENT "include graphical client (GTK+/Win32)" ON)
option(CURSES_CLIENT "include curses client" ON)
option(GUI_SERVER "use a simple GTK+/Win32 GUI for the server" "probe")
option(NETWORKING
       "should dopewars use TCP/IP networking to connect to servers?"
       ${HAVE_NETWORK})

option(WITH_ESD "support ESD sound output" ON)
option(WITH_SDL "support SDL sound output" ON)
option(PLUGINS "use dynamically-loaded sound modules" ON)

find_package(GLIB)
if (!GLIB_FOUND)
  message(FATAL_ERROR "GLib is required")
else()
  set(EXTRA_LIBS ${EXTRA_LIBS} ${GLIB_LIBRARIES})
  include_directories(${GLIB_INCLUDE_DIRS})
endif()

if (CURSES_CLIENT)
  find_package(Curses)
  if (CURSES_FOUND)
    add_subdirectory(src/curses_client)
    add_subdirectory(src/cursesport)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${CURSES_LIBRARIES} cursesclient cursesport)
    include_directories(${CURSES_INCLUDE_DIRS})
  else()
    message(WARNING "Cannot find any curses-type library")
    set(CURSES_CLIENT OFF)
  endif()
endif()

if (GUI_CLIENT OR GUI_SERVER)
  find_package(GTK2 2.0 COMPONENTS gtk)
  if (GTK2_FOUND)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${GTK2_LIBRARIES})
    include_directories(${GTK2_INCLUDE_DIRS})
    if (GUI_CLIENT)
      add_subdirectory(src/gui_client)
      set(EXTRA_LIBS ${EXTRA_LIBS} guiclient)
    endif()
    add_subdirectory(src/gtkport)
    set(EXTRA_LIBS ${EXTRA_LIBS} gtkport)
  else()
    message(WARNING "Cannot find GTK+; not building GUI client or server")
    set(GUI_CLIENT OFF)
    set(GUI_SERVER OFF)
  endif()
endif()

if (PLUGINS)
  find_package(DL)
  if (DL_FOUND)
    include_directories(${DL_INCLUDE_DIR})
    set(EXTRA_LIBS ${EXTRA_LIBS} ${DL_LIBRARIES})
  else()
    set(PLUGINS OFF)
  endif()
endif()

if (PLUGINS)
  # Add esound support if available
  find_package(ESD)
  if (ESD_FOUND)
    include_directories(${ESD_INCLUDE_DIR})
    set(SOUND_LIBS ${SOUND_LIBS} ${ESD_LIBRARY})
    set(HAVE_ESD ON)
  endif()

  # Add SDL_mixer sound support if available
  find_package(SDL_mixer)
  if (SDL_MIXER_FOUND)
    include_directories(${SDL_MIXER_INCLUDE_DIRS})
    set(SOUND_LIBS ${SOUND_LIBS} ${SDL_MIXER_LIBRARIES})
    set(HAVE_SDL_MIXER ON)
  endif()
endif()

add_subdirectory("src/plugins")

configure_file("${PROJECT_SOURCE_DIR}/config-cmake.h.in"
               "${PROJECT_BINARY_DIR}/config.h")

add_executable(dopewars
               "${PROJECT_SOURCE_DIR}/src/AIPlayer.c"
               "${PROJECT_SOURCE_DIR}/src/error.c"
               "${PROJECT_SOURCE_DIR}/src/sound.c"
               "${PROJECT_SOURCE_DIR}/src/admin.c"
               "${PROJECT_SOURCE_DIR}/src/log.c"
               "${PROJECT_SOURCE_DIR}/src/tstring.c"
               "${PROJECT_SOURCE_DIR}/src/configfile.c"
               "${PROJECT_SOURCE_DIR}/src/message.c"
               "${PROJECT_SOURCE_DIR}/src/util.c"
               "${PROJECT_SOURCE_DIR}/src/convert.c"
               "${PROJECT_SOURCE_DIR}/src/network.c"
               "${PROJECT_SOURCE_DIR}/src/dopewars.c"
               "${PROJECT_SOURCE_DIR}/src/serverside.c")

target_link_libraries(dopewars ${EXTRA_LIBS})

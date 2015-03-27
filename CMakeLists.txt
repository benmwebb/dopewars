cmake_minimum_required(VERSION 2.8)

project(dopewars)
set(VERSION SVN)

include(CheckIncludeFiles)

# Find generated config.h
include_directories("${PROJECT_BINARY_DIR}")

check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(stdlib.h HAVE_STDLIB_H)

# Process client options
option(GUI_CLIENT "include graphical client (GTK+/Win32)" "probe")
option(CURSES_CLIENT "include curses client" "probe")
option(GUI_SERVER "use a simple GTK+/Win32 GUI for the server" "probe")

option(WITH_ESD "support ESD sound output" "probe")
option(WITH_SDL "support SDL sound output" "probe")

configure_file("${PROJECT_SOURCE_DIR}/config-cmake.h.in"
               "${PROJECT_BINARY_DIR}/config.h")

add_executable(dopewars "${PROJECT_SOURCE_DIR}/src/*.c")
target_link_libraries(dopewars ${EXTRA_LIBS})

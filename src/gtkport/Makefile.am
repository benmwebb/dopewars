noinst_LIBRARIES = libgtkport.a
libgtkport_a_SOURCES = gtkport.c gtkport.h gtkenums.h \
                       unicodewrap.c unicodewrap.h treeview.h treeview.c \
                       itemfactory.c itemfactory.h gtktypes.h
AM_CPPFLAGS= -I${srcdir} -I$(top_srcdir)/src @GTK_CFLAGS@ @GLIB_CFLAGS@
DEFS       = @DEFS@

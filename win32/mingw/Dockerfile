FROM fedora:33
MAINTAINER Ben Webb <benwebb@users.sf.net>
RUN echo "zchunk=0" >> /etc/dnf/dnf.conf && dnf -y install mingw64-curl mingw64-glib2 mingw64-pkg-config mingw32-curl mingw32-glib2 mingw32-pkg-config automake autoconf which git make gettext diffutils mingw32-nsis

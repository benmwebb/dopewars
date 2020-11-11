## Building dopewars on Windows

(This is experimental.)

dopewars is built for Windows via cross-compilation on Linux using the
MinGW tools. See the `mingw` subdirectory for a suitable `Dockerfile` to set up
a [Docker](https://www.docker.com/) or [Podman](https://podman.io/)
compilation environment.

Once in the environment, build dopewars for 64-bit Windows with

    ./configure --host=x86_64-w64-mingw32 --enable-nativewin32 && make

For 32-bit Windows, use

    ./configure --host=i686-w64-mingw32 --enable-nativewin32 && make

In order for curl connections to the metaserver to work, copy
`/etc/pki/tls/certs/ca-bundle.crt` from the Docker/Podman environment to
the same directory as `dopewars.exe`.


## Windows installer

This directory contains the code for a simple Windows install/uninstall
package. The `makeinstall` program takes a list of files (in `filelist`) to
install, and produces a compressed copy of these files, which are placed
into the resources of the `setup` program. This program installs the listed
files to a target machine, and sets up the necessary registry keys for the
`uninstall` program to then be able to remove those files again. The only
file that should need to be distributed, therefore, is `setup.exe`.

## Building dopewars on Windows

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

The top-level `configure` script will also generate `install.nsi` in this
directory. This can be used as input to
[NSIS](https://nsis.sourceforge.io/) to build a Windows installer.

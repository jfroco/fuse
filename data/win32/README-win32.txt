Fuse for Windows
================

This is a binary distribution of Fuse (the Free Unix Spectrum Emulator) for
Windows. For an introduction to Fuse, see README.txt. For more detailed
documentation, read fuse.html.

Requirements
------------

The 32-bit build runs on a supported 32-bit or 64-bit Windows system. The
64-bit build (packages whose names end in -x64) requires a 64-bit Windows
system.

Libraries
---------

These binaries are built with MinGW-w64. Depending on the build, the selected
UI and the enabled features, the package may include DLLs from these
components:

    bzip2
    iconv (win-iconv or GNU libiconv)
    libgcrypt
    libgpg-error
    libpng
    libspectrum
    libstdc++ (32-bit build)
    libxml2
    winpthreads
    xz / liblzma (32-bit build)
    zlib
    SDL2

The DLLs actually bundled in this package are listed in LICENSES.txt, together
with their copyright and license details.

Building
--------

The 32-bit build uses the Cygwin MinGW-w64 packages of the Windows CI
workflow; see:
https://sourceforge.net/p/fuse-emulator/wiki/Building%20Fuse%20on%20Windows%20with%20Cygwin/

The 64-bit build is cross-compiled from Linux with the development container
in the .devcontainer directory of the Fuse source tree; see
.devcontainer/build-windows.sh.

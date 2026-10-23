FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. Herramientas de build (autotools, perl para el código generado de Fuse y
#    libspectrum), compilador MinGW-w64 x86_64 (modelo posix) y herramientas de
#    empaquetado usadas por data/win32/distribution.mk (groff para fuse.html,
#    unix2dos, zip, 7z).
RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    bison \
    build-essential \
    ca-certificates \
    curl \
    dos2unix \
    file \
    flex \
    gawk \
    ghostscript \
    git \
    gpg \
    gpgv \
    groff \
    libtool \
    make \
    netpbm \
    p7zip-full \
    patch \
    perl \
    pkg-config \
    psutils \
    python3 \
    sed \
    tar \
    unzip \
    wget \
    xz-utils \
    zip \
    zstd \
    gcc-mingw-w64-x86-64-posix \
    g++-mingw-w64-x86-64-posix \
    binutils-mingw-w64-x86-64 \
    && rm -rf /var/lib/apt/lists/*

# El workspace montado desde Windows tiene otro dueño que root
RUN git config --global --add safe.directory '*'

# 2. Clonar quasi-msys2 en modo MINGW64
#    Usar el repo principal: mirror.msys2.org redirige cada descarga a un mirror distinto
#    (a veces desincronizados) y la verificación de firmas falla al azar.
WORKDIR /opt/quasi-msys2
RUN git clone https://github.com/HolyBlackCat/quasi-msys2.git . && \
    sed -i 's|^MIRROR_URL := https://mirror.msys2.org$|MIRROR_URL := https://repo.msys2.org|' Makefile && \
    grep -q '^MIRROR_URL := https://repo.msys2.org$' Makefile && \
    echo "MINGW64" > msystem.txt

# 3. Instalar las librerías MINGW64 que usan Fuse y libspectrum, y crear el symlink /mingw64
#    La lista se define en devcontainer.json (build.args.QUASI_MSYS2_PACKAGES); este valor es solo el respaldo.
#    El caché de BuildKit conserva los paquetes descargados entre rebuilds.
ARG QUASI_MSYS2_PACKAGES="_zlib _bzip2 _xz _libiconv _libpng _libxml2 _libgcrypt _libgpg-error _SDL2"
RUN --mount=type=cache,id=quasi-msys2-mingw64-fuse,target=/opt/quasi-msys2/cache,sharing=locked \
    make install $QUASI_MSYS2_PACKAGES && \
    ln -nfs /opt/quasi-msys2/root/mingw64 /mingw64

# 4. Compilador que usará el entorno de quasi-msys2 (env/vars.src) para CC/CXX:
#    el MinGW GCC de Ubuntu vía los wrappers win-gcc / win-g++.
ENV WIN_DEFAULT_COMPILER=native_gcc

# 5. Script de entorno para terminales interactivas y tareas de VS Code (BASH_ENV)
RUN printf '%s\n' \
    'if [ -z "$QUASI_MSYS2_ROOT" ]; then' \
    '  export QUASI_MSYS2_QUIET=1' \
    '  source /opt/quasi-msys2/env/vars.src >/dev/null' \
    '  export PKG_CONFIG_PATH="/opt/libspectrum/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"' \
    'fi' > /etc/profile.d/mingw-env.sh && \
    echo 'source /etc/profile.d/mingw-env.sh' >> /root/.bashrc

ENV BASH_ENV=/etc/profile.d/mingw-env.sh
SHELL ["/bin/bash", "-c"]

# 6. libspectrum desde git: Fuse usa funciones que todavía no están en ningún
#    release (p. ej. los bloques TZX CSW de tape_block.c). Fijar el commit hace
#    el build reproducible; para actualizarlo, cambia LIBSPECTRUM_REF en
#    devcontainer.json y reconstruye el contenedor.
#    Variables de caché de configure:
#    - lt_cv_deplibs_check_method=pass_all: en compilación cruzada libtool no
#      reconoce las librerías de importación de MSYS2 y, como libspectrum se
#      enlaza con -no-undefined, terminaría generando solo la librería estática.
#    - ac_cv_search_iconv_open=-liconv: el libiconv de MSYS2 exporta
#      libiconv_open (iconv.h redefine iconv_open), así que la prueba de
#      configure, que no incluye iconv.h, no lo encuentra.
ARG LIBSPECTRUM_REPO=https://git.code.sf.net/p/fuse-emulator/libspectrum
ARG LIBSPECTRUM_REF=4018215cdb8a1674de09947af98520bf243b1658
RUN git init /opt/src/libspectrum && \
    cd /opt/src/libspectrum && \
    git remote add origin "$LIBSPECTRUM_REPO" && \
    git fetch --depth 1 origin "$LIBSPECTRUM_REF" && \
    git checkout FETCH_HEAD && \
    ./autogen.sh && \
    ./configure \
      --host=x86_64-w64-mingw32 \
      --prefix=/opt/libspectrum \
      --with-fake-glib \
      --disable-static \
      lt_cv_deplibs_check_method=pass_all \
      ac_cv_search_iconv_open=-liconv && \
    make -j"$(nproc)" && \
    make install-strip && \
    install -D -m 644 COPYING /opt/libspectrum/share/licenses/libspectrum/COPYING && \
    git rev-parse HEAD > /opt/libspectrum/share/licenses/libspectrum/REVISION && \
    rm -rf /opt/src

WORKDIR /workspaces

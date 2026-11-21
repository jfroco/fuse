# Entorno de compilación cruzada de Fuse para Nintendo Wii (powerpc-eabi).
#
# La imagen oficial de devkitPro trae devkitPPC (powerpc-eabi-gcc), libogc,
# libfat, wiiuse, libwiikeyboard y los portlibs de PPC que usa Fuse (zlib,
# bzip2, libpng). Fijar la etiqueta hace el build reproducible; para
# actualizarla, cambia DEVKITPPC_TAG en devcontainer.json y reconstruye.
ARG DEVKITPPC_TAG=20261102
FROM devkitpro/devkitppc:${DEVKITPPC_TAG}

ENV DEBIAN_FRONTEND=noninteractive

# 1. Autotools (la imagen no las trae), perl para el código generado de Fuse
#    y libspectrum, y bison/flex para el debugger de Fuse.
RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    bison \
    file \
    flex \
    libtool \
    && rm -rf /var/lib/apt/lists/*

# El workspace montado desde Windows tiene otro dueño que root
RUN git config --global --add safe.directory '*'

# 2. Variables de entorno de Wii de devkitPro (CFLAGS, CPPFLAGS, LDFLAGS, LIBS,
#    PORTLIBS_WII, PATH con powerpc-eabi-*) para terminales interactivas y
#    tareas de VS Code (BASH_ENV).
RUN printf '%s\n' \
    'if [ -z "$PORTLIBS_WII" ]; then' \
    '  source "$DEVKITPRO/wiivars.sh"' \
    'fi' > /etc/profile.d/wii-env.sh && \
    echo 'source /etc/profile.d/wii-env.sh' >> /root/.bashrc

ENV BASH_ENV=/etc/profile.d/wii-env.sh
SHELL ["/bin/bash", "-c"]

# 3. libspectrum desde git, el mismo commit que el contenedor de Windows (Fuse
#    usa funciones que todavía no están en ningún release). Se instala como
#    librería estática en los portlibs de Wii, donde powerpc-eabi-pkg-config la
#    encuentra sin configuración adicional.
ARG LIBSPECTRUM_REPO=https://git.code.sf.net/p/fuse-emulator/libspectrum
ARG LIBSPECTRUM_REF=4018215cdb8a1674de09947af98520bf243b1658
RUN git init /opt/src/libspectrum && \
    cd /opt/src/libspectrum && \
    git remote add origin "$LIBSPECTRUM_REPO" && \
    git fetch --depth 1 origin "$LIBSPECTRUM_REF" && \
    git checkout FETCH_HEAD && \
    ./autogen.sh && \
    ./configure \
      --host=powerpc-eabi \
      --prefix="$PORTLIBS_WII" \
      --with-fake-glib \
      --without-libgcrypt \
      --enable-static \
      --disable-shared && \
    make -j"$(nproc)" && \
    make install && \
    install -D -m 644 COPYING "$PORTLIBS_WII/share/licenses/libspectrum/COPYING" && \
    git rev-parse HEAD > "$PORTLIBS_WII/share/licenses/libspectrum/REVISION" && \
    rm -rf /opt/src

WORKDIR /workspaces

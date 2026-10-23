#!/usr/bin/env bash
# Build 64-bit Windows packages of Fuse inside the devcontainer.
#
# Usage: .devcontainer/build-windows.sh [win32] [sdl2]
#   With no arguments both variants are built. Each variant is configured in
#   its own build directory and packaged as dist/windows/fuse-<version>-<ui>-x64.zip
#
# Environment:
#   FUSE_BUILD_ROOT  build directories (default: /tmp/fuse-build-windows)
#   FUSE_DIST_DIR    output directory  (default: <source>/dist/windows)
#   JOBS             parallel make jobs (default: nproc)

set -euo pipefail

src_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_root=${FUSE_BUILD_ROOT:-/tmp/fuse-build-windows}
dist_dir=${FUSE_DIST_DIR:-$src_dir/dist/windows}
jobs=${JOBS:-$(nproc)}
host=x86_64-w64-mingw32
libspectrum_prefix=/opt/libspectrum
quasi_dir=/opt/quasi-msys2

variants=("$@")
if [ "${#variants[@]}" -eq 0 ]; then
  variants=(win32 sdl2)
fi

for variant in "${variants[@]}"; do
  case "$variant" in
    win32|sdl2) ;;
    *) echo "Unknown variant: $variant (expected win32 or sdl2)" >&2; exit 1 ;;
  esac
done

if [ -z "${QUASI_MSYS2_ROOT:-}" ]; then
  # shellcheck disable=SC1091
  source /etc/profile.d/mingw-env.sh
fi

# Out-of-tree builds need a source tree that has not been configured in place
if [ -f "$src_dir/config.status" ]; then
  echo "ERROR: $src_dir has been configured in-tree; run 'make distclean' there first." >&2
  exit 1
fi

if [ ! -x "$src_dir/configure" ] || [ "$src_dir/configure.ac" -nt "$src_dir/configure" ]; then
  echo "Running autogen.sh .."
  (cd "$src_dir" && ./autogen.sh)
fi

# data/win32/check_win32_dll_deps.sh calls plain objdump; point it at the
# MinGW one so it reads PE import tables regardless of the host binutils.
tools_dir=$(mktemp -d)
trap 'rm -rf "$tools_dir"' EXIT
ln -s "$(command -v $host-objdump)" "$tools_dir/objdump"

copy_notice() {
  local file=$1 name=$2

  case "$name" in
    *.txt|*.TXT) ;;
    *) name=$name.txt ;;
  esac
  cp "$file" "$notice_dir/$name"
}

# Copy the license files of the packages that provide the bundled DLLs. The
# owning MSYS2 package of each DLL is looked up in quasi-msys2's file index.
copy_notices() {
  local package_dir=$1 dll index file found
  notice_dir=$package_dir/3rd-party

  mkdir -p "$notice_dir"
  for dll in "$package_dir"/*.dll; do
    dll=$(basename "$dll")
    found=0

    case "$dll" in
      libspectrum-*.dll)
        for file in "$libspectrum_prefix/share/licenses/libspectrum"/*; do
          copy_notice "$file" "libspectrum-$(basename "$file")"
          found=1
        done
        ;;
      *)
        index=$(grep -lxF "mingw64/bin/$dll" "$quasi_dir"/index/*) || {
          echo "ERROR: no quasi-msys2 package provides $dll" >&2
          exit 1
        }
        while IFS= read -r file; do
          [ -f "$quasi_dir/root/$file" ] || continue
          # share/licenses/<package>/<path> -> <package>-<path with dashes>
          file=${file#mingw64/share/licenses/}
          copy_notice "$quasi_dir/root/mingw64/share/licenses/$file" "${file//\//-}"
          found=1
        done < <(grep '^mingw64/share/licenses/' "$index")
        ;;
    esac

    if [ "$found" -eq 0 ]; then
      echo "ERROR: no license files found for $dll" >&2
      exit 1
    fi
  done
}

build_variant() {
  local variant=$1 build_dir configure_params version package_dir

  case "$variant" in
    win32)
      configure_params=(--with-win32 --with-audio-driver=directsound --disable-sdl2)
      ;;
    sdl2)
      configure_params=(--without-win32 --with-sdl --with-audio-driver=sdl)
      ;;
  esac

  build_dir=$build_root/$variant
  echo "=== Building $variant UI in $build_dir"
  mkdir -p "$build_dir"
  cd "$build_dir"

  # WINDRES: use the quasi-msys2 wrapper, which adds /mingw64/include so the
  # resource compiler can preprocess libspectrum.h (it includes gcrypt.h)
  "$src_dir/configure" \
    --host=$host \
    --prefix=/usr/local \
    --disable-static \
    "${configure_params[@]}" \
    WINDRES=windres \
    2>&1 | tee configure.out

  # Fail early if configure picked something other than what was requested
  grep -q "User interface: $variant" configure.out
  grep -q "libxml2 support: yes" configure.out
  case "$variant" in
    win32) grep -q "Selected audio driver: directsound" configure.out ;;
    sdl2)
      grep -q "Selected audio driver: sdl" configure.out
      grep -q "Using SDL: 2" configure.out
      ;;
  esac

  make -j"$jobs"

  version=$(sed -n 's/^PACKAGE_VERSION = //p' Makefile)
  package_dir=fuse-$version-$variant

  # Install the executable and data files, then pull in the DLLs it needs
  rm -rf "$package_dir"
  make dist-win32-dir
  PATH="$tools_dir:$PATH" \
    WIN32_DLL_SEARCH_DIRS="$libspectrum_prefix/bin /mingw64/bin" \
    bash "$src_dir/data/win32/check_win32_dll_deps.sh" "$package_dir"

  copy_notices "$package_dir"

  # dist-win32-zip reinstalls over the same directory, now with the DLLs in
  # place so LICENSES.txt keeps exactly their entries, and zips it
  make dist-win32-zip

  mkdir -p "$dist_dir"
  cp "$package_dir.zip" "$dist_dir/fuse-$version-$variant-x64.zip"
  (cd "$dist_dir" && sha1sum "fuse-$version-$variant-x64.zip" > "fuse-$version-$variant-x64.zip.sha1")

  echo "=== $variant package: $dist_dir/fuse-$version-$variant-x64.zip"
  echo "DLLs:"
  (cd "$package_dir" && ls -1 ./*.dll | sed 's|^\./|  |')
}

for variant in "${variants[@]}"; do
  build_variant "$variant"
done

## This file does not need automake. Include in the final Makefile.
## Copyright (c) 2013-2016 Sergio Baldoví

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: serbalgi@gmail.com

package_win32=$(PACKAGE)-$(PACKAGE_VERSION)-$(UI)
top_win32dir=$(top_builddir)/$(package_win32)

install-win32: all
	test -n "$(DESTDIR)" || { echo "ERROR: set DESTDIR path"; exit 1; }
	$(MKDIR_P) $(DESTDIR)/roms/ || exit 1
	$(MKDIR_P) $(DESTDIR)/lib/ || exit 1
	$(MKDIR_P) $(DESTDIR)/3rd-party/ || exit 1
	test "$(UI)" != "sdl2" || $(MKDIR_P) $(DESTDIR)/ui/widget/ || exit 1
	cp $(top_srcdir)/roms/*.rom $(DESTDIR)/roms
	cp $(top_srcdir)/roms/README.copyright $(DESTDIR)/roms
	cp $(top_srcdir)/lib/*.bmp $(DESTDIR)/lib
	cp $(top_srcdir)/lib/*.png $(DESTDIR)/lib
	cp $(top_srcdir)/lib/*.scr $(DESTDIR)/lib
	test "$(UI)" != "sdl2" || cp $(top_builddir)/ui/widget/fuse.font $(DESTDIR)/ui/widget
#	Copy fuse executable (we should manually copy the required libraries)
	cp $(top_builddir)/.libs/fuse$(EXEEXT) $(DESTDIR) || \
	cp $(top_builddir)/fuse$(EXEEXT) $(DESTDIR)
#	Get text files
	for file in AUTHORS ChangeLog COPYING README; \
	  do cp "$(top_srcdir)/$$file" "$(DESTDIR)/$$file.txt"; \
	done
	cp "$(top_srcdir)/data/win32/README-win32.txt" "$(DESTDIR)/README-win32.txt"
	cp "$(top_srcdir)/data/win32/LICENSES.txt" "$(DESTDIR)/LICENSES.txt"
#	Keep only the DLL entries of LICENSES.txt that match a bundled DLL
	@dlls=`cd "$(DESTDIR)" && ls *.dll 2>/dev/null`; \
	awk -v dlls="$$dlls" '\
	  BEGIN { n = split( dlls, present ); } \
	  /^Files: / { \
	    skip = 0; files = ""; \
	    for( i = 2; i <= NF; i++ ) { \
	      if( $$i !~ /\.dll$$/ ) { files = ""; break; } \
	      star = index( $$i, "*" ); \
	      prefix = star ? substr( $$i, 1, star - 1 ) : $$i; \
	      suffix = star ? substr( $$i, star + 1 ) : ""; \
	      for( j = 1; j <= n; j++ ) { \
	        name = present[j]; \
	        if( star ? ( length( name ) >= length( prefix ) + length( suffix ) && \
	                     substr( name, 1, length( prefix ) ) == prefix && \
	                     substr( name, length( name ) - length( suffix ) + 1 ) == suffix ) \
	                 : name == $$i ) { files = files " " $$i; break; } \
	      } \
	      if( i == NF && files == "" ) skip = 1; \
	    } \
	    if( files != "" ) $$0 = "Files:" files; \
	  } \
	  !skip { print }' \
	  "$(DESTDIR)/LICENSES.txt" > "$(DESTDIR)/LICENSES.txt.tmp" && \
	mv "$(DESTDIR)/LICENSES.txt.tmp" "$(DESTDIR)/LICENSES.txt"
#	Get manuals
	if test -n "$(GROFF)"; then \
	  sed ':a;N;$$!ba;s/\.PP\n\.TS/\.bp\n&/g' $(top_srcdir)/man/fuse.1 | \
	  $(GROFF) -t -Thtml -man -P -I -P manual > $(DESTDIR)/fuse.html; \
	elif test -n "$(MAN2HTML)"; then \
	  $(MAN2HTML) -r $(top_srcdir)/man/fuse.1 | sed '1d' > $(DESTDIR)/fuse.html; \
	fi
	-mv manual*.png $(DESTDIR);
#	Convert to DOS line endings
	test -z "$(UNIX2DOS)" || find $(DESTDIR) -type f \( -name "*.txt" -or -name "*.html" -or -name "*.copyright" \) -exec $(UNIX2DOS) {} \;

install-win32-strip: install-win32
	test -z "$(STRIP)" || $(STRIP) $(DESTDIR)/fuse$(EXEEXT)

dist-win32-dir:
	$(MAKE) DESTDIR="$(top_win32dir)" install-win32-strip

dist-win32-zip: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	zip -q -9 -r $(abs_top_builddir)/$(package_win32).zip .
	-sha1sum $(top_builddir)/$(package_win32).zip > $(top_builddir)/$(package_win32).zip.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).zip.sha1; }

dist-win32-7z: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
	cd $(top_win32dir) && \
	7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -bd $(abs_top_builddir)/$(package_win32).7z .
	-sha1sum $(top_builddir)/$(package_win32).7z > $(top_builddir)/$(package_win32).7z.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32).7z.sha1; }

dist-win32-exe: dist-win32-dir
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe.sha1
	test -n "$(top_win32dir)" || exit 1
	@test `find $(top_win32dir) -type f -name \*.dll -print | wc -l` -ne 0 || \
	{ echo "ERROR: external libraries not found in $(top_win32dir). Please, manually copy them."; exit 1; }
#	Locate NSIS in system path, MSYS drive or Cygwin drive
	@NSISFILE="$(abs_top_builddir)/data/win32/installer.nsi"; \
	if makensis -VERSION > /dev/null 2>&1; then \
	  MAKENSIS="makensis"; \
	elif [ -x "/c/Program Files/NSIS/makensis" ]; then \
	  MAKENSIS="/c/Program\ Files/NSIS/makensis"; \
	elif [ -x "/cygdrive/c/Program Files/NSIS/makensis" ]; then \
	  MAKENSIS="/cygdrive/c/Program\ Files/NSIS/makensis"; \
	else \
	  echo 'ERROR: cannot locate makensis tool'; exit 1; \
	fi; \
	case "`uname -s`" in \
	  CYGWIN*) NSISFILE=`cygpath -m $$NSISFILE`;; \
	esac; \
	cd $(top_win32dir); \
	eval "$$MAKENSIS -V2 -NOCD $$NSISFILE"
	mv $(top_win32dir)/$(package_win32)-setup.exe $(top_builddir)
	-sha1sum $(top_builddir)/$(package_win32)-setup.exe > $(top_builddir)/$(package_win32)-setup.exe.sha1 && \
	{ test -z "$(UNIX2DOS)" || $(UNIX2DOS) $(top_builddir)/$(package_win32)-setup.exe.sha1; }

dist-win32: dist-win32-zip dist-win32-7z dist-win32-exe

distclean-win32:
	if test -d "$(top_builddir)/$(package_win32)"; then \
	  rm -rf -- "$(top_builddir)/$(package_win32)"; \
	fi
	rm -f -- $(top_builddir)/$(package_win32).zip
	rm -f -- $(top_builddir)/$(package_win32).zip.sha1
	rm -f -- $(top_builddir)/$(package_win32).7z
	rm -f -- $(top_builddir)/$(package_win32).7z.sha1
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe
	rm -f -- $(top_builddir)/$(package_win32)-setup.exe.sha1


Summary: FreeType library
Name: freetype
Version: 1.1
Release: 0
Source: ftp://ftp.physiol.med.tu-muenchen.de/pub/freetype/freetype-1.1.tar.gz
URL: http://www.freetype.org/
Copyright: BSD-Like
Group: Libraries
BuildRoot: /var/tmp/freetype

%description
The FreeType engine is a free and portable TrueType font rendering engine.
It has been developed to provide TT support to a great variety of platforms
and environments.

Note that FreeType is a *library*. It is not a font server for your favorite
platform, even though it was designed to be used in many of them. Note also 
that it is *not* a complete text-rendering library. Its purpose is simply to
open and manage font files, as well as load, hint and render individual 
glyphs efficiently. You can also see it as a "TrueType driver" for a 
higher-level library, though rendering text with it is extremely easy, as 
demo-ed by the test programs.

This package contains the files needed to run programs that use the
FreeType engine.

%package devel
Summary: FreeType development headers and libraries
Group: Development/Libraries
Requires: freetype = 1.1

%description devel
The FreeType engine is a free and portable TrueType font rendering engine.
It has been developed to provide TT support to a great variety of platforms
and environments.

Note that FreeType is a *library*. It is not a font server for your favorite
platform, even though it was designed to be used in many of them. Note also 
that it is *not* a complete text-rendering library. Its purpose is simply to
open and manage font files, as well as load, hint and render individual 
glyphs efficiently. You can also see it as a "TrueType driver" for a 
higher-level library, though rendering text with it is extremely easy, as 
demo-ed by the test programs.

This package contains all supplementary files you need to develop your
own programs using the FreeType engine.

%package demo
Summary: FreeType test and demo programs
Group: Applications/Graphics
Requires: freetype = 1.1

%description demo
The FreeType engine is a free and portable TrueType font rendering engine.
It has been developed to provide TT support to a great variety of platforms
and environments.

Note that FreeType is a *library*. It is not a font server for your favorite
platform, even though it was designed to be used in many of them. Note also 
that it is *not* a complete text-rendering library. Its purpose is simply to
open and manage font files, as well as load, hint and render individual 
glyphs efficiently. You can also see it as a "TrueType driver" for a 
higher-level library, though rendering text with it is extremely easy, as 
demo-ed by the test programs.

This package contains several programs bundled with the FreeType engine for
testing and demonstration purposes.

%changelog
* Wed May 27 1998 Pavel Kankovsky <peak@kerberos.troja.mff.cuni.cz>
- changed group attr of freetype and freetype-devel package
- fixed misc glitches

* Sun May 24 1998 Pavel Kankovsky <peak@kerberos.troja.mff.cuni.cz>
- split the package into three parts (runtime library, development
  tools, and demo programs)
- added missing files (headers, NLS)
- added ldconfing upon (de)installation

* Thu Mar 12 1998 Bruno Lopes F. Cabral <bruno@openline.com.br>
- NLS for portuguese language is missing, sorry (may be in a near future)
  (please note the workaround using --with-locale-dir and gnulocaledir.
  NLS Makefile needs a bit more rework but again I'll not patch it here)

%prep 
%setup

find . -name CVS -type d | xargs rm -rf

%build
./configure --prefix=/usr --with-locale-dir=/usr/share/locale --enable-static
make all

%install
make install prefix=$RPM_BUILD_ROOT/usr gnulocaledir=$RPM_BUILD_ROOT/usr/share/locale

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%doc announce license.txt
/usr/lib/libttf.so.2.0.0
/usr/lib/libttf.so.2
/usr/share/locale/de/LC_MESSAGES/freetype.mo
/usr/share/locale/fr/LC_MESSAGES/freetype.mo
/usr/share/locale/nl/LC_MESSAGES/freetype.mo
/usr/share/locale/cs/LC_MESSAGES/freetype.mo

%files devel
%doc HOWTO.txt README announce license.txt readme.1st
%doc docs/
/usr/lib/libttf.so
/usr/lib/libttf.la
/usr/lib/libttf.a
/usr/include/freetype.h
/usr/include/ftxkern.h
/usr/include/ftxgasp.h
/usr/include/ftxcmap.h
/usr/include/ftxpost.h
/usr/include/ftxwidth.h
/usr/include/ftxerr18.h

%files demo
/usr/bin/ftview
/usr/bin/fttimer
/usr/bin/ftlint
/usr/bin/ftdump
/usr/bin/ftzoom
/usr/bin/ftstring
/usr/bin/ftstrpnm
/usr/bin/fterror


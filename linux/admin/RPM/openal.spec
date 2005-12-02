#
# spec file for package openal (Version 0.0.8)
#
# Copyright (c) 2005 SUSE LINUX Products GmbH, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://www.suse.de/feedback/
#

# norootforbuild
# neededforbuild SDL SDL-devel aalib aalib-devel alsa alsa-devel arts arts-devel audiofile esound esound-devel glib2 glib2-devel libogg libogg-devel libstdc++-devel libvorbis libvorbis-devel pkgconfig resmgr slang smpeg smpeg-devel xorg-x11-libs

BuildRequires: aaa_base acl attr bash bind-utils bison bzip2 coreutils cpio cpp cracklib cvs cyrus-sasl db devs diffutils e2fsprogs file filesystem fillup findutils flex gawk gdbm-devel glibc glibc-devel glibc-locale gpm grep groff gzip info insserv klogd less libacl libattr libgcc libnscd libselinux libstdc++ libxcrypt libzio m4 make man mktemp module-init-tools ncurses ncurses-devel net-tools netcfg openldap2-client openssl pam pam-modules patch permissions popt procinfo procps psmisc pwdutils rcs readline sed strace syslogd sysvinit tar tcpd texinfo timezone unzip util-linux vim zlib zlib-devel autoconf automake binutils gcc gdbm gettext libtool perl rpm SDL SDL-devel aalib aalib-devel alsa alsa-devel arts arts-devel audiofile esound esound-devel glib2 glib2-devel libogg libogg-devel libstdc++-devel libvorbis libvorbis-devel pkgconfig resmgr slang smpeg smpeg-devel xorg-x11-libs dialog expat fontconfig fontconfig-devel freeglut freeglut-devel freetype2 freetype2-devel gcc-c++ gnome-filesystem jack jack-devel libjpeg liblcms liblcms-devel libmng libmng-devel libpng libpng-devel libsndfile libtiff pciutils qt3 qt3-devel xorg-x11-Mesa xorg-x11-Mesa-devel xorg-x11-devel aaa_skel ash bind-libs gpg libgcj logrotate openslp suse-build-key suse-release tcsh

Name:         openal
License:      LGPL
Group:        System/Libraries
Autoreqprov:  on
Version:      0.0.8
Release:      1
URL:          http://www.openal.org/
Icon:         openal.xpm
Summary:      Open Audio Library
Source:       openal-%{version}.tar.gz
BuildRoot:    %{_tmppath}/%{name}-%{version}-build

%description
OpenAL is an audio library designed in the spirit of OpenGL--machine
independent, cross platform, and data format neutral with a clean,
simple C-based API.



Authors:
--------
    Adam D. Moss <adam@steambird.com>
    Bernd Kreimeier <bk@oddworld.com>
    Christopher Purnell <cjp@lost.org.uk>
    Dirk Ehlke <dehlke@mip.informatik.uni-kiel.de>
    Elias Naur <elias@oddlabs.com>
    Erik Greenwald <erik@smluc.org>
    Erik Hofman <erik@ehofman.com>
    Garin Hiebert <garinh@cheesetoast.net>
    Guillaume Borios <gborios@free.fr>
    Jason Daly <jdaly@ist.ucf.edu>
    John E. Stone <j.stone@acm.org>
    Joseph I. Valenzuela <valenzuela@treyarch.com>
    Michael Vance <michael@linuxgames.com>
    Prakash Punnoor <prakash@punnoor.de>
    Ryan C. Gordon <ryan@epicgames.com>
    Sven Panne <sven.panne@aedion.de>
    Yotam Gingold <ygingold@cs.brown.edu>
    Zoltan Ponekker <pontscho@kac.poliod.hu>

%package devel
Summary:      Static libraries, header files and tests for openal library
Requires:     openal = %{version}
Group:        Development/Libraries/C and C++

%description devel
OpenAL is an audio library designed in the spirit of OpenGL - machine
independent, cross platform, and data format neutral, with a clean,
simple C-based API.



Authors:
--------
    Adam D. Moss <adam@steambird.com>
    Bernd Kreimeier <bk@oddworld.com>
    Christopher Purnell <cjp@lost.org.uk>
    Dirk Ehlke <dehlke@mip.informatik.uni-kiel.de>
    Elias Naur <elias@oddlabs.com>
    Erik Greenwald <erik@smluc.org>
    Erik Hofman <erik@ehofman.com>
    Garin Hiebert <garinh@cheesetoast.net>
    Guillaume Borios <gborios@free.fr>
    Jason Daly <jdaly@ist.ucf.edu>
    John E. Stone <j.stone@acm.org>
    Joseph I. Valenzuela <valenzuela@treyarch.com>
    Michael Vance <michael@linuxgames.com>
    Prakash Punnoor <prakash@punnoor.de>
    Ryan C. Gordon <ryan@epicgames.com>
    Sven Panne <sven.panne@aedion.de>
    Yotam Gingold <ygingold@cs.brown.edu>
    Zoltan Ponekker <pontscho@kac.poliod.hu>

%debug_package
%prep
%setup -q

%build
%{?suse_update_config:%{suse_update_config -f admin/autotools}}
./autogen.sh
export CFLAGS="$RPM_OPT_FLAGS"
%ifarch ia64
export LDFLAGS=-Wl,-relax
%endif
./configure --prefix=%{_prefix}		\
	    --enable-optimization	\
	    --enable-arts		\
	    --enable-esd		\
	    --enable-null		\
	    --enable-sdl		\
	    --enable-smpeg		\
	    --enable-vorbis		\
	    --enable-waveout		\
	    --enable-capture
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT \
     DESTLIB=$RPM_BUILD_ROOT%{_libdir} \
     install
#
# documentation
install -m 755 -d $RPM_BUILD_ROOT%{_defaultdocdir}/%{name}
install -m 644 ChangeLog				\
	       AUTHORS 					\
	       README					\
	       common/specification/OpenAL1-1Spec.pdf	\
	       $RPM_BUILD_ROOT%{_defaultdocdir}/%{name}/
#
# configuration
install -m 755 -d $RPM_BUILD_ROOT/etc
cat > $RPM_BUILD_ROOT/etc/openalrc <<EOFMARKER
(define devices '(alsa native))

;; uncomment this to output via the 2nd soundcard
;;(define alsa-device "plughw:2,0")
EOFMARKER

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc %{_defaultdocdir}/%{name}/ChangeLog
%doc %{_defaultdocdir}/%{name}/AUTHORS
%doc %{_defaultdocdir}/%{name}/README
/etc/openalrc
%{_libdir}/libopenal.so.*

%files devel
%defattr(-,root,root)
%doc %{_defaultdocdir}/%{name}/OpenAL1-1Spec.pdf
%{_prefix}/bin/openal-config
%{_includedir}/AL/al.h
%{_includedir}/AL/alc.h
%{_includedir}/AL/alext.h
%{_libdir}/libopenal.a
%{_libdir}/libopenal.la
%{_libdir}/libopenal.so
%{_libdir}/pkgconfig/openal.pc

%changelog -n openal
* Tue Nov 29 2005 - sven.panne@aedion.de
- Synched with new directory structure
* Mon Nov 28 2005 - sven.panne@aedion.de
- Fixed build dependencies
- Updated file list
* Thu Oct 27 2005 - sven.panne@aedion.de
- Synched with latest Makefile changes
* Fri Sep 30 2005 - sven.panne@aedion.de
- Updated authors
* Fri Sep 23 2005 - sven.panne@aedion.de
- Various changes to bring the spec file in line with the OpenAL repository again.
* Tue Mar 15 2005 - sbrabec@suse.cz
- Removed incorrect ALSA flags (#72855).
* Thu Feb 03 2005 - sbrabec@suse.cz
- Updated to 2005-01-06 CVS snapshot (version 0.0.8).
* Fri Jan 21 2005 - ro@suse.de
- disable arch-asm (does not compile ATM)
* Fri Jan 21 2005 - sbrabec@suse.cz
- Updated to 2005-01-06 CVS snapshot.
* Thu Sep 02 2004 - sbrabec@suse.cz
- Updated to actual CVS snapshot.
* Wed Apr 28 2004 - ro@suse.de
- added -fno-strict-aliasing
* Sat Jan 10 2004 - adrian@suse.de
- add %%defattr
* Tue Dec 09 2003 - ro@suse.de
- build with alsa compatibility defines
* Mon Oct 06 2003 - ro@suse.de
- added glib2, glib2-devel to neededforbuild (arts)
* Mon Aug 11 2003 - sbrabec@suse.cz
- Updated to actual CVS snapshot.
* Thu Jul 31 2003 - ro@suse.de
- include sys/time.h before alsa
* Thu Apr 24 2003 - ro@suse.de
- fix install_info --delete call and move from preun to postun
* Mon Feb 10 2003 - sbrabec@suse.cz
- Use %%install_info (bug #23445).
* Mon Feb 10 2003 - sbrabec@suse.cz
- Updated to 20030131 CVS snapshot.
* Wed Oct 30 2002 - sbrabec@suse.cz
- Updated to actual CVS version.
- Fixed compiler warnings.
* Fri Aug 30 2002 - pmladek@suse.cz
- fixed dependency of the devel subpackage on the main package (used %%version)
* Fri Jul 05 2002 - kukuk@suse.de
- Use %%ix86 macro
* Mon Apr 08 2002 - pmladek@suse.cz
- arch specific assembler is only for i386
- fixed includes for ia64
* Wed Apr 03 2002 - pmladek@suse.cz
- used some pieces from old patches
- fixed to compile with autoconf-2.53:
- fixed acinclude.m4, used aclocal
- some fixes in configure.in
- cleaned up spec file
- removed boom.mp3 to avoid potential license problems
* Wed Feb 20 2002 - ro@suse.de
- removed kdelibs3-artsd from neededforbuild
  (artsd is there anyway)
* Sat Feb 16 2002 - sndirsch@suse.de
- enabled optimization and arch specific assembler
* Fri Feb 15 2002 - sndirsch@suse.de
- added SMPEG/SDL support to get MP3 playback support (required
  by VegaStrike)
* Fri Feb 15 2002 - ro@suse.de
- changed neededforbuild <kdelibs3-artsd> to <arts arts-devel>
* Fri Feb 08 2002 - bk@suse.de
- (re)enable optimisations and move prepare stuff to %%prep
* Thu Feb 07 2002 - sndirsch@suse.de
- added Ogg/Vorbis and Capture support
- removed compiler flags "-Werror -pedantic-errors"
* Thu Feb 07 2002 - sndirsch@suse.de
- added support for aRTs and esound daemon
- added global config file + a small patch to read this one
* Wed Feb 06 2002 - tiwai@suse.de
- added ALSA 0.9.0 support.  see README.alsa.
- clean up spec file, using %%_libdir.
- removed SDL.
* Fri Feb 01 2002 - sndirsch@suse.de
- updated to CVS sources of 20020201 (required for vegastrike)
- disabled patches (not required any more)
* Fri Jan 11 2002 - pmladek@suse.cz
- devel package created
- used macro %%{_librdir} to fix for lib64
* Wed Aug 08 2001 - ro@suse.de
- changed neededforbuild <sdl> to <SDL>
- changed neededforbuild <sdl-devel> to <SDL-devel>
* Tue May 22 2001 - pmladek@suse.cz
- fixed include files on ia64
- fixed preprocessor warnigs by patch for alpha
* Tue May 08 2001 - mfabian@suse.de
- bzip2 sources
* Thu Apr 19 2001 - pmladek@suse.cz
- fixed to compile on axp
* Wed Apr 04 2001 - schwab@suse.de
- Pass -relax to linker on ia64.
- Fix makefile to use LDFLAGS.
- Remove -fPIC when building non-library object.
* Mon Mar 26 2001 - ro@suse.de
- changed neededforbuild <sdl> to <sdl sdl-devel>
* Thu Nov 30 2000 - ro@suse.de
- added suse-update-config
* Mon Nov 06 2000 - ro@suse.de
- fixed neededforbuild
* Thu Jun 08 2000 - cihlar@suse.cz
- uncommented %%clean
* Tue May 09 2000 - smid@suse.cz
- buildroot added
- upgrade to version from 08.05.2000
* Tue Apr 11 2000 - sndirsch@suse.de
- removed '-Werror' and '-pedantic-erros' compiler flags
* Mon Mar 27 2000 - uli@suse.de
- renamed dif for easier maintenance
- __linux -> __linux__
- now uses RPM_OPT_FLAGS
  Wed Mar 15 19:31:35 CET 2000
- added test demos
* Thu Mar 09 2000 - sndirsch@suse.de
- created package

# This spec file is intended to be used to create a source RPM -- as there are too many configure variations possible to make a regular RPM worthwhile (so far).
#
# Instructions for building source RPM:
# -- Create a copy of the docs, include, and linux folders and place in a folder named "openal"
# -- Into the "openal" directory also copy the COPYING and README files
# -- Create an archive named openal.tar.gz with in it the folder openal and all of its contents.
# -- Place this archive into your /usr/src/redhat/SOURCES (or similar) folder
# -- Change to the directory that contains this spec file and run	"rpm -bs openal.spec" (or rpmbuild -bs openal.# spec for RedHat 8.0 users)
# -- This will give you a src RPM in your /usr/src/redhat/SRPMS (or similar) directory
#
#
#
#
Name: OpenAL
Summary: A portable audio API.
Version: 0.0.7
Release: CVS110904
Copyright: LGPL
Group: System/Libraries
Source: openal.tar.gz
URL: http://openal.org
Packager: Daniel Aleksandrow <dandandaman@users.sourceforge.net>
BuildRoot: /tmp/openal
Prefix: /usr/local
Provides: openal

%description
OpenAL: A portable audio API.

Please visit http://www.openal.org/ for API documentation.  For questions and comments about the API or any implementation
please use the mailing list.

Contact information is available at http://www.openal.org/lists.html

%prep
rm -rf $RPM_BUILD_ROOT

%setup -n openal

%build
echo "Configuring"
cd linux
./autogen.sh
./configure --prefix=/usr/local --enable-optimization --enable-alsa --enable-arts --enable-esd --enable-waveout --enable-null --enable-sdl --enable-vorbis --enable-smpeg
echo "Making"
make

%install
echo "Installing"
cd linux
# Installing Library Files
mkdir -p $RPM_BUILD_ROOT/usr/local/lib/
cp src/libopenal.so.0.0.7 $RPM_BUILD_ROOT/usr/local/lib/
ln -s -f /usr/local/lib/libopenal.so.0.0.7 $RPM_BUILD_ROOT/usr/local/lib/libopenal.so.0
ln -s -f /usr/local/lib/libopenal.so.0.0.7 $RPM_BUILD_ROOT/usr/local/lib/libopenal.so
cp src/libopenal.a $RPM_BUILD_ROOT/usr/local/lib/
# Installing Development Files
mkdir -p $RPM_BUILD_ROOT/usr/local/include/AL
cp include/AL/al*.h $RPM_BUILD_ROOT/usr/local/include/AL/
cp ../include/AL/al*.h $RPM_BUILD_ROOT/usr/local/include/AL/
# Installing Documentation Files
mkdir -p $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
rm -f doc/CVS/*
rmdir doc/CVS
cp doc/* $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
rm -f ../docs/CVS/*
rmdir ../docs/CVS
cp ../docs/* $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
cp ../COPYING $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
cp ../README $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
cp ChangeLog $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
cp CREDITS $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
cp PLATFORM $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
cp NOTES $RPM_BUILD_ROOT/usr/local/doc/OpenAL/
rm $RPM_BUILD_ROOT/usr/local/doc/OpenAL/Makefile*

%clean
rm -rf $RPM_BUILD_ROOT

%files
# Documentation files
/usr/local/doc/OpenAL/
# Library files
/usr/local/lib/libopenal.so.0.0.7
/usr/local/lib/libopenal.so.0
/usr/local/lib/libopenal.so
/usr/local/lib/libopenal.a
# Devel files
/usr/local/include/AL/

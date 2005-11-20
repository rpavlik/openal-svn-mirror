The CMake build files are maintained by prakash@punnoor.de, so
if you have troubles building OpenAL using CMake, please contact
me and not the OpenAL devs, as the official build system is still
the one using autoconf/make. The CMake build files are currenty
meant as an additional possibility to build OpenAL.

The aim of using CMake is making portable development easier, as
CMake containg generators for various build systems. On eg. Unix
Makefiles will be built, and on Windows MS VC++ project files, if
you wish. You can get CMake at cmake.org.

Current status:
(+,o,- indicates feature more/same/less then default OpenAL
build system.)

o building shared OpenAL lib, using (next) to identical parameters
  as the autoconf build system does
- no debug flags yet
- no static lib is built yet
- testcase isn't built yet
+ out of tree builts are supported
+ correct dependency tracking of source files
+ prepared for adding support for non GNU compiler
  (setting libraries to link to probably needs cleaning up)

This document explains briefly how to build CMake on Linux via out
of tree build:

- Change to the linux dir.
- Create a dir, eg "default", and change into it.
- Now (eg.) run:

cmake .. -DCMAKE_INSTALL_PREFIX:STRING="/usr" -DCMAKE_C_FLAGS:STRING="-march=athlon-xp -O2"
make
make install

- OpenAL should get installed as you got used to it.


I really would like to get CMake building OpenAL on every
supported platform. So please contact me if it doesn't build
on yours. I'll try to fix this with your help.


Some Tips:

- You can use a console GUI named ccmake for configuring cmake.
  It also takes some parameters as cmake as such does, so eg:

ccmake .. -DCMAKE_INSTALL_PREFIX:STRING="/usr" -DCMAKE_C_FLAGS:STRING="-march=athlon-xp -O2"

  sets the two variables defined on command line and then starts
  the GUI. Btw, to set boolean vars, use -DVAR:BOOL=X, where X
  is eg. ON or OFF.

- If you want more output at compile time, use

make VERBOSE=1

- If you want to install to a differnt directory (using same prefix):, use

make install DESTDIR=/foo/bar

- CMake doesn't has a distclean target by default, so you better
  really do an out of tree build, then you can simple delete its
  content when you want a distclean... Furthermore it is easier
  to have differnt build using different parameters via out of
  tree builds.

- If you are interested in variables to set, take a look into
  CMakeCache.txt.


Cheers,

Prakash

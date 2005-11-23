The openal/mac branch of code is no longer the preferred OS X codebase -- it's been replaced
by openal/macosx.  This codebase is mainly useful for OS 8 and 9, but you can still compile
it under OS X if you wish.

Compiling using CodeWarrior:

[NOTE: The CodeWarrior project has not been maintained for a while, so it may require some updating
to make it work properly.]

Run the osx_prebuild script, and then compile and link using the CodeWarrior 7 project.  After
running the script, a build directory will have been created which contains a complete OpenAL
framework, minus the library which will be compiled in CodeWarrior.  The contents of the build
directory can be placed in /Library/Frameworks/OpenAL.framework  for systemwide use or
in ~/Library/Frameworks/OpenAL.framework for single-user use.

Compiling using Project Builder:

Compile and link using the Project Builder project in the al_osx directory.  It will create an OpenAL
framework in the al_osx/build directory which can be placed in /Library/Frameworks for systemwide
use or in ~/Library/Frameworks for single-user use.

If you want to compile the Ogg Vorbis extension into the framework, you will need to have
already installed the Ogg and Vorbis frameworks on your system.  At the time
of this writing, they both have to be compiled from the nightly source code
available at http://www.xiph.org/ogg/vorbis/download/.  Compile the Ogg
framework first using the Project Builder project file.  To compile the
Vorbis framework, you'll first need to run the autogen.sh script, which will
give you an error because it can't find the Ogg library.  Ignore the error
and continue by using the Project Builder project file -- it will be able to
build the framework.


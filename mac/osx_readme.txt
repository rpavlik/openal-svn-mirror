For the moment, there are two ways to build OpenAL for OS X.  The CodeWarrior project will
not be maintained any longer, so will eventually go out of date.

Compiling using CodeWarrior:

Run the osx_prebuild script, and then compile and link using the CodeWarrior 7 project.  After
running the script, a build directory will have been created which contains a complete OpenAL
framework, minus the library which will be compiled in CodeWarrior.  The contents of the build
directory can be placed in /Library/Frameworks/OpenAL.framework  for systemwide use or
in ~/Library/Frameworks/OpenAL.framework for single-user use.

Compiling using Project Builder:

Compile and link using the Project Builder project in the al_osx directory.  It will create an OpenAL
framework in the al_osx/build directory which can be placed in /Library/Frameworks for systemwide
use or in ~/Library/Frameworks for single-user use.  Note -- at the time of this writing, Project
Builder will not create the Header link or copy the OpenAL headers into Versions/Current/Headers --
these steps should be done manually before distributing the framework.

If you want to compile the Ogg Vorbis extension into the framework, you will need to have
already installed the Ogg and Vorbis frameworks on your system.  At the time
of this writing, they both have to be compiled from the nightly source code
available at http://www.xiph.org/ogg/vorbis/download/.  Compile the Ogg
framework first using the Project Builder project file.  To compile the
Vorbis framework, you'll first need to run the autogen.sh script, which will
give you an error because it can't find the Ogg library.  Ignore the error
and continue by using the Project Builder project file -- it will be able to
build the framework.


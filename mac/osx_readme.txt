For the moment, there are two ways to build OpenAL for OS X.  The CodeWarrior project will probably
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

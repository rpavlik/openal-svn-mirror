For the moment, there are two ways to build OpenAL for OS X.  The CodeWarrior project will probably
not be maintained any longer, so will eventually go out of date.

1) Run the osx_prebuild script, and then compile and link using the CodeWarrior 7 project.  After
running the script, a build directory will have been created which contains a complete OpenAL
framework, minus the library which will be compiled in CodeWarrior.  The contents of the build
directory should be placed in /System/Library/Frameworks when installed.

2) Compile and link using the Project Builder project in the al_osx directory.  It will create an
OpenAL framework which can be placed with the other system libraries or used privately.




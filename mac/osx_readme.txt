To do a build for OS X, run the osx_prebuild script before compiling and linking using the
CodeWarrior 7 project.  After running the script, a build directory will have been created
which contains a complete OpenAL framework, minus the library which will be compiled in
CodeWarrior.  The contents of the build directory should be placed in /System/Library/Frameworks
when installed.


CC = gcc
CXX = g++

INCLUDE = -I/usr/local/include -I/usr/X11R6/include 
LIBDIR  = -L/usr/local/lib -L/usr/X11R6/lib -L. 

WARNFLAGS=-Wall -ansi -Wno-unused -Wno-long-long -Wno-deprecated
COMPILERFLAGS = $(WARNFLAGS) -fPIC -DPIC -g -O2 -DLINUX

CFLAGS = $(COMPILERFLAGS) $(INCLUDE)
CXXFLAGS = $(COMPILERFLAGS) $(INCLUDE)

LIBRARIES = -lopenal -lglut -lGL -lGLU -lXmu -lXext -lXi -lX11 -lpthread -ldl
COMMONOBJS = ../common/audioenv.o ../common/common.o ../common/object.o ../common/textout.o ../common/Obstruct.o

.SUFFIXES: .cpp .c .o
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

all : xldemo

xldemo : $(COMMONOBJS) xlmain.o 
	$(CXX) $(CXXFLAGS) -o xldemo $(LIBDIR) xlmain.o ../common/audioenv.o ../common/common.o ../common/object.o ../common/textout.o ../common/Obstruct.o $(LIBRARIES)  

clean:
	-rm -f $(COMMONOBJS) xldemo xlmain.o

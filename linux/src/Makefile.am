# We want to build a libopeanl library with libtool.
lib_LTLIBRARIES = libopenal.la

# libopenal consists of the followin sources and internal headers:
libopenal_la_SOURCES =		\
	al_bpool.o		\
	al_buffer.o		\
	al_config.o		\
	al_debug.o		\
	al_distance.o		\
	al_error.o		\
	al_ext.o		\
	al_filter.o		\
	al_listen.o		\
	al_main.o		\
	al_mixer.o		\
	al_mixfunc.o		\
	al_mixmanager.o		\
	al_mspool.o		\
	al_mutexlib.o		\
	al_queue.o		\
	al_rctree.o		\
	al_rcvar.o		\
	al_source.o		\
	al_spool.o		\
	al_state.o		\
	al_threadlib.o		\
	al_vector.o		\
	mixaudio16.o

# We use sin and friends, so we might need -lm, autoconf takes care of this.
libopenal_la_LIBADD = $(LIBM)

# The following libtool flag is neccessary for building DLLs on Windows.
libopenal_la_LDFLAGS = -no-undefined

# Specifying the following path is needed to find <AL/alc.h> and <AL/al.h>.
libopenal_la_CPPFLAGS = -I$(top_srcdir)/../include

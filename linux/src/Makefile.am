# We want to build a libopeanl library with libtool.
lib_LTLIBRARIES = libopenal.la

# libopenal consists of the following sources and internal headers:
libopenal_la_SOURCES =			\
	al_bpool.c			\
	al_buffer.c			\
	al_config.c			\
	al_debug.c			\
	al_distance.c			\
	al_error.c			\
	al_ext.c			\
	al_filter.c			\
	al_listen.c			\
	al_main.c			\
	al_mixer.c			\
	al_mixfunc.c			\
	al_mixmanager.c			\
	al_mspool.c			\
	al_mutexlib.c			\
	al_queue.c			\
	al_rctree.c			\
	al_rcvar.c			\
	al_source.c			\
	al_spool.c			\
	al_state.c			\
	al_threadlib.c			\
	al_vector.c			\
	mixaudio16.c

# backend sources
libopenal_la_SOURCES +=			\
	arch/interface/interface_sound.c \
	arch/alsa/alsa.c		\
	arch/arts/arts.c		\
	arch/esd/esd.c			\
	arch/null/null.c		\
	arch/sdl/sdl.c			\
	arch/waveout/waveout.c		\
	arch/portable/cpu_caps.c

# OS-specific sources
libopenal_la_SOURCES +=			\
	arch/linux/lin_dsp.c

# ALC sources
libopenal_la_SOURCES +=			\
	alc/alc_context.c		\
	alc/alc_device.c		\
	alc/alc_error.c			\
	alc/alc_speaker.c

# audiconvert sources
libopenal_la_SOURCES +=			\
	audioconvert/ac_misc.c		\
	audioconvert/ac_freq.c		\
	audioconvert/ac_bits.c		\
	audioconvert/ac_channels.c	\
	audioconvert/ac_helper.c	\
	audioconvert/ac_adpcm.c		\
	audioconvert/ac_endian.c

# extension sources
libopenal_la_SOURCES +=			\
	extensions/al_ext_capture.c	\
	extensions/al_ext_loki.c	\
	extensions/al_ext_mp3.c		\
	extensions/al_ext_vorbis.c

# The first path is needed to find <AL/alc.h> and <AL/al.h> and the second one
# is needed for <AL/alext.h>
libopenal_la_CPPFLAGS =			\
	-I$(top_srcdir)/../include	\
	-I$(top_srcdir)/include

# Add compiler flags.
libopenal_la_CFLAGS = $(WARNINGCFLAGS) $(OPTIMIZATIONCFLAGS) $(FEATURECFLAGS)

# The -no-undefined flag is neccessary for building DLLs on Windows.
libopenal_la_LDFLAGS = -no-undefined $(PROFILINGLDFLAGS)

# We use sin and friends, so we might need -lm, autoconf takes care of this.
libopenal_la_LIBADD = $(LIBM)

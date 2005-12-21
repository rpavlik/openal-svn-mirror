# We want to build a libopeanl library with libtool.
lib_LTLIBRARIES = libopenal.la

# libopenal consists of the following sources and internal headers:
libopenal_la_SOURCES =			\
	al_bpool.c			\
	al_bpool.h			\
	al_buffer.c			\
	al_buffer.h			\
	al_config.c			\
	al_config.h			\
	al_cpu_caps.h			\
	al_debug.c			\
	al_debug.h			\
	al_distance.c			\
	al_distance.h			\
	al_error.c			\
	al_error.h			\
	al_ext.c			\
	al_ext.h			\
	al_filter.c			\
	al_filter.h			\
	al_listen.c			\
	al_listen.h			\
	al_main.c			\
	al_main.h			\
	al_mixer.c			\
	al_mixer.h			\
	al_mixfunc.c			\
	al_mixfunc.h			\
	al_mixmanager.c			\
	al_mixmanager.h			\
	al_mspool.c			\
	al_mspool.h			\
	al_mutexlib.c			\
	al_mutexlib.h			\
	al_queue.c			\
	al_queue.h			\
	al_rctree.c			\
	al_rctree.h			\
	al_rcvar.c			\
	al_rcvar.h			\
	al_siteconfig.h			\
	al_source.c			\
	al_source.h			\
	al_spool.c			\
	al_spool.h			\
	al_state.c			\
	al_threadlib.c			\
	al_threadlib.h			\
	al_types.h			\
	al_vector.c			\
	al_vector.h			\
	al_matrix.c			\
	al_matrix.h			\
	mixaudio16.h			\
	mixaudio16.c

# backend sources

if USEBACKENDALSA
libopenal_la_SOURCES +=			\
	arch/alsa/alsa.c		\
	arch/alsa/alsa.h
endif

if USEBACKENDARTS
libopenal_la_SOURCES +=			\
	arch/arts/arts.c		\
	arch/arts/arts.h
endif

if USEBACKENDESD
libopenal_la_SOURCES +=			\
	arch/esd/esd.c			\
	arch/esd/esd.h
endif

if USEBACKENDNATIVEIRIX
libopenal_la_SOURCES +=			\
	arch/irix/iris.c		\
	arch/irix/iris.h
endif

if USEBACKENDNATIVELINUX
libopenal_la_SOURCES +=			\
	arch/linux/lin_dsp.c		\
	arch/linux/lin_dsp.h
endif

if USEBACKENDNULL
libopenal_la_SOURCES +=			\
	arch/null/null.c		\
	arch/null/null.h
endif

if USEBACKENDSDL
libopenal_la_SOURCES +=			\
	arch/sdl/sdl.c			\
	arch/sdl/sdl.h
endif

if USEBACKENDNATIVESOLARIS
libopenal_la_SOURCES +=			\
	arch/solaris/solaris_native.c	\
	arch/solaris/solaris_native.h
endif

if USEBACKENDWAVEOUT
libopenal_la_SOURCES +=			\
	arch/waveout/waveout.c		\
	arch/waveout/waveout.h
endif

if USEBACKENDNATIVEWINDOWS
libopenal_la_SOURCES +=			\
	arch/windows/windows_native.c	\
	arch/windows/windows_native.h
endif

libopenal_la_SOURCES +=			\
	arch/interface/interface_sound.c \
	arch/interface/interface_sound.h \
	arch/interface/platform.h	\
	arch/portable/cpu_caps.c

# processor-specific sources
libopenal_la_SOURCES +=			\
	arch/portable/floatmul.c	\
	arch/i386/x86_cpu_caps_prk.h

# ALC sources
libopenal_la_SOURCES +=			\
	alc/alc_context.c		\
	alc/alc_context.h		\
	alc/alc_device.c		\
	alc/alc_device.h		\
	alc/alc_error.c			\
	alc/alc_error.h			\
	alc/alc_speaker.c		\
	alc/alc_speaker.h

# audiconvert sources
libopenal_la_SOURCES +=			\
	audioconvert/ac_adpcm.c		\
	audioconvert/ac_adpcm.h		\
	audioconvert/ac_bits.c		\
	audioconvert/ac_channels.c	\
	audioconvert/ac_endian.c	\
	audioconvert/ac_endian.h	\
	audioconvert/ac_freq.c		\
	audioconvert/ac_helper.c	\
	audioconvert/ac_misc.c		\
	audioconvert/ac_wave.h		\
	audioconvert/audioconvert.h

# extension sources
libopenal_la_SOURCES +=			\
	extensions/al_ext_capture.c	\
	extensions/al_ext_capture.h	\
	extensions/al_ext_loki.c	\
	extensions/al_ext_loki.h	\
	extensions/al_ext_mp3.c		\
	extensions/al_ext_mp3.h		\
	extensions/al_ext_needed.h	\
	extensions/al_ext_vorbis.c	\
	extensions/al_ext_vorbis.h

# The first path is needed to find <AL/alc.h> and <AL/al.h> and the second one
# is needed for <AL/alext.h>
libopenal_la_CPPFLAGS =			\
	-I$(top_srcdir)/common/include	\
	-I$(top_srcdir)/include

# Add compiler flags.
libopenal_la_CFLAGS = $(WARNINGCFLAGS) $(OPTIMIZATIONCFLAGS) $(FEATURECFLAGS)

# The -no-undefined flag is neccessary for building DLLs on Windows.
libopenal_la_LDFLAGS = -no-undefined $(PROFILINGLDFLAGS)

# We use sin and friends, so we might need -lm, autoconf takes care of this.
libopenal_la_LIBADD = $(LIBM)

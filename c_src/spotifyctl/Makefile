ifeq ($(shell uname),Darwin)
ifdef USE_AUDIOQUEUE
AUDIO_DRIVER ?= osx
LDFLAGS += -framework AudioToolbox
else
AUDIO_DRIVER ?= openal
LDFLAGS += -framework OpenAL
endif
else
CFLAGS  = -fPIC $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags alsa)
LDFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs-only-L alsa)
LDLIBS  = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs-only-l --libs-only-other alsa)
AUDIO_DRIVER ?= alsa
endif

TARGET = spotifyctl
## TARGET = playtrack
OBJS = $(TARGET).o spotifyctl_util.o appkey.o $(AUDIO_DRIVER)-audio.o audio.o

$(TARGET): $(OBJS)
#	$(CC) $(CFLAGS) $(LDFLAGS) $^ -shared -Wl,-soname,libspotifyctl.so.1 -o lib$@.so $(LDLIBS)
#	ar rcs libspotifyctl.a $(OBJS)
	echo $(OBJS)

ifdef DEBUG
ifeq ($(shell uname),Darwin)
	install_name_tool -change @loader_path/../Frameworks/libspotify.framework/libspotify @rpath/libspotify.so $@
endif
endif

all:	check-libspotify $(TARGET)

#
# Direct path to libspotify
#
ifdef LIBSPOTIFY_PATH

P=$(shell cd "$(LIBSPOTIFY_PATH)" && pwd)

check-libspotify:
	@test -f $(P)/lib/libspotify.so || (echo "Failed to find libspotify.so in $(P)/lib" >&2 ; exit 1)
	@test -f $(P)/include/libspotify/api.h || (echo "Failed to find libspotify/api.h in $(P)/include" >&2 ; exit 1)

CFLAGS  += -I$(P)/include
LDFLAGS += -Wl,-rpath,$(P)/lib -L$(P)/lib
LDLIBS  += -lspotify

ifeq ($(shell uname),Darwin)
CPUARCH   ?= $(shell uname -m)
CFLAGS    += -D__APPLE__ -arch $(CPUARCH)
LDFLAGS   += -arch $(CPUARCH)
endif

#
# Use pkg-config(1)
#
else

check-libspotify:
	@pkg-config --exists libspotify || (echo "Failed to find libspotify using pkg-config(1)" >&2 ; exit 1)

CFLAGS  += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags libspotify)
LDFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs-only-L libspotify)
LDLIBS  += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs-only-l --libs-only-other libspotify)

endif

ifdef DEBUG
CFLAGS += -g3 -O0
endif

CFLAGS += -Wall

.PHONY: all check-libspotify clean

vpath %.c ../

clean:
	rm -f *.o *.so *~ $(TARGET)


audio.o: audio.c audio.h
alsa-audio.o: alsa-audio.c audio.h
dummy-audio.o: dummy-audio.c audio.h
osx-audio.o: osx-audio.c audio.h
openal-audio.o: openal-audio.c audio.h
spotify.o: spotify.c audio.h
playtrack.o: playtrack.c audio.h
spotifyctl_util.o: spotifyctl_util.c spotifyctl.h

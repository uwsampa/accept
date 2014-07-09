TARGET := ccv

lib/config.mk: lib/config.mk.in
	cd lib ; ./configure

SOURCES := $(wildcard lib/*.c) $(wildcard lib/3rdparty/dsfmt/*.c) \
	$(wildcard lib/3rdparty/kissfft/*.c) $(wildcard lib/3rdparty/sfmt/*.c) \
	$(wildcard lib/3rdparty/sha1/*.c) bin/bbfdetect.c

CFLAGS := -Ilib -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION
LDFLAGS := -lm

RUNARGS := samples/cmyk-jpeg-format.jpg samples/face

APP_MK := ../app.mk
include $(APP_MK)

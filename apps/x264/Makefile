TARGET := x264
SOURCES := $(wildcard *.c) $(wildcard common/*.c) $(encoder/*.c) \
	       $(wildcard extras/*.c)
HEADERS := $(wildcard *.h) $(wildcard common/*.h) $(wildcard common/x86/*.h) \
	       $(wildcard encoder/*.h) $(wildcard extras/*.h)

CFLAGS := -I.  # Prioritize local x264.h over system.
RUNARGS := XXX

APP_MK := ../app.mk
include $(APP_MK)

TARGET := x264
SOURCES := $(wildcard *.c) $(wildcard common/*.c) $(wildcard encoder/*.c) \
	       $(wildcard extras/*.c)
HEADERS := $(wildcard *.h) $(wildcard common/*.h) $(wildcard common/x86/*.h) \
	       $(wildcard encoder/*.h) $(wildcard extras/*.h)

# -I. : Prioritize local x264.h over system.
# -Wno-array-bounds : Work around a Clang bug.
#  http://llvm.org/bugs/show_bug.cgi?id=12818
# ... and some configuration defines.
CFLAGS := -I. -Wno-array-bounds -DHAVE_STDINT_H -DHAVE_PTHREAD
RUNARGS := XXX

APP_MK := ../app.mk
include $(APP_MK)

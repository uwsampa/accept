TARGET := x264
SOURCES := common/mc.c common/predict.c common/pixel.c common/macroblock.c \
	       common/frame.c common/dct.c common/cpu.c common/cabac.c \
	       common/common.c common/mdate.c common/set.c \
	       common/quant.c common/vlc.c \
	       encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
	       encoder/set.c encoder/macroblock.c encoder/cabac.c \
	       encoder/cavlc.c encoder/encoder.c \
		   x264.c matroska.c muxers.c
HEADERS := $(wildcard *.h) $(wildcard common/*.h) $(wildcard common/x86/*.h) \
	       $(wildcard encoder/*.h)

# -I. : Prioritize local x264.h over system.
# -Wno-array-bounds : Work around a Clang bug.
#  http://llvm.org/bugs/show_bug.cgi?id=12818
# -Wno-incompatible-pointer-types : enerc deficiency
# ... and some configuration defines.
CFLAGS := -I. -Wno-array-bounds -Wno-incompatible-pointer-types -DHAVE_STDINT_H -DHAVE_PTHREAD
LDFLAGS := -lm -lpthread

# simmedium
#RUNARGS := --quiet --qp 20 --partitions b8x8,i4x4 --ref 5 --direct auto --b-pyramid --weightb --mixed-refs --no-fast-pskip --me umh --subme 7 --analyse b8x8,i4x4 --threads 4 -o eledream.264 eledream_640x360_32.y4m
# simlarge
RUNARGS := --quiet --qp 20 --partitions b8x8,i4x4 --ref 5 --direct auto --b-pyramid --weightb --mixed-refs --no-fast-pskip --me umh --subme 7 --analyse b8x8,i4x4 --threads 4 -o eledream.264 eledream_640x360_128.y4m

APP_MK := ../app.mk
include $(APP_MK)

ARCH    := msp430
SOURCES := ar.c

CFLAGS  := -DUSE_LEDS -O0
LDFLAGS := -O0 -Wl,--section-start -Wl,FRAMVARS=0xD000
LIBS     = -lm

# msp430 library dependencies
MSP430LIBS = adxl362z wisp5

APP_MK  := ../app.mk
include $(APP_MK)

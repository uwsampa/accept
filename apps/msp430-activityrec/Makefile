ARCH    := msp430
SOURCES := ar.c

CFLAGS  := -DUSE_LEDS -O0
LDFLAGS := -O0 -Wl,--section-start -Wl,FRAMVARS=0xD000
LIBS     = -lm

# msp430 library dependencies
MSP430LIBS = adxl362z wisp5

EXTRADEPS += intr.o
LDFLAGS += intr.o
CLEANMETOO += intr.o

APP_MK  := ../app.mk
include $(APP_MK)

# Hack to get the interrupt handler for perfctr correctly built
intr.o: intr.c
	msp430-gcc -mmcu=msp430fr5969 -I$(ACCEPTDIR)/rt/msp430/perfctr -O0 -c $< -o $@

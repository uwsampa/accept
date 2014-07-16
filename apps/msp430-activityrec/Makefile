ARCH    := msp430
SOURCES := ar.c
CFLAGS  := -Wall -fno-stack-protector -D__MSP430FR5969__ -DUSE_LEDS -O2
LDFLAGS := -mmcu=msp430fr5969 -O2

ARLIBS    := $(wildcard lib/*) ../../rt/msp430/perfctr
SUBDIRS   := $(ARLIBS)
CFLAGS    += $(addprefix -I,$(ARLIBS))
LIBS      += -lm $(foreach L,$(ARLIBS),-L$(L) -l$(notdir $(L)))
EXTRADEPS := $(foreach L,$(ARLIBS),$(L)/lib$(notdir $(L)).a)

APP_MK  := ../app.mk
include $(APP_MK)

lib/adxl362z/libadxl362z.a:
	make -C lib/adxl362z

lib/wisp5/libwisp5.a:
	make -C lib/wisp5

../../rt/msp430/perfctr/libperfctr.a:
	make -C ../../rt/msp430/perfctr

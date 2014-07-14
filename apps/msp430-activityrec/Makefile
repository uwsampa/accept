ARCH := msp430
SOURCES := ar.c

CFLAGS  = -Wall -O0 -fno-stack-protector -g
CFLAGS += -D__MSP430FR5969__
CFLAGS += -DUSE_LEDS
LDFLAGS += -mmcu=msp430fr5969

ARLIBS  = $(notdir $(wildcard lib/*))
CFLAGS += $(addprefix -Ilib/, $(ARLIBS))
LIBS   += -lm $(foreach L,$(ARLIBS),-Llib/$(L) -l$(L))

APP_MK := ../app.mk
include $(APP_MK)

lib/adxl362z/libadxl362z.a:
	make -C lib/adxl362z

lib/wisp5/libwisp5.a:
	make -C lib/wisp5

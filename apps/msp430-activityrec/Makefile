CFLAGS  = -Wall -O0 -fno-stack-protector -g
CFLAGS += -target msp430-elf -D__MSP430FR5969__ $(MSPGCCINCLUDES)
CFLAGS += -DUSE_LEDS

LIBS    = wisp5 adxl362z
LDFLAGS = -lm $(foreach L,$(LIBS),-Llib/$(L) -l$(L))
CFLAGS += $(foreach L,$(LIBS),-Ilib/$(L))

HEADERS = $(wildcard *.h)

all: ar.elf

lib/adxl362z/libadxl362z.a:
	make -C lib/adxl362z

lib/wisp5/libwisp5.a:
	make -C lib/wisp5

MSPGCC := msp430-gcc
AR := msp430-ar
RANLIB := msp430-ranlib
MSPGCCINCLUDES := $(foreach I, \
	$(shell msp430-cpp -Wp,-v </dev/null 2>&1 | grep /include | \
		sed -e 's/^ *//'), \
	-I$(I))

CC := clang

.PRECIOUS: %.bc %.s

ar.elf: $(foreach L,$(LIBS),lib/$(L)/lib$(L).a)

%.bc: %.c $(HEADERS)
	$(CC) $(CFLAGS) -emit-llvm -c -o $@ $<

%.s: %.bc
	llc -O0 -regalloc=fast -march=msp430 -msp430-hwmult-mode=no -o $@ $<

%.elf: %.s
	$(MSPGCC) -O0 -mmcu=msp430fr5969 -o $@ $< $(LDFLAGS)

%.ll: %.bc
	llvm-dis $^

.PHONY: clean
clean:
	$(RM) *.elf *.bc *.ll *.s *.dot *.pdf *.a *.o
	for SUBDIR in $(wildcard lib/*); do \
		make -C "$$SUBDIR" clean ; \
	done

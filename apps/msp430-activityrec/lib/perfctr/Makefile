CC=msp430-gcc
AR=msp430-ar
OPT=-O0
DBG=-g
CFLAGS=-mmcu=msp430fr5969 $(OPT) $(DBG)

all: libperfctr.a

%.o: $(ACCEPTDIR)/rt/msp430/perfctr/%.c
	$(CC) $(CFLAGS) -c $< -o $@

libperfctr.a: perfctr.o
	$(AR) r $@ $^

clean:
	$(RM) *.elf *.o *.a

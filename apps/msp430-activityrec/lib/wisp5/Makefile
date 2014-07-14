CC=msp430-gcc
AR=msp430-ar
OPT=-O0
DBG=-g
CFLAGS=-mmcu=msp430fr5969 $(OPT) $(DBG)

all: libwisp5.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libwisp5.a: accel.o spi.o
	$(AR) r $@ $^

clean:
	$(RM) *.elf *.o *.a

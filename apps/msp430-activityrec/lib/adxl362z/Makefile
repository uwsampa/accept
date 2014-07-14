CC=msp430-gcc
AR=msp430-ar
OPT=-O0
DBG=-g
CFLAGS=-mmcu=msp430fr5969 $(OPT) $(DBG)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libadxl362z.a: adxl362z.o
	$(AR) r $@ $^

clean:
	$(RM) *.elf *.o *.a

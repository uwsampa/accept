TARGET := msp430-strawman
ARCH := msp430
CLANGARGS := -O0 -Werror
LINKER := msp430-gcc
LDFLAGS := -mmcu=msp430f2618

# there is literally no good way to do this without autoconf
ifndef MSPGCC_ROOT
  ifeq ($(shell which port >/dev/null && echo 1 || echo 0),1)
    MSPGCC_ROOT=/opt/local
  else
    $(error MSPGCC_ROOT is undefined, and could not detect MacPorts)
  endif
endif

CFLAGS := -target msp430 -I$(MSPGCC_ROOT)/lib/gcc/msp430/4.6.3/include -I$(MSPGCC_ROOT)/lib/gcc/msp430/4.6.3/include-fixed -I$(MSPGCC_ROOT)/msp430/include -D__MSP430F2618__ $(CLANGARGS)

include ../app.mk

PROFLIB=enercrt-msp430.bc
ENERCRT_SRC=$(ENERCDIR)/rt/enercrt.c
$(PROFLIB): $(ENERCRT_SRC)
	$(CC) $(CFLAGS) -emit-llvm -c -o $@ $<

# replace with an MSP430 simulator
RUNSHIM=$(MSPGCC_ROOT)/bin/msp430-objdump -d

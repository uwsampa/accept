###
# Generic Makefile for enerc benchmark apps.
###
# To provide arguments to the program when profiling, set RUNARGS.
# To provide extra arguments to clang, set CLANGARGS.
# To run the executable through something else (e.g., a simulator), set RUNSHIM
###
# Don't use this file directly!  Include it from a Makefile in an app's
# subdirectory or else the paths to various tools will be wrong.
###

# Paths to components of the ACCEPT toolchain.
ACCEPTDIR := $(realpath $(lastword $(MAKEFILE_LIST))/..)
INCLUDEDIR := $(ACCEPTDIR)/include
BUILTDIR := $(ACCEPTDIR)/build/built
CC := $(BUILTDIR)/bin/clang
CXX := $(BUILTDIR)/bin/clang++
LLVMDIS := $(BUILTDIR)/bin/llvm-dis
LLVMLINK := $(BUILTDIR)/bin/llvm-link
LLVMOPT := $(BUILTDIR)/bin/opt
LLVMLLC := $(BUILTDIR)/bin/llc
LLVMLLI := $(BUILTDIR)/bin/lli
RTDIR := $(ACCEPTDIR)/rt

# Target platform specifics.
ARCH ?= default
RTLIB ?= $(RTDIR)/acceptrt.$(ARCH).bc
EXTRABC += $(RTLIB)

# Host platform specifics.
ifeq ($(shell uname -s),Darwin)
	ifeq ($(shell uname -r | sed -e 's/\..*//'),13) # OS X 10.9
		XCODEINCLUDES = $(shell xcrun --show-sdk-path)/usr/include
		ifeq ($(ARCH),default)
			CFLAGS += -I$(XCODEINCLUDES)
			CXXFLAGS += -I$(XCODEINCLUDES)
		endif
	endif
	LIBEXT := dylib
else
	ifneq ($(wildcard /usr/include/x86_64-linux-gnu/c++/4.8),)
		# Work around Clang include path search bug on Debian/Ubuntu.
		# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=697127
		CXXFLAGS += -I/usr/include/x86_64-linux-gnu/c++/4.8
	endif
	LIBEXT := so
endif
ENERCLIB ?= $(BUILTDIR)/lib/EnerCTypeChecker.$(LIBEXT)
PASSLIB ?= $(BUILTDIR)/lib/enerc.$(LIBEXT)

# General compiler flags.
override CFLAGS += -I$(INCLUDEDIR) -g -fno-use-cxa-atexit
override CXXFLAGS += $(CFLAGS)
LLCARGS += -O2

# Compiler flags to pass to Clang to add the ACCEPT machinery.
ENERCFLAGS :=  -Xclang -load -Xclang $(ENERCLIB) \
	-Xclang -add-plugin -Xclang enerc-type-checker

# SOURCES is a list of source files, *.{c,cpp} by default.
SOURCES ?= $(wildcard *.c) $(wildcard *.cpp)

# Build products.
TARGET ?= app
BCFILES := $(SOURCES:.c=.bc)
BCFILES := $(BCFILES:.cpp=.bc)
LINKEDBC := $(TARGET)_all.bc
LLFILES := $(BCFILES:.bc=.ll)

# Attempt to guess which linker to use.
ifneq ($(filter %.cpp,$(SOURCES)),)
	LINKER ?= $(CXX)
else
	LINKER ?= $(CC)
endif

# Determine which command-line arguments to use depending on whether we are
# "training" (profiling/measuring) or "testing" (performing a final
# evaluation).
ifneq ($(ACCEPT_TEST),)
	ifneq ($(TESTARGS),)
		RUNARGS = $(TESTARGS)
	endif
endif

#################################################################
# The different executable configurations we can build.
CONFIGS := orig opt dummy

BUILD_TARGETS := $(CONFIGS:%=build_%)
RUN_TARGETS := $(CONFIGS:%=run_%)
.PHONY: all setup clean profile $(BUILD_TARGETS) $(RUN_TARGETS)

all: build_orig

# See below for BUILD_TARGETS rule

$(RUN_TARGETS): run_%: $(TARGET).%
	$(RUNSHIM) ./$< $(RUNARGS)

# Blank setup target (for overriding).
setup:
#################################################################

# Platform-specific settings for the Zynq.
ifeq ($(ARCH),zynq)
ZYNQDIR := $(ACCEPTDIR)/plat/zynqlib
override CFLAGS += -target arm-none-linux-gnueabi \
	-ccc-gcc-name arm-linux-gnueabi-gcc \
	-D_GNU_SOURCE=1 \
	-I$(ZYNQDIR) -I$(ZYNQDIR)/bsp/include
ARMTOOLCHAIN ?= /sampa/share/Xilinx/14.6/14.6/ISE_DS/EDK/gnu/arm/lin
LINKER := $(ARMTOOLCHAIN)/bin/arm-xilinx-eabi-gcc
LDFLAGS := -Wl,-T -Wl,$(ZYNQDIR)/lscript.ld -L$(ZYNQDIR)/bsp/lib
LIBS := -Wl,--start-group,-lxil,-lgcc,-lc,-lm,--end-group
LLCARGS += -march=arm -mcpu=cortex-a9
RUNSHIM := $(ACCEPTDIR)/plat/zynqrun.sh $(ZYNQBIT)
CLEANMETOO += output.txt zynqlog.txt
OPTARGS += -accept-npu
endif

# And for msp430.
ifeq ($(ARCH),msp430)
LINKER     := msp430-gcc
LLCARGS    += -march=msp430 -msp430-hwmult-mode=no
LDFLAGS    += -mmcu=msp430fr5969
RUNSHIM    := $(ACCEPTDIR)/plat/msp430/run.sh
MSP430LIBS += perfctr
MSP430DEPS := $(foreach L,$(MSP430LIBS),$(ACCEPTDIR)/rt/msp430/$(L)/lib$(L).a)
EXTRADEPS  += $(MSP430DEPS)
override \
CFLAGS     += -target msp430-elf -Wall -fno-stack-protector -D__MSP430FR5969__ \
		$(addprefix -I, \
			$(shell msp430-cpp -Wp,-v </dev/null 2>&1 | \
			grep /include | sed -e 's/^ *//')) \
		$(addprefix -I$(ACCEPTDIR)/rt/msp430/,$(MSP430LIBS))
LIBS       += $(foreach L,$(MSP430LIBS),-L$(ACCEPTDIR)/rt/msp430/$(L) -l$(L))

$(MSP430DEPS):
	make -C $(dir $@)
endif

# BUILD_TARGETS rule is here to catch any EXTRADEPS assigned above
$(BUILD_TARGETS): build_%: setup $(EXTRADEPS) $(TARGET).%

# Make LLVM bitcode from C/C++ sources.
%.bc: %.c $(HEADERS)
	$(CC) $(ENERCFLAGS) $(CFLAGS) $(CLANGARGS) -c -emit-llvm -o $@ $<
%.bc: %.cpp $(HEADERS)
	$(CXX) $(ENERCFLAGS) $(CXXFLAGS) $(CLANGARGS) -c -emit-llvm -o $@ $<
%.ll: %.bc
	$(LLVMDIS) $<

# Make the ACCEPT runtime library for the target architecture.
$(RTLIB):
	make -C $(RTDIR) acceptrt.$(ARCH).bc CC="$(CC)" CFLAGS="$(CFLAGS)"

# Link component bitcode files into a single file.
$(LINKEDBC): $(BCFILES) $(EXTRABC)
	$(LLVMLINK) $^ > $@

# For debugging: we can also disassemble to .ll files.
#	for f in $(BCFILES); do \
#		$(LLVMDIS) $$f; \
#	done

# Versions of the amalgamated program.
$(TARGET).orig.bc: $(LINKEDBC)
	$(LLVMOPT) -load $(PASSLIB) -O1 $(OPTARGS) $< -o $@
$(TARGET).opt.bc: $(LINKEDBC) accept_config.txt
	$(LLVMOPT) -load $(PASSLIB) -O1 -accept-relax $(OPTARGS) $< -o $@
$(TARGET).dummy.bc: $(LINKEDBC)
	cp $< $@

# .bc -> .s
$(TARGET).%.s: $(TARGET).%.bc
	$(LLVMLLC) $(LLCARGS) $< > $@

# .s -> executable (assemble and link)
$(TARGET).%: $(TARGET).%.s
	$(LINKER) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	$(RM) $(TARGET) $(TARGET).s $(BCFILES) $(LLFILES) $(LINKEDBC) \
	accept-globals-info.txt accept_config.txt accept_config_desc.txt \
	accept_log.txt accept_time.txt \
	$(CONFIGS:%=$(TARGET).%.bc) $(CONFIGS:%=$(TARGET).%) \
	accept-approxRetValueFunctions-info.txt accept-npuArrayArgs-info.txt \
	$(CLEANMETOO)
	for SUBDIR in $(SUBDIRS); do make -C "$$SUBDIR" clean; done

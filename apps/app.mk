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

ENERCDIR := ../..
INCLUDEDIR := $(ENERCDIR)/include
BUILTDIR := $(ENERCDIR)/build/built
CC := $(BUILTDIR)/bin/clang
CXX := $(BUILTDIR)/bin/clang++
LLVMDIS := $(BUILTDIR)/bin/llvm-dis
LLVMLINK := $(BUILTDIR)/bin/llvm-link
LLVMOPT := $(BUILTDIR)/bin/opt
LLVMLLC := $(BUILTDIR)/bin/llc
LLVMLLI := $(BUILTDIR)/bin/lli
LLVMPROF := $(BUILTDIR)/bin/llvm-prof
SUMMARY := $(ENERCDIR)/bin/summary.py

ifeq ($(shell uname -s),Darwin)
	ifeq ($(shell uname -r | sed -e 's/\..*//'),13) # OS X 10.9
		XCODEINCLUDES = $(shell xcrun --show-sdk-path)/usr/include
		CFLAGS += -I$(XCODEINCLUDES)
		CXXFLAGS += -I$(XCODEINCLUDES)
	endif
	LIBEXT := dylib
else
#CFLAGS += -L/home/andreolb/accept-git/accept/apps/npu/lib -I/usr/arm-linux-gnueabi/include/c++/4.7.3/arm-linux-gnueabi/ -I/usr/arm-linux-gnueabi/include/ -I/usr/arm-linux-gnueabi/include/c++/4.7.3
#CFLAGS += -I/usr/arm-linux-gnueabi/include/c++/4.7.3/arm-linux-gnueabi/ -I/usr/arm-linux-gnueabi/include/ -I/usr/arm-linux-gnueabi/include/c++/4.7.3
#CXXFLAGS += -I/usr/arm-linux-gnueabi/include/c++/4.7.3/arm-linux-gnueabi/ -I/usr/arm-linux-gnueabi/include/ -I/usr/arm-linux-gnueabi/include/c++/4.7.3
	CFLAGS += -I../lib -I../lib/bsp/include
	CXXFLAGS += -I../lib -I../lib/bsp/include
	LIBEXT := so
endif
ENERCLIB ?= $(BUILTDIR)/lib/EnerCTypeChecker.$(LIBEXT)
PASSLIB ?= $(BUILTDIR)/lib/enerc.$(LIBEXT)
PROFLIB ?= $(BUILTDIR)/../enerc/rt/enercrt.bc
LIBPROFILERT := $(BUILTDIR)/lib/libprofile_rt.$(LIBEXT)

#override CFLAGS += -Xclang -load -Xclang $(ENERCLIB)
override CFLAGS += -target arm-none-linux-gnueabi -ccc-gcc-name arm-linux-gnueabi-gcc --static -Xclang -load -Xclang $(ENERCLIB) \
	-Xclang -add-plugin -Xclang enerc-type-checker \
	-g -fno-use-cxa-atexit \
	-I$(INCLUDEDIR) -I/home/andreolb/accept-git/accept/apps/npu/lib -I/home/andreolb/accept-git/accept/apps/npu/include -emit-llvm
override CXXFLAGS += $(CFLAGS)

# SOURCES is a list of source files, *.{c,cpp} by default
SOURCES ?= $(wildcard *.c) $(wildcard *.cpp)

BCFILES := $(SOURCES:.c=.bc)
BCFILES := $(BCFILES:.cpp=.bc)
LINKEDBC := $(TARGET)_all.bc
LLFILES := $(BCFILES:.bc=.ll)

# attempt to guess which linker to use
ifneq ($(filter %.cpp,$(SOURCES)),)
	LINKER ?= $(CXX)
else
	LINKER ?= $(CC)
endif

# The different executable configurations we can build.
CONFIGS := orig opt prof

#################################################################
BUILD_TARGETS := $(CONFIGS:%=build_%)
RUN_TARGETS := $(CONFIGS:%=run_%)
.PHONY: all clean profile $(BUILD_TARGETS) $(RUN_TARGETS)

all: build_orig

$(BUILD_TARGETS): build_%: $(TARGET).%

$(RUN_TARGETS): run_%: $(TARGET).%
	$(RUNSHIM) ./$< $(RUNARGS)

profile: $(TARGET).prof.bc llvmprof.out
	$(LLVMPROF) $^
#################################################################

# make LLVM bitcode from C/C++ sources
%.bc: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(CLANGARGS) -c -o $@ $<
%.bc: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLANGARGS) -c -o $@ $<
%.ll: %.bc
	$(LLVMDIS) $<

# Link component bitcode files into a single file.
ifeq ($(ARCH),msp430)
$(LINKEDBC): $(BCFILES)
else
$(LINKEDBC): $(BCFILES) $(PROFLIB)
endif
	$(LLVMLINK) $^ > $@
	for f in $(BCFILES); do \
		$(LLVMDIS) $$f; \
	done

# Three different transformations of the amalgamated program.
$(TARGET).prof.bc: $(LINKEDBC)
	$(LLVMOPT) -insert-edge-profiling $< -o $@
$(TARGET).orig.bc: $(LINKEDBC)
	$(LLVMOPT) -print-after-all -load $(PASSLIB) -O1  $< -o $@
$(TARGET).opt.bc: $(LINKEDBC) accept_config.txt
	$(LLVMOPT) -print-after-all -load $(PASSLIB) -accept-relax -O1  $< -o $@

# .bc -> .o
ifeq ($(ARCH),msp430)
# llc cannot generate object code for msp430, so emit assembly
#.INTERMEDIATE: $(TARGET).%.s
$(TARGET).%.s: $(TARGET).%.bc
	$(LLVMOPT) -strip $< | \
	$(LLVMLLC) -march=msp430 > $@
$(TARGET).%.o: $(TARGET).%.s
	msp430-gcc $(MSPGCC_CFLAGS) -c $<
else
$(TARGET).%.s: $(TARGET).%.bc
	$(LLVMOPT) -strip $< | \
	$(LLVMLLC) -march=arm -float-abi=soft > inversek2j.s
endif

#/usr/bin/arm-linux-gnueabihf-gcc-4.7 -Wl,-T -Wl,/home/andreolb/accept-git/accept/apps/npu/lscript.ld,/home/andreolb/accept-git/accept/apps/npu/lib/libxil.a -march=armv7-a -static -mfloat-abi=hard -o $@ $< -lm
#/usr/bin/arm-linux-gnueabihf-gcc-4.7 -march=armv7-a -static -mfloat-abi=hard -o $@ $< -lm -Wl,/home/andreolb/accept-git/accept/apps/npu/lib/libxil.a 
#/usr/bin/arm-linux-gnueabihf-gcc-4.7 -Wl,-T -Wl,/home/andreolb/accept-git/accept/apps/npu/lscript.ld -march=armv7-a -static -mfloat-abi=hard -o $@ $< -lm
#/usr/bin/arm-linux-gnueabihf-gcc-4.7 -march=armv7-a -static -mfloat-abi=hard -o $@ $< -lm
# .o -> executable
#source /opt/Xilinx/14.6/ISE_DS/settings64.sh
$(TARGET).%: $(TARGET).%.s
	echo "bla"
#/opt/Xilinx/14.6/ISE_DS/EDK/gnu/arm/lin/bin/arm-xilinx-eabi-gcc -Wl,-T -Wl,/opt/Xilinx/14.6/ISE_DS/EDK/sw/lib/sw_apps/fsbl/src/lscript.ld -o $@ $< -Wl,--start-group,-lxil,-lgcc,-lc,-lm,--end-group
#/usr/bin/arm-linux-gnueabihf-gcc-4.7 -Wl,-T -Wl,/home/andreolb/accept-git/accept/apps/npu/lscript.ld -Wl,--build-id=none -march=armv7-a -static -mfloat-abi=hard -o $@ $< /home/andreolb/accept-git/accept/apps/npu/lib/libxil.a -lm

# LLVM profiling pipeline.
llvmprof.out: $(TARGET).prof
	./$(TARGET).prof $(RUNARGS)
# Profiling executable requires linking with an additional (native) library.
.INTERMEDIATE: $(TARGET).prof.o
$(TARGET).prof: $(TARGET).prof.o
	$(LINKER) -lprofile_rt $(LDFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET) $(TARGET).o $(BCFILES) $(LLFILES) $(LINKEDBC) \
	accept-approxRetValueFunctions-info.txt accept-globals-info.txt accept_config.txt accept_config_desc.txt accept_log.txt accept_time.txt inversek2j.s accept-npuArrayArgs-info.txt inversek2j.elf output.txt \
	$(CONFIGS:%=$(TARGET).%.bc) $(CONFIGS:%=$(TARGET).%) \
	llvmprof.out \
	$(CLEANMETOO)

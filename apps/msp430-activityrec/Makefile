# Run "make" to build with mspgcc

CFLAGS=-g -O0 -fomit-frame-pointer 
LDFLAGS=-lm -fomit-frame-pointer

ACCELDIR=.
ACCELLIB=adxl362z

ifdef NOMSP430
  CC=gcc
  CFLAGS+=-I../../Runtime
else
  CC=msp430-gcc
  CFLAGS+=-mmcu=msp430fr5969 -I$(ACCELDIR)
  LDFLAGS+=-L$(ACCELDIR) -l$(ACCELLIB)
endif

all: AR.cue.elf 

.PRECIOUS: %.elf

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@ -c

%.a43: %.elf
	msp430-objcopy -O ihex $^ $@

AR_train.cue.elf: Accelerometer.o NormalDistributionModel.o vec3.o FeatureVector.o CUEAR_featurizer.o AR_train.o libadxl362z.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

AR.cue.elf: Accelerometer.o CUEAR_binnednaivebayes.o vec3.o FeatureVector.o CUEAR_featurizer.o AR.o libadxl362z.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

AR.lp.elf: lowpower_ar.o  libadxl362z.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

TEMP.cue.elf: Thermometer.o VarianceDetector.o FeatureVector.o THERM_featurizer.o AR.o adc_sensor.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

person.elf: Person.o NormalDistributionModel.o vec3.o FeatureVector.o PERSON_featurizer.o AR.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) -x c -c $^ -o $@

libadxl362z.a: adxl362z.o
	msp430-ar r $@ $^
	msp430-ranlib $@

clean:
	$(RM) *.o *.elf

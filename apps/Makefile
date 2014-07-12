SUBDIRS = blackscholes cpptest jpeg strawman streamcluster sobel canneal fluidanimate x264 zynq-sobel zynq-jmeint zynq-inversek2j zynq-fft ccv
CLEANDIRS = $(SUBDIRS:%=clean-%)

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: clean $(CLEANDIRS)
clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

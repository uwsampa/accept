SUBDIRS = blackscholes cpptest jpeg strawman streamcluster
CLEANDIRS = $(SUBDIRS:%=clean-%)

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: clean $(CLEANDIRS)
clean: $(CLEANDIRS)
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean

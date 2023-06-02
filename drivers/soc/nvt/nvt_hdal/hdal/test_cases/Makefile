include $(NVT_PRJCFG_MODEL_CFG)

SUBDIRS := $(dir $(wildcard */Makefile))
SUBDIRS := $(shell echo $(SUBDIRS) | sed -e 's/\/$$//g';)

all:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n || exit "$$?"; \
	done

install:
ifneq ($(NVT_SAMPLES_INSTALL),DISABLE)
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n install; \
	done
endif

clean:
	@for n in $(SUBDIRS); do \
		$(MAKE) -C $$n clean; \
	done
	@ rm -rf output

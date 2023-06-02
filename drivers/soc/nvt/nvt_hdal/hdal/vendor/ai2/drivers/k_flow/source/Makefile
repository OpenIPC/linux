SUBDIRS := $(dir $(wildcard */Makefile */makefile))

all:
	@for n in $(SUBDIRS); do \
		echo ">>>>>>>>>> processing folder $$n"; \
		$(MAKE) -C $$n || exit "$$?"; \
		echo "Done!"; \
		echo " "; \
	done

modules_install:
	@for n in $(SUBDIRS); do \
		echo ">>>>>>>>>> processing folder $$n install"; \
		$(MAKE) -C $$n modules_install; \
		echo "Done!"; \
		echo " "; \
	done

clean:
	@for n in $(SUBDIRS); do \
		echo ">>>>>>>>>> processing folder $$n clean"; \
		$(MAKE) -C $$n clean; \
		echo "Done!"; \
		echo " "; \
	done

codesize:
	@for n in $(SUBDIRS); do \
		echo ">>>>>>>>>> processing folder $$n codesize"; \
		$(MAKE) -C $$n codesize; \
		echo "Done!"; \
		echo " "; \
	done
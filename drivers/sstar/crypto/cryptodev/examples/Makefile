CC=${CROSS_COMPILE}gcc

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZER
LDFLAGS +=  -static -I. -I../

target := aes-sstar-unique aes sha sha-copy test_aes_cbc_split test_cbc_iv

check: $(target)
	mkdir -p output
	mv $(target) output
	make -C cipher
	mv cipher/install output
clean:
	rm -rf *.o *~ $(target) output/
	make clean -C cipher

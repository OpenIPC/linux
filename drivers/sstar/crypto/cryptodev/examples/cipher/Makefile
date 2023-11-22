CC=$(CROSS_COMPILE)gcc

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZER
CFLAGS += -I. \
	  -I../../../ \
	  -I../../crypto/

ciphers := cipher_aes_sync cipher_aes_async cipher_aes_speed_sync cipher_aes_speed_async \
			 cipher_hash256_sync cipher_hash256_async cipher_hash256_speed_sync cipher_hash256_speed_async \
			 cipher_rsa_sync cipher_rsa_speed_sync \
			 fullspeed sha_speed

all: $(ciphers)
	mkdir -p install
	mv $(ciphers) install

clean:
	rm -f *.o *~ $(ciphers)
	rm -fr install


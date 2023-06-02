CFLAGS += -I.. $(CRYPTODEV_CFLAGS) -Wall -Werror

CC := $(CROSS_COMPILE)gcc

comp_progs := cipher_comp hash_comp hmac_comp

hostprogs := cipher cipher-aead hmac speed async_cipher async_hmac \
	async_speed sha_speed hashcrypt_speed fullspeed cipher-gcm \
	cipher-aead-srtp $(comp_progs)

cipher-objs := cipher.o
cipher-aead-objs := cipher-aead.o
hmac-objs := hmac.o
speed-objs := speed.c
fullspeed-objs := fullspeed.c
sha-speed-objs := sha_speed.c
async-cipher-objs := async_cipher.o
async-hmac-objs := async_hmac.o
async-speed-objs := async_speed.o
hashcrypt-speed-objs := hashcrypt_speed.c

prefix ?= /usr/local
execprefix ?= $(prefix)
bindir = $(execprefix)/bin

all: $(hostprogs)

check: $(hostprogs)
	./cipher
	./hmac
	./async_cipher
	./async_hmac
	./cipher-aead-srtp
	./cipher-gcm
	./cipher-aead

install:
	install -d $(DESTDIR)/$(bindir)
	for prog in $(hostprogs); do \
		install -m 755 $$prog $(DESTDIR)/$(bindir); \
	done

clean:
	rm -f *.o *~ $(hostprogs)

${comp_progs}: LDLIBS += -lssl -lcrypto
${comp_progs}: %: %.o openssl_wrapper.o

.PHONY: all clean check install

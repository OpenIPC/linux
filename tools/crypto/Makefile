

CC = $(CROSS_COMPILE)gcc
#PTHREAD_LIBS = -lpthread
#WARNINGS = -Wall -Wextra
#CFLAGS = $(WARNINGS) -g -I../include
#LDFLAGS = $(PTHREAD_LIBS)

all: test_crypto
%: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) test_crypto


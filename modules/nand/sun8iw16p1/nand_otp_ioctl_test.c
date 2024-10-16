/*
 * nand_blk.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>



#define OTP_IOCTL_READ				IO('V', 23)
#define OTP_IOCTL_WRITE				_IO('V', 24)
#define OTP_IOCTL_READ_UID		_IO('V', 25)
#define OTP_IOCTL_PROTECT			_IO('V', 26)

struct otpblc_op_t {
	int block;
	int page;
	unsigned char *buf;
	unsigned int len;
};


int main(int argc, char **argv)
{

	int fd;
	int ret;
	unsigned char buf[2048] = {0};
	struct otpblc_op_t otp_arg;
	int i = 0;
	int write = 0;
	int protect_write = 0;
	int uid = 0;
	printf("ver0.3\n");

	if (argc != 5) {
		printf("arg error\n");
		return -1;
	}
	printf("%s\n", argv[1]);
	if (!strncmp(argv[1], "w", 1)) {
		write = 1;
		printf("normal write\n");
	} else if (!strncmp(argv[1], "pw", 2)) {
		protect_write = 1;
		write = 1;
		printf("protect write\n");
	} else if (!strncmp(argv[1], "r",  1)) {
		printf("read\n");
	} else if (!strncmp(argv[1], "u",  1)) {
		printf("uid\n");
		uid = 1;
	} else {
		printf("unknow arg\n");
		return -1;
	}

	printf("block%d\n", atoi(argv[2]));
	printf("page%d\n", atoi(argv[3]));
	printf("len%d\n", atoi(argv[4]));

	fd = open("/dev/nanda", O_RDWR);
		if (fd < 0) {
				perror("open");
				exit(-2);
	}

	if (!write) {
		printf("in read\n");
		otp_arg.block = atoi(argv[2]);
		otp_arg.page = atoi(argv[3]);
		otp_arg.len =  atoi(argv[4]);
		otp_arg.buf = buf;

		if (uid) {
			ret = ioctl(fd, OTP_IOCTL_READ_UID, &otp_arg);
		} else {
			ret = ioctl(fd, OTP_IOCTL_READ, &otp_arg);
		}
		if (ret) {
			perror("ioctl error:");
			exit(-4);
		}

		for (i = 0; i < 2048/4; i++) {
			if (!(i%4))
				printf("\n");
			printf("0x%8x ", *((unsigned int *) &buf[i*4]));
		}
	} else {
		printf("in write\n");
		memset(buf, 0x55, 2048);
		otp_arg.block = atoi(argv[2]);
		otp_arg.page = atoi(argv[3]);
		otp_arg.len =  atoi(argv[4]);
		otp_arg.buf = buf;

#if 0
	for (i = 0; i < 2048/4; i++) {
		if (!(i%4))
			printf("\n");
		printf("0x%x ", *((unsigned int *) &buf[i*4]));
	}
#endif
		if (protect_write == 1) {
			ret = ioctl(fd, OTP_IOCTL_PROTECT, &otp_arg);
			printf("protect write\n");
		} else {
			ret = ioctl(fd, OTP_IOCTL_WRITE, &otp_arg);
		}
		if (ret) {
			perror("ioctl error:");
			exit(-4);
		}
	}

		close(fd);
		return 0;
}

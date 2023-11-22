/*
 * test-queue.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xrp_kernel_defs.h"

static int fails;

static void test_queue_in(int fd)
{
    char                   buf[5];
    struct xrp_ioctl_queue q = {0};
    int                    rc;
    q.in_data_addr = 0x90000000;
    q.in_data_size = 4;
    rc             = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        perror("XFAIL in_data 1");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL in_data 1\n");
    }

    q.in_data_size = 4096;
    rc             = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        perror("XFAIL in_data 2");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL in_data 2\n");
    }

    q.in_data_addr = (__u64)(uintptr_t)(buf + 1);
    q.in_data_size = 4;
    rc             = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL in_data 3");
    }
    else
    {
        fprintf(stderr, "PASS in_data 3\n");
    }
}

static void test_queue_out(int fd)
{
    char                   buf[5];
    struct xrp_ioctl_queue q = {0};
    int                    rc;
    q.out_data_addr = 0x90000000;
    q.out_data_size = 4;
    rc              = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        perror("XFAIL out_data 1");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL out_data 1\n");
    }

    q.out_data_size = 4096;
    rc              = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        perror("XFAIL out_data 2");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL out_data 2\n");
    }

    q.out_data_addr = (__u64)(uintptr_t)(buf + 1);
    q.out_data_size = 4;
    rc              = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL out_data 3");
    }
    else
    {
        fprintf(stderr, "PASS out_data 3\n");
    }
}

static void test_queue_buf(int fd)
{
    char                    mem[16];
    struct xrp_ioctl_buffer buf = {0};
    struct xrp_ioctl_queue  q   = {0};
    int                     rc;
    q.buffer_addr = 0x90000000;
    q.buffer_size = sizeof(struct xrp_ioctl_buffer);
    rc            = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        perror("XFAIL buf 1");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL buf 1\n");
    }

    q.buffer_size = 4096;
    rc            = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        perror("XFAIL buf 2");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL buf 2\n");
    }

    q.buffer_addr = (__u64)(uintptr_t)&buf;
    q.buffer_size = sizeof(buf);
    rc            = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL buf 3");
    }
    else
    {
        fprintf(stderr, "PASS buf 3\n");
    }

    buf.flags = XRP_FLAG_READ_WRITE;
    buf.size  = 1;
    buf.addr  = 0x90000000;
    rc        = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        perror("XFAIL buf 4");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL buf 4\n");
    }

    buf.flags = XRP_FLAG_READ_WRITE;
    buf.size  = sizeof(mem);
    buf.addr  = (__u64)(uintptr_t)&mem;
    rc        = ioctl(fd, XRP_IOCTL_QUEUE, &q);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL buf 5");
    }
    else
    {
        fprintf(stderr, "PASS buf 5\n");
    }
}

int main()
{
    int fd = open("/dev/xvp0", O_RDWR);
    test_queue_in(fd);
    test_queue_out(fd);
    test_queue_buf(fd);
    return fails;
}

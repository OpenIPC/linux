/*
 * test-map.c- Sigmastar
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

static void test_unmap(int fd)
{
    struct xrp_ioctl_alloc alloc = {
        .size = 4096 * 4,
    };
    int   rc = ioctl(fd, XRP_IOCTL_ALLOC, &alloc);
    void* p  = (void*)(uintptr_t)alloc.addr;

    if (rc == -1)
    {
        ++fails;
        perror("FAIL ioctl(XRP_IOCTL_ALLOC)");
        return;
    }
    else
    {
        fprintf(stderr, "PASS ioctl(XRP_IOCTL_ALLOC), addr = %p\n", p);
    }

    rc = munmap(p + 4096 * 3, 4096);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL munmap 1");
    }
    else
    {
        fprintf(stderr, "PASS munmap 1\n");
    }

    rc = munmap(p + 4096, 4096);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL munmap 2");
    }
    else
    {
        fprintf(stderr, "PASS munmap 2\n");
    }

    rc = munmap(p, 4096 * 4);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL munmap 3");
    }
    else
    {
        fprintf(stderr, "PASS munmap 3\n");
    }
}

static void test_remap(int fd)
{
    struct xrp_ioctl_alloc alloc = {
        .size = 4096 * 4,
    };
    int   rc = ioctl(fd, XRP_IOCTL_ALLOC, &alloc);
    void* p  = (void*)(uintptr_t)alloc.addr;
    void* p1;

    if (rc == -1)
    {
        ++fails;
        perror("FAIL ioctl(XRP_IOCTL_ALLOC)");
        return;
    }
    else
    {
        fprintf(stderr, "PASS ioctl(XRP_IOCTL_ALLOC), addr = %p\n", p);
    }

    /* cannot make allocation longer */
    p1 = mremap(p, 4096 * 4, 4096 * 5, MREMAP_MAYMOVE);

    if (p1 == MAP_FAILED)
    {
        perror("XFAIL mremap 1");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL mremap 1\n");
    }

    /* but can make it shorter */
    p1 = mremap(p, 4096 * 4, 4096 * 3, MREMAP_MAYMOVE);

    if (p1 == MAP_FAILED)
    {
        ++fails;
        perror("FAIL mremap 2");
    }
    else
    {
        fprintf(stderr, "PASS mremap 2\n");
    }

    /* but then can't get freed part back */
    p1 = mremap(p, 4096 * 3, 4096 * 4, MREMAP_MAYMOVE);

    if (p1 == MAP_FAILED)
    {
        perror("XFAIL mremap 3");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL mremap 3\n");
    }

    /* the same starting in the middle of the mapping: can't make the tail longer*/
    p1 = mremap(p + 4096, 4096 * 2, 4096 * 3, MREMAP_MAYMOVE);

    if (p1 == MAP_FAILED)
    {
        perror("XFAIL mremap 4");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL mremap 4\n");
    }

    /* but can make it shorter */
    p1 = mremap(p + 4096, 4096 * 2, 4096, MREMAP_MAYMOVE);

    if (p1 == MAP_FAILED)
    {
        ++fails;
        perror("FAIL mremap 5");
    }
    else
    {
        fprintf(stderr, "PASS mremap 5\n");
    }

    /* but then can't get freed part back */
    p1 = mremap(p + 4096, 4096, 4096 * 2, MREMAP_MAYMOVE);

    if (p1 == MAP_FAILED)
    {
        perror("XFAIL mremap 6");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL mremap 6\n");
    }

    /* move allocated page inside the current allocation */
    p1 = mremap(p + 4096, 4096, 4096, MREMAP_MAYMOVE | MREMAP_FIXED, p);

    if (p1 == MAP_FAILED)
    {
        ++fails;
        perror("FAIL mremap 7");
    }
    else
    {
        fprintf(stderr, "PASS mremap 7\n");
    }

    /* move allocated page outside the original allocation */
    p1 = mremap(p, 4096, 4096, MREMAP_MAYMOVE | MREMAP_FIXED, p + 4096 * 4);

    if (p1 == MAP_FAILED)
    {
        ++fails;
        perror("FAIL mremap 8");
    }
    else
    {
        fprintf(stderr, "PASS mremap 8\n");
    }

    rc = munmap(p + 4096 * 4, 4096);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL munmap");
    }
    else
    {
        fprintf(stderr, "PASS munmap\n");
    }
}

static void test_remap_file_pages(int fd)
{
    struct xrp_ioctl_alloc alloc = {
        .size = 4096 * 4,
    };
    int   rc = ioctl(fd, XRP_IOCTL_ALLOC, &alloc);
    void* p  = (void*)(uintptr_t)alloc.addr;
    void* p1;

    if (rc == -1)
    {
        ++fails;
        perror("FAIL ioctl(XRP_IOCTL_ALLOC)");
        return;
    }
    else
    {
        fprintf(stderr, "PASS ioctl(XRP_IOCTL_ALLOC), addr = %p\n", p);
    }

    rc = remap_file_pages(p + 4096, 4096, PROT_READ | PROT_WRITE, 0, 0);

    if (rc == -1)
    {
        perror("XFAIL remap_file_pages");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL remap_file_pages\n");
    }

    rc = ioctl(fd, XRP_IOCTL_FREE, &alloc);

    if (rc == -1)
    {
        ++fails;
        perror("FAIL ioctl(XRP_IOCTL_FREE)");
    }
    else
    {
        fprintf(stderr, "PASS ioctl(XRP_IOCTL_FREE)\n");
    }
}

int main()
{
    int   fd = open("/dev/xvp0", O_RDWR);
    void* p  = mmap(NULL, 4096 * 3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (p == MAP_FAILED)
    {
        perror("XFAIL mmap");
    }
    else
    {
        ++fails;
        fprintf(stderr, "FAIL mmap succeeded\n");
    }

    test_unmap(fd);
    test_remap(fd);
    test_remap_file_pages(fd);
    return fails;
}

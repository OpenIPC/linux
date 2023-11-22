/*
 * cam_dev_wrapper.h- Sigmastar
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

#ifndef __CAM_DEV_WRAPPER_H__
#define __CAM_DEV_WRAPPER_H__

#define CAM_DEV_WRAPPER_VERSION "v1.0.2"

#ifdef CAM_OS_RTK /*start RTK part*/
#include "cam_drv_poll.h"

struct pollfd
{
    int   fd;
    short events;
    short revents;
};

#define _IOC(dir, type, nr, size) (((dir) << 0) | ((type) << 8) | ((nr) << 16) | ((size) << 24))

#define _IOC_TYPECHECK(t) (sizeof(t))

/* used to create numbers */
#define _IO(type, nr)         _IOC(0, (type), (nr), 0)
#define _IOR(type, nr, size)  _IOC(1, (type), (nr), (_IOC_TYPECHECK(size)))
#define _IOW(type, nr, size)  _IOC(2, (type), (nr), (_IOC_TYPECHECK(size)))
#define _IOWR(type, nr, size) _IOC(3, (type), (nr), (_IOC_TYPECHECK(size)))

#define FILL_VERCHK_TYPE(var, var_ver, var_size, version) \
    (                                                     \
        {                                                 \
            var_ver  = ((version & 0xffffffff));          \
            var_size = sizeof(var);                       \
            var;                                          \
        })

int   CamDevOpen(char *name);
int   CamDevClose(int fd);
int   CamDevIoctl(int fd, unsigned long request, void *param);
int   CamDevPoll(struct pollfd *fds, int nfds, int timeout);
void *CamDevMmap(int length, int fd, int offset);
int   CamDevMunmap(int fd, void *start, int length);
int   CamDevRead(int fd, void *buf, unsigned int count);
int   CamDevWrite(int fd, const void *buf, unsigned int count);

#else // For linux user space

#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define CamDevOpen(a)         open(a, O_RDWR)
#define CamDevClose(a)        close(a)
#define CamDevIoctl(a, b, c)  ioctl(a, b, c)
#define CamDevPoll(a, b, c)   poll(a, b, c)
#define CamDevMmap(a, b, c)   mmap(0, a, PROT_READ | PROT_WRITE, MAP_SHARED, b, c)
#define CamDevMunmap(a, b, c) munmap(b, c)
#define CamDevRead(a, b, c)   read(a, b, c)
#define CamDevWrite(a, b, c)  write(a, b, c)

#endif

#endif // __CAM_DEV_WRAPPER_H__

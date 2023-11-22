/*
 * key.c- Sigmastar
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
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <linux/input.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int                fd;
    struct input_event ev_key;
    if (argc != 2)
    {
        printf("Usage:\n");
        printf("%s /dev/input/event0 or /dev/input/event1\n", argv[0]);
        return 0;
    }
    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        perror("open device buttons");
        exit(1);
    }
    while (1)
    {
        read(fd, &ev_key, sizeof(struct input_event));
        printf("type:%d,code:%d,value:%d\n", ev_key.type, ev_key.code, ev_key.value);
    }

    close(fd);
    return 0;
}

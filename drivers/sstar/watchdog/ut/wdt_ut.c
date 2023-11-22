/*
 * wdt_ut.c- Sigmastar
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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/signal.h>

#define WATCHDOG_IOCTL_BASE 'W'

#define WDIOC_SETOPTIONS    _IOR(WATCHDOG_IOCTL_BASE, 4, int)
#define WDIOC_SETTIMEOUT    _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_KEEPALIVE     _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define WDIOC_GETTIMEOUT    _IOR(WATCHDOG_IOCTL_BASE, 7, int)
#define WDIOC_SETPRETIMEOUT _IOWR(WATCHDOG_IOCTL_BASE, 8, int)
#define WDIOS_DISABLECARD   0x0001 /* Turn off the watchdog timer */
#define WDIOS_ENABLECARD    0x0002 /* Turn on the watchdog timer */

int wdt_fd          = -1;
int keep_alive_flag = 1;

int stop_watchdog(void)
{
    if (wdt_fd == -1)
    {
        printf("stop_watchdog fail to open watchdog device\n");
        return -1;
    }
    int option = WDIOS_DISABLECARD;
    ioctl(wdt_fd, WDIOC_SETOPTIONS, &option);
    if (wdt_fd != -1)
    {
        close(wdt_fd);
        wdt_fd = -1;
    }
    return 0;
}

int open_watchdog(void)
{
    wdt_fd = open("/dev/watchdog", O_WRONLY);
    if (wdt_fd == -1)
    {
        printf("fail to open watchdog device %d\n", wdt_fd);
    }
    return wdt_fd;
}

int set_watchdog_timeout(int time)
{
    if (wdt_fd == -1)
    {
        printf("set_watchdog_timeout fail to open watchdog device\n");
        return -1;
    }
    ioctl(wdt_fd, WDIOC_SETTIMEOUT, &time);
    return 0;
}

int set_watchdog_pretimeout(int time)
{
    if (wdt_fd == -1)
    {
        printf("set_watchdog_pretimeout fail to open watchdog device\n");
        return -1;
    }
    ioctl(wdt_fd, WDIOC_SETPRETIMEOUT, &time);
    return 0;
}

int set_watchdog_keep_alive(void)
{
    if (wdt_fd == -1)
    {
        printf("WatchdogKeepAlive fail to open watchdog device\n");
        return -1;
    }
    ioctl(wdt_fd, WDIOC_KEEPALIVE, 0);
    return 0;
}

void psdk_watch_dog_get_timeout(int* time)
{
    ioctl(wdt_fd, WDIOC_GETTIMEOUT, &time);
}

void exit_watchdog(int sign)
{
    printf("exit watchdog\n");
    stop_watchdog();
    exit(0);
}

int main(int argc, char* argv[])
{
    int       time         = 5;
    long long itime        = 0;
    char      ctrltype[32] = {0};

    if (argc > 1)
    {
        strcpy(ctrltype, argv[1]);
        printf("ctrl:%s\n", ctrltype);
    }
    else
    {
        printf("===================\n");
        printf("watchdog [start/reset] [time]\n");
        printf("===================\n");
        return -1;
    }

    // set a signal, if press "ctrl+c", it will exit watchdog
    signal(SIGINT, exit_watchdog);

    if (strcmp("start", ctrltype) == 0)
    {
        if (argc == 3)
        {
            char times[32] = {0};
            strcpy(times, argv[2]);
            time = atoi(times);
        }
        printf("timeout:%ds\n", time);
    }
    else if (strcmp("reset", ctrltype) == 0)
    {
        if (argc == 3)
        {
            char times[32] = {0};
            strcpy(times, argv[2]);
            time = atoi(times);
        }
        printf("timeout:%ds\n", time);

        keep_alive_flag = 0;
        printf("just start the watchdog and don't to ping it\n");
    }
    else
    {
        printf("===================\n");
        printf("watchdog [start/reset] [time]\n");
        printf("===================\n");
        return -1;
    }

    open_watchdog();

    if (time > 0)
    {
        set_watchdog_timeout(time);
        set_watchdog_pretimeout(time / 4);
    }
    else
    {
        printf("set_watchdog_timeout err\n");
        return -1;
    }

    if (keep_alive_flag)
    {
        printf("now it will do keep watchdog alive...\n");
        itime = (time * 1000 * 1000) / 2;
    }
    else
    {
        itime = (time * 1000 * 1000) * 2;
    }

    while (1)
    {
        set_watchdog_keep_alive();
        usleep(itime);
    }
    return 0;
}

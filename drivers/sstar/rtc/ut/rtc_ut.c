/*
 * rtc_ut.c- Sigmastar
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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <linux/rtc.h>

#define FLAG_SET   0x0001
#define FLAG_ALARM 0x0100

#define DEVICE_NAME "/dev/rtc0"

struct rtc_ut_t
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int flag;
    int offset;
};

static struct rtc_time   rtc_tm;
static struct rtc_wkalrm rtc_alm;
static struct rtc_ut_t   rtc_ut;

static void print_usage(const char *prog)
{
    printf("Usage: %s [-wayMdhms]\n", prog);
    puts(
        "  -w --write    set time or alarm (default read)\n"
        "  -a --alarm    read or set alarm or time (default time)\n"
        "  -y --year     specify the year (1900~)\n"
        "  -M --month    specify the month (1-12)\n"
        "  -d --day      specify the day (0-31)\n"
        "  -h --hour     specify the hour (0-23)\n"
        "  -m --minute   specify the minute (0-60)\n"
        "  -s --second   specify the second (0-60)\n"
        "  -o --offset   specify the offset (unit second)\n");
    exit(1);
}

static void parse_opts(int argc, char *argv[])
{
    while (1)
    {
        static const struct option lopts[] = {
            {"write", 0, 0, 'w'},  {"alarm", 0, 0, 'a'}, {"year", 1, 0, 'y'},   {"month", 1, 0, 'M'},
            {"day", 1, 0, 'd'},    {"hour", 1, 0, 'h'},  {"minute", 1, 0, 'm'}, {"second", 1, 0, 's'},
            {"offset", 1, 0, 'o'}, {NULL, 0, 0, 0},
        };
        int c;

        c = getopt_long(argc, argv, "way:M:d:h:m:s:o:", lopts, NULL);

        if (c == -1)
            break;

        switch (c)
        {
            case 'w':
                rtc_ut.flag |= FLAG_SET;
                break;
            case 'a':
                rtc_ut.flag |= FLAG_ALARM;
                break;
            case 'y':
                rtc_ut.year = atoi(optarg);
                break;
            case 'M':
                rtc_ut.month = atoi(optarg);
                break;
            case 'd':
                rtc_ut.day = atoi(optarg);
                break;
            case 'h':
                rtc_ut.hour = atoi(optarg);
                break;
            case 'm':
                rtc_ut.minute = atoi(optarg);
                break;
            case 's':
                rtc_ut.second = atoi(optarg);
                break;
            case 'o':
                rtc_ut.offset = atoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                break;
        }
    }

    if (((rtc_ut.year < 1900 || rtc_ut.month < 1) && (rtc_ut.flag & FLAG_SET))
        && ((!rtc_ut.offset) && (rtc_ut.flag & FLAG_SET)))
        print_usage(argv[0]);
}

static void read_time(void)
{
    int fd;

    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0)
    {
        printf(DEVICE_NAME " open fail\n");
        exit(1);
    }

    if (ioctl(fd, RTC_RD_TIME, &rtc_tm))
    {
        printf("RTC_RD_TIME ioctl fail\n");
        goto err;
    }

    printf("current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n", rtc_tm.tm_mday, rtc_tm.tm_mon + 1,
           rtc_tm.tm_year + 1900, rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
err:
    close(fd);
}

static void read_alarm(void)
{
    int fd;

    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0)
    {
        printf(DEVICE_NAME " open fail\n");
        exit(1);
    }

    if (ioctl(fd, RTC_WKALM_RD, &rtc_alm))
    {
        printf("RTC_WKALM_RD ioctl fail\n");
        goto err;
    }

    printf("RTC alarm date/time is %d-%d-%d, %02d:%02d:%02d.\n", rtc_alm.time.tm_mday, rtc_alm.time.tm_mon + 1,
           rtc_alm.time.tm_year + 1900, rtc_alm.time.tm_hour, rtc_alm.time.tm_min, rtc_alm.time.tm_sec);
err:
    close(fd);
}

static void set_time(void)
{
    int fd;

    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0)
    {
        printf(DEVICE_NAME " open fail\n");
        exit(1);
    }

    if (rtc_ut.offset)
    {
        if (ioctl(fd, RTC_RD_TIME, &rtc_tm))
        {
            printf("RTC_RD_TIME ioctl fail\n");
            goto err;
        }

        rtc_tm.tm_sec += rtc_ut.offset;
        if (rtc_tm.tm_sec >= 60)
        {
            rtc_tm.tm_sec %= 60;
            rtc_tm.tm_min += rtc_ut.offset / 60;
        }
        if (rtc_tm.tm_min == 60)
        {
            rtc_tm.tm_min %= 60;
            rtc_tm.tm_hour += rtc_ut.offset / 3600;
        }
        if (rtc_tm.tm_hour == 24)
        {
            printf("offset unsupport cross day\n");
            goto err;
        }

        printf("%d-%d-%d, %02d:%02d:%02d.\n", rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900, rtc_tm.tm_hour,
               rtc_tm.tm_min, rtc_tm.tm_sec);

        if (ioctl(fd, RTC_SET_TIME, &rtc_tm))
        {
            printf("RTC_SET_TIME ioctl fail\n");
            goto err;
        }
    }
    else
    {
        rtc_tm.tm_year = rtc_ut.year - 1900;
        rtc_tm.tm_mon  = rtc_ut.month - 1;
        rtc_tm.tm_mday = rtc_ut.day;
        rtc_tm.tm_hour = rtc_ut.hour;
        rtc_tm.tm_min  = rtc_ut.minute;
        rtc_tm.tm_sec  = rtc_ut.second;

        printf("%d-%d-%d, %02d:%02d:%02d.\n", rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900, rtc_tm.tm_hour,
               rtc_tm.tm_min, rtc_tm.tm_sec);

        if (ioctl(fd, RTC_SET_TIME, &rtc_tm))
        {
            printf("RTC_SET_TIME time is error\n");
            goto err;
        }
    }

err:
    close(fd);
}

void set_alarm(void)
{
    int fd;

    fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0)
    {
        printf(DEVICE_NAME " open fail\n");
        exit(1);
    }

    if (rtc_ut.offset)
    {
        if (ioctl(fd, RTC_RD_TIME, &rtc_tm))
        {
            printf("RTC_RD_TIME ioctl fail\n");
            goto err;
        }

        rtc_alm.enabled = 1;
        memcpy(&rtc_alm.time, &rtc_tm, sizeof(struct rtc_time));

        rtc_alm.time.tm_sec += rtc_ut.offset;
        if (rtc_alm.time.tm_sec >= 60)
        {
            rtc_alm.time.tm_sec %= 60;
            rtc_alm.time.tm_min += rtc_ut.offset / 60;
        }
        if (rtc_alm.time.tm_min == 60)
        {
            rtc_alm.time.tm_min %= 60;
            rtc_alm.time.tm_hour += rtc_ut.offset / 3600;
        }
        if (rtc_alm.time.tm_hour == 24)
        {
            printf("offset unsupport cross day\n");
            goto err;
        }

        printf("%d-%d-%d, %02d:%02d:%02d.\n", rtc_alm.time.tm_mday, rtc_alm.time.tm_mon + 1,
               rtc_alm.time.tm_year + 1900, rtc_alm.time.tm_hour, rtc_alm.time.tm_min, rtc_alm.time.tm_sec);

        if (ioctl(fd, RTC_WKALM_SET, &rtc_alm))
        {
            perror("RTC_WKALM_SET ioctl");
            goto err;
        }
    }
    else
    {
        rtc_alm.enabled      = 1;
        rtc_alm.time.tm_year = rtc_ut.year - 1900;
        rtc_alm.time.tm_mon  = rtc_ut.month - 1;
        rtc_alm.time.tm_mday = rtc_ut.day;
        rtc_alm.time.tm_hour = rtc_ut.hour;
        rtc_alm.time.tm_min  = rtc_ut.minute;
        rtc_alm.time.tm_sec  = rtc_ut.second;

        printf("%d-%d-%d, %02d:%02d:%02d.\n", rtc_alm.time.tm_mday, rtc_alm.time.tm_mon + 1,
               rtc_alm.time.tm_year + 1900, rtc_alm.time.tm_hour, rtc_alm.time.tm_min, rtc_alm.time.tm_sec);

        if (ioctl(fd, RTC_WKALM_SET, &rtc_alm))
        {
            perror("RTC_WKALM_SET ioctl");
            goto err;
        }
    }

err:
    close(fd);
}

int main(int argc, char *argv[])
{
    int ret = 0;

    parse_opts(argc, argv);

    switch (rtc_ut.flag)
    {
        case 0:
            read_time();
            break;
        case FLAG_ALARM:
            read_alarm();
            break;
        case FLAG_SET:
            set_time();
            break;
        case FLAG_SET | FLAG_ALARM:
            set_alarm();
            break;
    }

    return ret;
}

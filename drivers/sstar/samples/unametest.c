/*
* unametest.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/utsname.h>
/*
The output is as following:
system name = Linux
node name   = (none)
release     = 3.18.14
version     = #421 SMP PREEMPT Thu Jun 11 07:05:52 CST 2015
machine     = armv7l
*/
int main(void) {

   struct utsname buffer;

   errno = 0;
   if (uname(&buffer) != 0) {
      perror("uname");
      exit(EXIT_FAILURE);
   }

   printf("system name = %s\n", buffer.sysname);
   printf("node name   = %s\n", buffer.nodename);
   printf("release     = %s\n", buffer.release);
   printf("version     = %s\n", buffer.version);
   printf("machine     = %s\n", buffer.machine);

   #ifdef _GNU_SOURCE
      printf("domain name = %s\n", buffer.domainname);
   #endif

   return EXIT_SUCCESS;
}

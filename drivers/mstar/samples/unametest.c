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

#ifndef _HWCLOCK_INT_H
#define _HWCLOCK_INT_H

/**
     Initialize system timer

     Initialize system timer of counter and longcounter, this API will be called when 1st task start
*/
extern void hwclock_init(void);

/**
     Un-initialize system timer.

*/
extern void hwclock_exit(void);

#endif /* _HWCLOCK_INT_H */


#ifndef _ISP_LIB_H_
#define _ISP_LIB_H_

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define ABS(x)   ((x) < 0 ? (-1 * (x)):(x))
#define CLAMP(x, min, max)    (((x) >= (max)) ? (max) : (((x) <= (min)) ? (min) : (x)))

#endif

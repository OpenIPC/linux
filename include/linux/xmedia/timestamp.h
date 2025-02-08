/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#ifdef CONFIG_ARCH_XM720XXX
typedef u64 timestamp_type;
#else
typedef u32 timestamp_type;
#endif

#pragma pack(4)
typedef struct {
    timestamp_type stamp;
    char *func;
    unsigned int line;
    unsigned int type;
} timestamp_item;
#pragma pack()

/* TimeStamp log funtions */
int timestamp_init(unsigned int count);
void timestamp_deinit(void);
int timestamp_mark(const char *func, unsigned int line, unsigned int type);
int timestamp_markex(const timestamp_item *item);
int timestamp_clean(void);
int timestamp_print(unsigned int type);
timestamp_type timestamp_get_ms(void);

/* stopwatch function */
void stopwatch_print(void);
int stopwatch_clear(void);
int stopwatch_trigger(void);


/* TimeStamp log macro define */
#define TIME_STAMP(x) timestamp_mark(__func__, __LINE__, x)
#define TIME_STAMP_CLEAN() timestamp_clean()
#define TIME_STAMP_PRINT(x) timestamp_print(x)

#endif /* End of #ifndef TIMESTAMP_H */

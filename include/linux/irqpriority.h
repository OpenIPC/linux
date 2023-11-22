/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IRQPRIORITY_H
#define _LINUX_IRQPRIORITY_H

/**
 * enum irqpriority
 * @IRQP_HIGH       address to low response latency interrupt e.g. error
 *          signaling
 * @IRQP_DEFAULT    default priority and set for all interrupt sources
 *          during interrupt controller initialization
 * @IRQP_LOW        interrupt which doesn't really care about response
 *          latency
 * @...         place for priority extension
 */
enum irqpriority {
    IRQP_HIGH = 0,
    IRQP_DEFAULT,
    IRQP_LOW,

    IRQP_LEVELS_NR
};

typedef enum irqpriority irqpriority_t;

#endif /* _LINUX_IRQPRIORITY_H */

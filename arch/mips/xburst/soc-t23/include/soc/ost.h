/*
 * JZ T20 ost register definition.
 *
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __OST_H__
#define __OST_H__

#define OST_TCCR    (0x00)   /* OS Timer Clock Control Register*/
#define OST_TER     (0x04)   /* OS Timer Counter Enable Register */
#define OST_TESR    (0x34)   /* OS Timer Counter Enable Set Register */
#define OST_TECR    (0x38)   /* OS Timer Counter Enable Clear Register */
#define OST_TCR     (0x08)   /* OS Timer clear Register */
#define OST_TFR     (0x0C)   /* OS Timer Flag Register */
#define OST_TMR     (0x10)   /* OS Timer Mask Register */
#define OST_T1DFR   (0x14)   /* OS Timer1 Data FULL Register */
#define OST_T1CNT   (0x18)   /* OS Timer1 Counter */
#define OST_T2CNTL  (0x20)   /* OS Timer2 Counter Lower 32 Bits */
#define OST_T2CNTH  (0x1c)   /* OS Timer2 Counter High 32 Bits */
#define OST_TCNT2HBUF   (0x24)   /* OS Timer2 Counter Higher 32 Bits Buffer */

#define TESR_OSTEN1   (1 << 0 )   /* enable the counter1 in ost */
#define TESR_OSTEN2   (1 << 1 )   /* enable the counter2 in ost */
#define TMR_OSTM    (1 << 0 )   /* ost comparison match interrupt mask */
#define TMR_OSTN    (0 << 0 )   /* ost comparison match interrupt no mask */
#define TFR_OSTM    (1 << 0)   /* Comparison match */
#define TFR_OSTN    (0 << 0)   /* Comparison not match */
#define TSR_OSTS    (1 << 15)   /*the clock supplies to osts is stopped */

#endif

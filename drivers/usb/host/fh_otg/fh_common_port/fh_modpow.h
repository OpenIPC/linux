/*
 * fh_modpow.h
 * See fh_modpow.c for license and changes
 */
#ifndef _FH_MODPOW_H
#define _FH_MODPOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fh_os.h"

/** @file
 *
 * This file defines the module exponentiation function which is only used
 * internally by the FH UWB modules for calculation of PKs during numeric
 * association.  The routine is taken from the PUTTY, an open source terminal
 * emulator.  The PUTTY License is preserved in the fh_modpow.c file.
 *
 */

typedef uint32_t BignumInt;
typedef uint64_t BignumDblInt;
typedef BignumInt *Bignum;

/* Compute modular exponentiaion */
extern Bignum fh_modpow(void *mem_ctx, Bignum base_in, Bignum exp, Bignum mod);

#ifdef __cplusplus
}
#endif

#endif /* _LINUX_BIGNUM_H */

/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_AXERA_EIP130_H
#define _UAPI_LINUX_AXERA_EIP130_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define AXERA_EIP130_ABI 1U
#define AXERA_EIP130_MAX_INPUT 65552U
#define AXERA_EIP130_ALG_ECB 1U
#define AXERA_EIP130_ALG_CTR 2U

/* One key/algorithm per open fd. All reserved fields must be zero. */
struct axera_eip130_key {
	__u32 version;
	__u32 algorithm;
	__u32 key_size;
	__u32 reserved;
	__u8 key[32];
};

/* Process addresses, never DMA/physical addresses. Synchronous completion;
 * input may equal output. output_size is zero on failure. CTR uses a complete
 * 128-bit big-endian IV; ECB requires iv_size=0 and whole AES blocks.
 */
struct axera_eip130_request {
	__u32 version;
	__u32 flags;
	__aligned_u64 src;
	__aligned_u64 dst;
	__u32 size;
	__u32 output_size;
	__u8 iv[16];
	__u32 iv_size;
	__u32 reserved;
};

#define AXERA_EIP130_SET_KEY _IOW('E', 0x30, struct axera_eip130_key)
#define AXERA_EIP130_RUN _IOWR('E', 0x31, struct axera_eip130_request)

#endif

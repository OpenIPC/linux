/*
 * Copyright (c) 2007-2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file    ti/cmem.h
 * @brief   Describes the interface to the contiguous memory allocator.
 *
 * The cmem user interface library wraps file system calls to an associated
 * kernel module (cmemk.ko), which needs to be loaded in order for calls to
 * this library to succeed.
 *
 * The CMEM kernel module accepts blocks of physical memory and a pool
 * geometry within those blocks.  The blocks and pools can be specified
 * either in the traditional command line method, or with the new DT
 * (Device Tree) method.  The DT method is the prefered method due to
 * its control by a system integrator.
 *
 * Command line block and pool specification
 * -----------------------------------------
 *
 * The following is an example of installing the cmem kernel module using
 * the traditional command line block and pool parameters (a DT example is
 * shown below):
 *
 * @verbatim /sbin/insmod cmemk.ko pools=4x30000,2x500000 phys_start=0x0 phys_end=0x3000000 @endverbatim
 *     - phys_start and phys_end must be specified in hexadecimal format
 *     - phys_start is "inclusive" and phys_end is "exclusive", i.e.,
 *       phys_end should be "end address + 1".
 *     - pools must be specified using decimal format (for both number and
 *       size), since using hexadecimal format would visually clutter the
 *       specification due to the use of "x" as a token separator
 *     - it's possible to insmod cmemk.ko without specifying any memory blocks,
 *       in which case CMEM_getPhys() and CMEM_cache*() APIs can still be
 *       utilized by an application, and CMA-based allocations can still be
 *       accomplished (by using blockid as CMEM_CMABLOCKID).
 *
 * This particular command creates 2 pools. The first pool is created with 4
 * buffers of size 30000 bytes and the second pool is created with 2 buffers
 * of size 500000 bytes. The CMEM pool buffers start at 0x0 and end at
 * 0x2FFFFFF (max).
 *
 * There is also support for a 2nd, 3rd, or 4th contiguous memory block to
 * be specified, with all the same features supported for those blocks as
 * with the 1st.  This 2nd block is specified with *_1 parameters (and 3rd
 * block with *_2, 4th block with *_3).  The following example expands upon
 * the first example above:
 *
 * @verbatim /sbin/insmod cmemk.ko pools=4x30000,2x500000 phys_start=0x0 phys_end=0x3000000
    pools_1=4x65536 phys_start_1=0x80000000 phys_end_1=0x80010000 @endverbatim
 *
 * This particular command, in addition to the pools explained above,
 * creates 1 pool (with 4 buffers of size 64KB) in a 2nd memory block which
 * starts at 0x80000000 and ends at 0x8000FFFF (specified as "end + 1" on the
 * insmod command).
 *
 * In order to access blocks beyond the 1st block, new APIs have been added to
 * CMEM which allow specification of the block ID.
 *
 * There is another configuration "switch" for the cmemk.ko kernel module,
 * which can be specified on the 'insmod' (or 'modprobe') command lines:
 *     useHeapIfPoolUnavailable=[0|1]
 *
 * 'useHeapIfPoolUnavailable', when set to 1, will cause pool-based allocations
 * to fallback to a heap-based allocation if no pool buffer of sufficient size
 * is available (the CMEM heap is described below).
 *
 * Pool buffers are aligned on a module-dependent boundary, and their sizes are
 * rounded up to this same boundary.  This applies to each buffer within a
 * pool.  The total space used by an individual pool will therefore be greater
 * than (or equal to) the exact amount requested in the installation of the
 * module.
 *
 * The poolid used in the driver calls would be 0 for the first pool and 1 for
 * the second pool.
 *
 * Pool allocations can be requested explicitly by pool number, or more
 * generally by just a size.  For size-based allocations, the pool which best
 * fits the requested size is automatically chosen.  Some CMEM APIs (newer
 * ones) accept a blockid as a parameter, in order to specify which of the
 * multiple blocks to operate on.  For 'legacy' APIs (ones that existed before
 * the support for multiple blocks) where a blockid is still needed, block 0
 * is assumed.
 *
 * There is also support for a general purpose heap.  In addition to the 2
 * pools described above, a general purpose heap block is created from which
 * allocations of any size can be requested.  Internally, allocation sizes are
 * rounded up to a module-dependent boundary and allocation addresses are
 * aligned either to this same boundary or to the requested alignment
 * (whichever is greater).
 *
 * The size of the heap block is the amount of CMEM memory remaining after all
 * pool allocations.  If more heap space is needed than is available after pool
 * allocations, you must reduce the amount of CMEM memory granted to the pools.
 *
 * Buffer allocation is tracked at the file descriptor level by way of a
 * 'registration' list.  The initial allocator of a buffer (the process that
 * calls CMEM_alloc()) is automatically added to the registration list,
 * and further processes can become registered for the same buffer by way
 * of the CMEM_registerAlloc() API (and unregister with the
 * CMEM_unregister() API).  This registration list for each buffer
 * allows for buffer ownership tracking and cleanup on a
 * per-file-descriptor basis, so that when a process exits or dies without
 * having explicitly freed/unregistered its buffers, they get automatically
 * unregistered (and freed when no more registered file descriptors exist).
 * Only when the last registered file descriptor frees a buffer (either
 * explictily, or by auto-cleanup) does a buffer actually get freed back to
 * the kernel module.
 *
 * Block and pool specification using Device Tree (DT)
 * ---------------------------------------------------
 *
 * The Device Tree can be used to specify physical memory blocks and pool
 * geometry within those blocks, with the same capabilities as the command
 * line specification.  DT specification is preferred over command line
 * specification for a number of reasons:
 *   - allows specifying the physical "carveout" with direct linkage to
 *     that carveout in the pool specification, thereby eliminating potential
 *     errors that could occur when the physical carveout is defined in one
 *     place and then needing to be specified on the cmemk.ko command line
 *     exactly equivalent to the carveout definition.
 *   - allows a system integrator to define the CMEM specifications without
 *     the need to document instructions for the command line, or prepackage
 *     the command line in some filesystem-based script.
 *
 * CMEM allows both DT-specified and command line specified blocks.  Each
 * block is assigned a number that can be later used for allocations in
 * order to direct the allocation to a particular block.  This allows some
 * blocks to be defined in the DT and other blocks to be later defined
 * on the CMEM command line.  If a particular block number is assigned in
 * DT and also specified on the CMEM command line then the DT specification
 * takes precedence and the command line specification is ignored.
 *
 * The following DT example snippet illustrates the syntax...
 *
 * @code 
 * // Specify reserved physical blocks, using "no-map" to keep Linux away.
 * // The "reg" property is the base address and size in 32-bit ints.
 * // This is the generic "reserved-memory" node that might also contain
 * // other non-CMEM entries.
 *
 *      reserved-memory {
 *              #address-cells = <1>;
 *              #size-cells = <1>;
 *              ranges;
 *      ...
 *              cmem_block_mem_0: cmem_block_mem@a0000000 {
 *                      reg = <0xa0000000 0x10000000>;
 *                      no-map;
 *                      status = "okay";
 *              };
 *
 *              cmem_block_mem_1: cmem_block_mem@b0000000 {
 *                      reg = <0xb0000000 0x02000000>;
 *                      no-map;
 *                      status = "okay";
 *              };
 *      };
 *
 * // The CMEM node is specified below, and can be named arbitrarily ("cmem"
 * // name chosen here).  The node's "compatible" property must contain
 * // "ti,cmem" in order for CMEM to process it.
 *
 *      cmem {
 *              compatible = "ti,cmem";  // must be "ti,cmem"
 *              #address-cells = <1>;
 *              #size-cells = <0>;
 *
 *              status = "okay";
 *
 * // CMEM block specifications along with pool geometry.
 * //
 * // The "reg" property is the block identifier, and each child of the
 * // cmem node must specify a different "reg" value.  The domain of
 * // the block identifier is 0 -> <nblocks - 1>, where nblocks is the
 * // total amount of DT-specified and command line-specified blocks.
 * // CMEM currently supports up to 4 blocks (although that can be increased
 * // by simple comment-directed modifications in cmemk.c).
 * //
 * // The "memory-region" property points to the phandle of the
 * // reserved-memory area.
 * //
 * // The "cmem-buf-pools" property contains a variable number of comma-
 * // separated pools, with each pool specifying the number of buffers and
 * // the size of each buffer in <n size> format.
 *
 *              cmem_block_0: cmem_block@0 {
 *                      reg = <0>;
 *                      memory-region = <&cmem_block_mem_0>;
 *                      cmem-buf-pools = <1 0x0c000000>, <4 0x01000000>; // 1x256MB, 4x16MB pools
 *              };
 *
 *              cmem_block_1: cmem_block@1 {
 *                      reg = <1>;
 *                      memory-region = <&cmem_block_mem_1>;
 *                      cmem-buf-pools = <1 0x02000000>; // 1x32MB pool, remaining carveout is heap.
 *              };
 *      };
 * @endcode
 *
 * Decoupled allocation & memory mapping
 * -------------------------------------
 *
 * For every CMEM_alloc*() API, there is a corresponding "Phys" API that
 * returns the physical address of the allocation and does *not* do the
 * user-space memory mapping (mmap()).  There is also a corresponding
 * CMEM_freePhys() to be used for freeing allocations performed by the
 * "Phys" APIs.  The user-space memory mapping is performed with the
 * CMEM_map()/CMEM_unmap() APIs.  This allows large buffer allocations
 * without the burden of using up a large portion of the application's
 * virtual address space for the duration of the allocation's existence.
 * A mapping performed by CMEM_map() doesn't have to be for the whole of
 * the allocated buffer, it can be for just portions of the buffer (but
 * must not be larger than the allocated buffer).
 *
 * CMEM_map() can be called multiple times for the same physical allocation.
 * Each invocation of CMEM_map() will allocate a new user virtual address
 * range for the same physical allocation.
 *
 * CMA access
 * ----------
 *
 * On systems with CMA (Contiguous Memory Allocator) support, CMEM can be used
 * to allocate CMA buffers.  Both heap and pool allocations are supported.  A
 * special block ID, CMEM_CMABLOCKID, is used for these allocations, and
 * therefore the block-based CMEM APIs must be used (the ones ending in "2",
 * such as CMEM_alloc2()), where the 'blockid' parameter is CMEM_CMABLOCKID
 * (instead of 0 or 1).
 *
 * CMA always contains, at the minimum, a global area that can be accessed
 * by any device driver.  CMEM treats this global area as a heap, which means
 * that heap-based allocations can be performed on CMEM_CMABLOCKID.
 * See the Linux CMA documentation for details on specifying this global area.
 * Keep in mind that other Linux kernel entities can (and will) also allocate
 * memory from this global area, so you never really know how much is
 * available for allocations by CMEM.
 *
 * To achieve pool-based allocations from CMA, a special CMEM stub must be
 * builtin to the kernel, and CMEM must be built with CMEM_KERNEL_STUB=1 for
 * the 'make' command (which turns into -DCMEM_KERNEL_STUB on the compile
 * command).  This stub processes a kernel command line parameter named
 * "cmem_cma_pools=..." in the same fashion as the standard CMEM "pools=...".
 * A separate CMA region (area) is created for each of the separate
 * "cmem_cma_pools" (where each pool/region is one element of the comma-
 * separated list).
 *
 * The CMEM builtin kernel stub is located in
 *     .../src/cmem/module/kernel/drivers/cmem/cmemk_stub.c
 * You don't need this stub if you want to allocate only from the CMA
 * global area.  Please see comments at the head of the above file for
 * instructions on how to incorporate it into your kernel.
 *
 * In supporting the pool model of CMEM, a separate CMA region is created for
 * each pool.  A pool consists of n buffers of size s, resulting in a region
 * that is sized accordingly.  CMA places no restrictions on the size of an
 * allocation from a region, so the buffer size 's' becomes more of a
 * convention than a hard rule.  Given enough free space, any pool can be
 * used to allocate a buffer of any size, but it is recommended to partition
 * regions (or, in this case, pools) according to the size of buffers that
 * will be allocated in order to reduce fragmentation and wasted memory due
 * to size-based alignment padding.  In other words, if your system will be
 * allocating 10 buffers of size 4096, 8 of size 131072, and 4 of size
 * 1048576, create pools of
 *     cmem_cma_pools=10x4096,8x131072,4x1048576
 * and perform allocations from the respective pool.
 *
 * An application doesn't need to know the pool IDs that correspond to the
 * respective sizes of that pool's buffers.  The CMEM_alloc2() API decides
 * which pool to use according to the requested size of allocation, and will
 * choose the one that fits most efficiently.  Unlike non-CMA pool allocations
 * where, when the "best fit" pool has no remaining buffers it will choose a
 * pool with larger available buffers, CMEM_alloc2() won't "promote" a
 * CMA-based pool allocation to a pool with buffers larger than the the best
 * fit.
 *
 * CMEM supports a kernel command line parameter named "cmem_cma_heapsize".
 * When assigned, a CMA region of size "cmem_cma_heapsize" is created for use
 * by CMEM heap allocations for the CMA "block".  This heap is then used for
 * CMEM CMA heap allocations, and the CMA global area is no longer used by
 * CMEM.  This allows CMEM to have full usage of the CMEM CMA heap without
 * possible allocations by non-CMEM entities in the kernel.
 */
/**
 *  @defgroup   ti_CMEM  Contiguous Memory Manager
 *
 *  @copydoc ti/cmem.h
 */

#ifndef ti_CMEM_H
#define ti_CMEM_H

#if defined (__cplusplus)
extern "C" {
#endif

/** @ingroup    ti_CMEM */
/*@{*/

#define CMEM_VERSION    0x04000300U

#define MAX_POOLS 32

/* magic "block" corresponding to CMA regions */
#define CMEM_CMABLOCKID			-1

/* ioctl cmd "flavors" */
#define CMEM_WB				0x00010000
#define CMEM_INV			0x00020000
#define CMEM_HEAP			0x00040000  /**< operation applies to heap */
#define CMEM_POOL			0x00000000  /**< operation applies to a pool */
#define CMEM_CACHED			0x00080000  /**< allocated buffer is cached */
#define CMEM_NONCACHED			0x00000000  /**< allocated buffer is not cached */
#define CMEM_PHYS			0x00100000

#define CMEM_IOCMAGIC			0x0000fe00

/* supported "base" ioctl cmds for the driver. */
#define CMEM_IOCALLOC                   1
#define CMEM_IOCALLOCHEAP               2
#define CMEM_IOCFREE                    3
#define CMEM_IOCGETPHYS                 4
#define CMEM_IOCGETSIZE                 5
#define CMEM_IOCGETPOOL                 6
#define CMEM_IOCCACHE                   7
#define CMEM_IOCGETVERSION              8
#define CMEM_IOCGETBLOCK                9
#define CMEM_IOCREGUSER                 10
#define CMEM_IOCGETNUMBLOCKS            11
#define CMEM_IOCCACHEWBINVALL           12
/*
 * New ioctl cmds should use integers greater than the largest current cmd
 * in order to not break backward compatibility.
 */

/* supported "flavors" to "base" ioctl cmds for the driver. */
#define CMEM_IOCCACHEWBINV              CMEM_IOCCACHE | CMEM_WB | CMEM_INV
#define CMEM_IOCCACHEWB                 CMEM_IOCCACHE | CMEM_WB
#define CMEM_IOCCACHEINV                CMEM_IOCCACHE | CMEM_INV
#define CMEM_IOCALLOCCACHED             CMEM_IOCALLOC | CMEM_CACHED
#define CMEM_IOCALLOCHEAPCACHED         CMEM_IOCALLOCHEAP | CMEM_CACHED
#define CMEM_IOCFREEHEAP                CMEM_IOCFREE | CMEM_HEAP
#define CMEM_IOCFREEPHYS                CMEM_IOCFREE | CMEM_PHYS
#define CMEM_IOCFREEHEAPPHYS            CMEM_IOCFREE | CMEM_HEAP | CMEM_PHYS

#define CMEM_IOCCMDMASK			0x000000ff

/**
 * @brief Parameters for CMEM_alloc(), CMEM_alloc2(), CMEM_allocPool(),
 * CMEM_allocPool2(), CMEM_free().
 */
typedef struct CMEM_AllocParams {
    int type;		/**< either #CMEM_HEAP or #CMEM_POOL */
    int flags;		/**< either #CMEM_CACHED or #CMEM_NONCACHED */
    size_t alignment;	/**<
                         * only used for heap allocations, must be power of 2
                         */
} CMEM_AllocParams;

extern CMEM_AllocParams CMEM_DEFAULTPARAMS;

typedef struct CMEM_BlockAttrs {
    off_t phys_base;
    unsigned long long size;
} CMEM_BlockAttrs;

/** @cond INTERNAL */

/**
 */
union CMEM_AllocUnion {
    struct {			/**< */
	size_t size;
	size_t align;
	int blockid;
    } alloc_heap_inparams;	/**< */
    struct {			/**< */
	int poolid;
	int blockid;
    } alloc_pool_inparams;	/**< */
    struct {			/**< */
	int poolid;
	int blockid;
    } get_size_inparams;	/**< */
    struct {			/**< */
	unsigned long long size;
	int blockid;
    } get_pool_inparams;	/**< */
    struct {			/**< */
	unsigned long long physp;
	unsigned long long size;
    } alloc_pool_outparams;	/**< */
    struct {			/**< */
	unsigned long long physp;
	unsigned long long size;
    } get_block_outparams;	/**< */
    struct {			/**< */
	int poolid;
	size_t size;
    } free_outparams;		/**< */
    unsigned long long physp;
    void *virtp;
    size_t size;
    int poolid;
    int blockid;
};

/** @endcond */

/**
 * @brief Initialize the CMEM module. Must be called before other API calls.
 *
 * @return 0 for success or -1 for failure.
 *
 * @sa CMEM_exit
 */
int CMEM_init(void);

/**
 * @brief Find the pool that best fits a given buffer size and has a buffer
 * available.
 *
 * @param   size    The buffer size for which a pool is needed.
 *
 * @return A poolid that can be passed to CMEM_allocPool(), or -1 for error.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_free()
 * @sa CMEM_getPool2()
 */
int CMEM_getPool(unsigned long long size);

/**
 * @brief Find the pool in memory block blockid that best fits a given
 * buffer size and has a buffer available.
 *
 * @param   blockid  Block number
 * @param   size     The buffer size for which a pool is needed.
 *
 * @return A poolid that can be passed to CMEM_allocPool2(), or -1 for error.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_free()
 * @sa CMEM_getPool()
 */
int CMEM_getPool2(int blockid, unsigned long long size);

/**
 * @brief Allocate memory from a specified pool.
 *
 * @param   poolid  The pool from which to allocate memory.
 * @param   params  Allocation parameters.
 *
 * @remarks @c params->type is ignored - a pool will always be used.
 * @remarks @c params->alignment is unused, since pool buffers are already
 *          aligned to specific boundaries.
 *
 * @return A pointer to the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_alloc()
 * @sa CMEM_alloc2()
 * @sa CMEM_allocPool2()
 * @sa CMEM_registerAlloc()
 * @sa CMEM_unregister()
 * @sa CMEM_free()
 */
void *CMEM_allocPool(int poolid, CMEM_AllocParams *params);

/**
 * @brief Allocate memory from a specified pool in a specified memory block.
 *
 * @param   blockid The memory block from which to allocate.
 * @param   poolid  The pool from which to allocate memory.
 * @param   params  Allocation parameters.
 *
 * @remarks @c params->type is ignored - a pool will always be used.
 * @remarks @c params->alignment is unused, since pool buffers are already
 *          aligned to specific boundaries.
 *
 * @return A pointer to the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_alloc()
 * @sa CMEM_alloc2()
 * @sa CMEM_allocPool()
 * @sa CMEM_registerAlloc()
 * @sa CMEM_unregister()
 * @sa CMEM_free()
 */
void *CMEM_allocPool2(int blockid, int poolid, CMEM_AllocParams *params);

/**
 * @brief Allocate memory of a specified size
 *
 * @param   size    The size of the buffer to allocate.
 * @param   params  Allocation parameters.
 *
 * @remarks Used to allocate memory from either a pool, the heap, or the CMA
 *          global area.
 *          If doing a pool allocation, the pool that best fits the requested
 *          size will be selected.  Use CMEM_allocPool() to allocate from a
 *          specific pool.
 *          Allocation will be cached or noncached, as specified by params.
 *          params->alignment valid only for heap allocation.
 *
 * @return A pointer to the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_alloc2()
 * @sa CMEM_registerAlloc()
 * @sa CMEM_unregister()
 * @sa CMEM_free()
 */
void *CMEM_alloc(size_t size, CMEM_AllocParams *params);

/**
 * @brief Allocate memory of a specified size from a specified memory block
 *
 * @param   blockid The memory block from which to allocate.
 * @param   size    The size of the buffer to allocate.
 * @param   params  Allocation parameters.
 *
 * @remarks Used to allocate memory from either a pool, the heap, or the CMA
 *          global area.
 *          If doing a pool allocation, the pool that best fits the requested
 *          size will be selected.  Use CMEM_allocPool() to allocate from a
 *          specific pool.
 *
 * @remarks Allocation will be cached or noncached, as specified by params.
 *          params->alignment valid only for heap allocation.
 *
 * @return A pointer to the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_alloc()
 * @sa CMEM_registerAlloc()
 * @sa CMEM_unregister()
 * @sa CMEM_free()
 */
void *CMEM_alloc2(int blockid, size_t size, CMEM_AllocParams *params);

/**
 * @brief Allocate unmapped memory from a specified pool.
 *
 * @param   poolid  The pool from which to allocate memory.
 * @param   params  Allocation parameters.
 *
 * @remarks @c params->type is ignored - a pool will always be used.
 * @remarks @c params->alignment is unused, since pool buffers are already
 *          aligned to specific boundaries.
 *
 * @return The physical address of the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPhys()
 * @sa CMEM_allocPhys2()
 * @sa CMEM_allocPoolPhys2()
 * @sa CMEM_freePhys()
 * @sa CMEM_map()
 */
off_t CMEM_allocPoolPhys(int poolid, CMEM_AllocParams *params);

/**
 * @brief Allocate unmapped memory from a specified pool in a specified
 *        memory block.
 *
 * @param   blockid The memory block from which to allocate.
 * @param   poolid  The pool from which to allocate memory.
 * @param   params  Allocation parameters.
 *
 * @remarks @c params->type is ignored - a pool will always be used.
 * @remarks @c params->alignment is unused, since pool buffers are already
 *          aligned to specific boundaries.
 *
 * @return The physical address of the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPhys()
 * @sa CMEM_allocPhys2()
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_freePhys()
 * @sa CMEM_map()
 */
off_t CMEM_allocPoolPhys2(int blockid, int poolid, CMEM_AllocParams *params);

#if defined(LINUXUTILS_BUILDOS_ANDROID)
/**
 * @brief Allocate unmapped memory from a specified pool in a specified
 *        memory block (Android only).
 *
 * @param   blockid The memory block from which to allocate.
 * @param   poolid  The pool from which to allocate memory.
 * @param   params  Allocation parameters.
 *
 * @remarks @c params->type is ignored - a pool will always be used.
 * @remarks @c params->alignment is unused, since pool buffers are already
 *          aligned to specific boundaries.
 *
 * @return The physical address of the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPhys()
 * @sa CMEM_allocPhys2()
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_freePhys()
 * @sa CMEM_map()
 */
off64_t CMEM_allocPoolPhys64(int blockid, int poolid, CMEM_AllocParams *params);
#endif

/**
 * @brief Allocate unmapped memory of a specified size
 *
 * @param   size    The size of the buffer to allocate.
 * @param   params  Allocation parameters.
 *
 * @remarks Used to allocate memory from either a pool, the heap, or the CMA
 *          global area.
 *          If doing a pool allocation, the pool that best fits the requested
 *          size will be selected.  Use CMEM_allocPool() to allocate from a
 *          specific pool.
 *          Allocation will be cached or noncached, as specified by params.
 *          params->alignment valid only for heap allocation.
 *
 * @return The physical address of the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_allocPoolPhys2()
 * @sa CMEM_allocPhys2()
 * @sa CMEM_freePhys()
 * @sa CMEM_map()
 */
off_t CMEM_allocPhys(size_t size, CMEM_AllocParams *params);

/**
 * @brief Allocate unmapped memory of a specified size from a specified
 *        memory block
 *
 * @param   blockid The memory block from which to allocate.
 * @param   size    The size of the buffer to allocate.
 * @param   params  Allocation parameters.
 *
 * @remarks Used to allocate memory from either a pool, the heap, or the CMA
 *          global area.
 *          If doing a pool allocation, the pool that best fits the requested
 *          size will be selected.  Use CMEM_allocPool() to allocate from a
 *          specific pool.
 *
 * @remarks Allocation will be cached or noncached, as specified by params.
 *          params->alignment valid only for heap allocation.
 *
 * @return The physical address of the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_allocPoolPhys2()
 * @sa CMEM_allocPhys()
 * @sa CMEM_freePhys()
 * @sa CMEM_map()
 */
off_t CMEM_allocPhys2(int blockid, size_t size, CMEM_AllocParams *params);

#if defined(LINUXUTILS_BUILDOS_ANDROID)
/**
 * @brief Allocate unmapped memory of a specified size from a specified
 *        memory block (Android only)
 *
 * @param   blockid The memory block from which to allocate.
 * @param   size    The size of the buffer to allocate.
 * @param   params  Allocation parameters.
 *
 * @remarks Used to allocate memory from either a pool, the heap, or the CMA
 *          global area.
 *          If doing a pool allocation, the pool that best fits the requested
 *          size will be selected.  Use CMEM_allocPool() to allocate from a
 *          specific pool.
 *
 * @remarks Allocation will be cached or noncached, as specified by params.
 *          params->alignment valid only for heap allocation.
 *
 * @return The physical address of the allocated buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_allocPoolPhys2()
 * @sa CMEM_allocPhys()
 * @sa CMEM_freePhys()
 * @sa CMEM_map()
 */
off64_t CMEM_allocPhys64(int blockid, size_t size, CMEM_AllocParams *params);
#endif

/**
 * @brief Register shared usage of an already-allocated buffer
 *
 * @param   physp  Physical address of the already-allocated buffer.
 *
 * @remarks Used to register the calling process for usage of an
 *          already-allocated buffer, for the purpose of shared usage of
 *          the buffer.
 *
 * @remarks Allocation properties (such as cached/noncached or heap/pool)
 *          are inherited from original allocation call.
 *
 * @return  A process-specific pointer to the allocated buffer, or NULL
 *          for failure.
 *
 * @pre Must have called some form of CMEM_alloc*()
 *
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_alloc()
 * @sa CMEM_free()
 * @sa CMEM_unregister()
 */
void *CMEM_registerAlloc(off_t physp);

#if defined(LINUXUTILS_BUILDOS_ANDROID)
/**
 * @brief Register shared usage of an already-allocated buffer using mmap64
 *        (Android only)
 *
 * @param   physp  Physical address of the already-allocated buffer.
 *
 * @remarks Used to register the calling process for usage of an
 *          already-allocated buffer, for the purpose of shared usage of
 *          the buffer.
 *
 * @remarks Allocation properties (such as cached/noncached or heap/pool)
 *          are inherited from original allocation call.
 *
 * @return  A process-specific pointer to the allocated buffer, or NULL
 *          for failure.
 *
 * @pre Must have called some form of CMEM_alloc*()
 *
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_alloc()
 * @sa CMEM_free()
 * @sa CMEM_unregister()
 */
void *CMEM_registerAlloc64(off64_t physp);
#endif

/**
 * @brief Free a buffer previously allocated with
 *        CMEM_alloc()/CMEM_allocPool().
 *
 * @param   ptr     The pointer to the buffer.
 * @param   params  Allocation parameters.
 *
 * @remarks Use the same CMEM_AllocParams as was used for the allocation.
 *          params->flags is "don't care".  params->alignment is "don't
 *          care".
 *
 * @return 0 for success or -1 for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_alloc()
 * @sa CMEM_alloc2()
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_registerAlloc()
 * @sa CMEM_unregister()
 */
int CMEM_free(void *ptr, CMEM_AllocParams *params);

/**
 * @brief Free an unmapped buffer previously allocated with
 *        CMEM_allocPhys()/CMEM_allocPoolPhys().
 *
 * @param   physp   The physical address of the buffer.
 * @param   params  Allocation parameters.
 *
 * @remarks Use the same CMEM_AllocParams as was used for the allocation.
 *          params->flags is "don't care".  params->alignment is "don't
 *          care".
 *
 * @return 0 for success or -1 for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPhys()
 * @sa CMEM_allocPhys2()
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_allocPoolPhys2()
 * @sa CMEM_unmap()
 */
int CMEM_freePhys(off_t physp, CMEM_AllocParams *params);

#if defined(LINUXUTILS_BUILDOS_ANDROID)
/**
 * @brief Free an unmapped buffer previously allocated with
 *        CMEM_allocPhys64()/CMEM_allocPoolPhys64() (Android only).
 *
 * @param   physp   The physical address of the buffer.
 * @param   params  Allocation parameters.
 *
 * @remarks Use the same CMEM_AllocParams as was used for the allocation.
 *          params->flags is "don't care".  params->alignment is "don't
 *          care".
 *
 * @return 0 for success or -1 for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPhys()
 * @sa CMEM_allocPhys2()
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_allocPoolPhys2()
 * @sa CMEM_unmap()
 */
int CMEM_freePhys64(off64_t physp, CMEM_AllocParams *params);
#endif

/**
 * @brief Unregister use of a buffer previously registered with
 *        CMEM_registerAlloc().
 *
 * @param   ptr     The pointer to the buffer.
 * @param   params  Allocation parameters.
 *
 * @remarks Use the same CMEM_AllocParams as was used for the allocation.
 *          params->flags is "don't care".  params->alignment is "don't
 *          care".
 *
 * @return 0 for success or -1 for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_alloc()
 * @sa CMEM_alloc2()
 * @sa CMEM_allocPool()
 * @sa CMEM_allocPool2()
 * @sa CMEM_registerAlloc()
 * @sa CMEM_free()
 */
int CMEM_unregister(void *ptr, CMEM_AllocParams *params);

/**
 * @brief Get the physical address of a contiguous buffer.
 *
 * @param   ptr     The pointer to the buffer.
 *
 * @return The physical address of the buffer or 0 for failure.
 *
 * @pre Must have called CMEM_init()
 */
off_t CMEM_getPhys(void *ptr);

#if defined(LINUXUTILS_BUILDOS_ANDROID)
/**
 * @brief Get the physical address of a contiguous buffer (Android only).
 *
 * @param   ptr     The pointer to the buffer.
 *
 * @return The physical address of the buffer or 0 for failure.
 *
 * @pre Must have called CMEM_init()
 */
off64_t CMEM_getPhys64(void *ptr);
#endif

/**
 * @brief Do a cache writeback/invalidate of the whole cache
 *
 * @return Success/failure boolean value
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_cacheWb()
 * @sa CMEM_cacheInv()
 * @sa CMEM_cacheWbInv()
 */
int CMEM_cacheWbInvAll(void);

/**
 * @brief Do a cache writeback of the block pointed to by @c ptr/@c size
 *
 * @param   ptr     Pointer to block to writeback
 * @param   size    Size in bytes of block to writeback.
 *
 * @return Success/failure boolean value
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_cacheInv()
 * @sa CMEM_cacheWbInv()
 * @sa CMEM_cacheWbInvAll()
 */
int CMEM_cacheWb(void *ptr, size_t size);

/**
 * @brief Map allocated memory
 *
 * @param   physp   Physical address of the already-allocated buffer.
 * @param   size    Size in bytes of block to map.
 *
 * @return A pointer to the mapped buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPhys()
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_unmap()
 */
void *CMEM_map(off_t physp, size_t size);

#if defined(LINUXUTILS_BUILDOS_ANDROID)
/**
 * @brief Map allocated memory using mmap64 (Android only)
 *
 * @param   physp   Physical address of the already-allocated buffer.
 * @param   size    Size in bytes of block to map.
 *
 * @return A pointer to the mapped buffer, or NULL for failure.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_allocPhys()
 * @sa CMEM_allocPoolPhys()
 * @sa CMEM_unmap()
 */
void *CMEM_map64(off64_t physp, size_t size);
#endif

/**
 * @brief Unmap allocated memory
 *
 * @param   userp   User virtual address of the mapped buffer.
 * @param   size    Size in bytes of block to unmap.
 *
 * @return Success/failure boolean value
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_freePhys()
 * @sa CMEM_map()
 */
int CMEM_unmap(void *userp, size_t size);

/**
 * @brief Do a cache invalidate of the block pointed to by @c ptr/@c size
 *
 * @param   ptr     Pointer to block to invalidate
 * @param   size    Size in bytes of block to invalidate
 *
 * @return Success/failure boolean value
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_cacheWb()
 * @sa CMEM_cacheWbInv()
 * @sa CMEM_cacheWbInvAll()
 */
int CMEM_cacheInv(void *ptr, size_t size);

/**
 * @brief Do a cache writeback/invalidate of the block pointed to by
 *        @c ptr/@c size
 *
 * @param   ptr     Pointer to block to writeback/invalidate
 * @param   size    Size in bytes of block to writeback/invalidate
 *
 * @return Success/failure boolean value
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_cacheInv()
 * @sa CMEM_cacheWb()
 * @sa CMEM_cacheWbInvAll()
 */
int CMEM_cacheWbInv(void *ptr, size_t size);

/**
 * @brief Retrieve version from CMEM driver.
 *
 * @return Installed CMEM driver's version number.
 *
 * @pre Must have called CMEM_init()
 */
int CMEM_getVersion(void);

/**
 * @brief Retrieve memory block bounds from CMEM driver
 *
 * @param   pphys_base   Pointer to storage for base physical address of
 *                       CMEM's memory block
 * @param   psize        Pointer to storage for size of CMEM's memory block
 *
 * @return Success (0) or failure (-1).
 *
 * @pre Must have called CMEM_init()
 *
 * @sa  CMEM_getBlockAttrs()
 * @sa  CMEM_getNumBlocks()
 */
int CMEM_getBlock(off_t *pphys_base, unsigned long long *psize);

/**
 * @brief Retrieve extended memory block attributes from CMEM driver
 *
 * @param   blockid      Block number
 * @param   pattrs       Pointer to CMEM_BlockAttrs struct
 *
 * @return Success (0) or failure (-1).
 *
 * @remarks Currently this API returns the same values as CMEM_getBlock().
 *
 * @pre Must have called CMEM_init()
 *
 * @sa  CMEM_getBlock()
 * @sa  CMEM_getNumBlocks()
 */
int CMEM_getBlockAttrs(int blockid, CMEM_BlockAttrs *pattrs);

/**
 * @brief Retrieve number of blocks configured into CMEM driver
 *
 * @param   pnblocks     Pointer to storage for holding number of blocks
 *
 * @return Success (0) or failure (-1).
 *
 * @remarks Blocks are configured into CMEM through the Linux 'insmod'
 *          command.
 *
 * @pre Must have called CMEM_init()
 *
 * @sa  CMEM_getBlock()
 * @sa  CMEM_getBlockAttrs()
 */
int CMEM_getNumBlocks(int *pnblocks);

/**
 * @brief Finalize the CMEM module.
 *
 * @return 0 for success or -1 for failure.
 *
 * @remarks After this function has been called, no other CMEM function may be
 *          called (unless CMEM is reinitialized).
 *
 * @pre Must have called CMEM_init()
 *
 * @sa CMEM_init()
 */
int CMEM_exit(void);

/*@}*/

#if defined (__cplusplus)
}
#endif

#endif

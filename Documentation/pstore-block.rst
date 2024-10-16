.. SPDX-License-Identifier: GPL-2.0

Pstore block oops/panic logger
==============================

Introduction
------------

Pstore block (pstore_blk) is an oops/panic logger that write its logs to a block
device before the system crashes. Pstore_blk needs the block device driver
to register a partition of the block device, like /dev/mmcblk0p7 for MMC
driver, and read/write APIs for this partition when on panic.

Pstore block concepts
---------------------

Pstore block begins at function ``blkz_register``, by which a block driver
registers to pstore_blk. Note that the block driver should register to
pstore_blk after block device has registered. The Block driver transfers a
structure ``blkz_info`` which is defined in *linux/pstore_blk.h*.

The following key members of ``struct blkz_info`` may be of interest to you.

blkdev
~~~~~~

The block device to use. Most of the time, it is a partition of block device.
It's ok to keep it as NULL if you are passing ``read`` and ``write`` in
blkz_info as ``blkdev`` is used by blkz_default_general_read/write. If both of
``blkdev``, ``read`` and ``write`` are NULL, no block device is effective and
the data will only be saved in RAM.

It accept the following variants:

1. <hex_major><hex_minor> device number in hexadecimal represents itself; no
   leading 0x, for example b302.
#. /dev/<disk_name> represents the device number of disk
#. /dev/<disk_name><decimal> represents the device number of partition - device
   number of disk plus the partition number
#. /dev/<disk_name>p<decimal> - same as the above; this form is used when disk
   name of partitioned disk ends with a digit.
#. PARTUUID=00112233-4455-6677-8899-AABBCCDDEEFF representing the unique id of
   a partition if the partition table provides it. The UUID may be either an
   EFI/GPT UUID, or refer to an MSDOS partition using the format SSSSSSSS-PP,
   where SSSSSSSS is a zero-filled hex representation of the 32-bit
   "NT disk signature", and PP is a zero-filled hex representation of the
   1-based partition number.
#. PARTUUID=<UUID>/PARTNROFF=<int> to select a partition in relation to a
   partition with a known unique id.
#. <major>:<minor> major and minor number of the device separated by a colon.

See more in section **read/write**.

total_size
~~~~~~~~~~

The total size in bytes of block device used for pstore_blk. It **MUST** be less
than or equal to size of block device if ``blkdev`` valid. It **MUST** be a
multiple of 4096. If ``total_size`` is zero with ``blkdev``, ``total_size`` will be
set to equal to size of ``blkdev``.

The block device area is divided into many chunks, and each event writes a chunk
of information.

dmesg_size
~~~~~~~~~~

The chunk size in bytes for dmesg(oops/panic). It **MUST** be a multiple of
SECTOR_SIZE (Most of the time, the SECTOR_SIZE is 512). If you don't need dmesg,
you can safely to set it to 0.

NOTE that, the remaining space, except ``pmsg_size`` and others, belongs to
dmesg. It means that there are multiple chunks for dmesg.

Pstore_blk will log to dmesg chunks one by one, and always overwrite the oldest
chunk if no free chunk.

pmsg_size
~~~~~~~~~

The chunk size in bytes for pmsg. It **MUST** be a multiple of SECTOR_SIZE (Most
of the time, the SECTOR_SIZE is 512). If you don't need pmsg, you can safely set
it to 0.

There is only one chunk for pmsg.

Pmsg is a user space accessible pstore object. Writes to */dev/pmsg0* are
appended to the chunk. On reboot the contents are available in
/sys/fs/pstore/pmsg-pstore-blk-0.

dump_oops
~~~~~~~~~

Dumping both oopses and panics can be done by setting 1 in the ``dump_oops``
member while setting 0 in that variable dumps only the panics.

read/write
~~~~~~~~~~

They are general ``read/write`` APIs. It is safe and recommended to ignore it,
but set ``blkdev``.

These general APIs are used all the time expect panic. The ``read`` API is
usually used to recover data from block device, and the ``write`` API is usually
to flush new data and erase to block device.

Pstore_blk will temporarily hold all new data before block device is ready. If
you ignore both of ``read/write`` and ``blkdev``, the old data will be lost.

NOTE that the general APIs must check whether the block device is ready if
self-defined.

panic_read/panic_write
~~~~~~~~~~~~~~~~~~~~~~

They are ``read/write`` APIs for panic. They are like the general
``read/write`` but will be used only when on panic.

The attentions for panic read/write see section
**Attentions in panic read/write APIs**.

Register to pstore block
------------------------

Block device driver call ``blkz_register`` to register to Pstore_blk.
For example:

.. code-block:: c

 #include <linux/pstore_blk.h>
 [...]

 static ssize_t XXXX_panic_read(char *buf, size bytes, loff_t pos)
 {
    [...]
 }

 static ssize_t XXXX_panic_write(const char *buf, size_t bytes, loff_t pos)
 {
        [...]
 }

 struct blkz_info XXXX_info = {
        .onwer = THIS_MODULE,
        .name = <...>,
        .dmesg_size = <...>,
        .pmsg_size = <...>,
        .dump_oops = true,
        .panic_read = XXXX_panic_read,
        .panic_write = XXXX_panic_write,
 };

 static int __init XXXX_init(void)
 {
        [... get block device information ...]
        XXXX_info.blkdev = <...>;
        XXXX_info.total_size = <...>;

        [...]
        return blkz_register(&XXXX_info);
 }

There are multiple ways by which you can get block device information.

A. Use the module parameters and kernel cmdline.
B. Use Device Tree bindings.
C. Use Kconfig.
D. Use Driver Feature.
   For example, traverse all MTD devices by ``register_mtd_user``, and get the
   matching name MTD partition.

NOTE that all of the above are done by the block driver rather then pstore_blk.
You can get sample on blkoops.

The attentions for panic read/write see section
**Attentions in panic read/write APIs**.

Compression and header
----------------------

Block device is large enough, it is not necessary to compress dmesg data.
Actually, we recommend not compressing because pstore_blk will insert some
information into the first line of dmesg data if no compression.
For example::

        Panic: Total 16 times

It means that it's the 16th times panic log since the first booting.
Sometimes, the oops|panic counter since burning is very important for embedded
device to judge whether the system is stable.

The following line is inserted by pstore filesystem.
For example::

        Oops#2 Part1

It means that it's the 2nd times oops log on last booting.

Reading the data
----------------

The dump data can be read from the pstore filesystem. The format for these
files is ``dmesg-pstore-blk-[N]`` for dmesg(oops|panic) and
``pmsg-pstore-blk-0`` for pmsg, where N is the record number. To delete a stored
record from block device, simply unlink the respective pstore file. The
timestamp of the dump file records the trigger time.

Attentions in panic read/write APIs
-----------------------------------

If on panic, the kernel is not going to be running for much longer. The tasks
will not be scheduled and the most kernel resources will be out of service. It
looks like a single-threaded program running on a single-core computer.

The following points need special attention for panic read/write APIs:

1. Can **NOT** allocate any memory.
   If you need memory, just allocate while the block driver is initializing
   rather than waiting until the panic.
#. Must be polled, **NOT** interrupt driven.
   No task schedule any more. The block driver should delay to ensure the write
   succeeds, but NOT sleep.
#. Can **NOT** take any lock.
   There is no other task, nor any share resource; you are safe to break all
   locks.
#. Just use CPU to transfer.
   Do not use DMA to transfer unless you are sure that DMA will not keep lock.
#. Operate register directly.
   Try not to use Linux kernel resources. Do I/O map while initializing rather
   than waiting until the panic.
#. Reset your block device and controller if necessary.
   If you are not sure the state of you block device and controller when panic,
   you are safe to stop and reset them.

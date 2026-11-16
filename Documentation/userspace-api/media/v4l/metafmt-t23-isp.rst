.. SPDX-License-Identifier: GPL-2.0

.. _v4l2-meta-fmt-t23-isp-stats:
.. _v4l2-meta-fmt-t23-isp-ext-params:

*******************************************************************************
V4L2_META_FMT_T23_ISP_EXT_PARAMS ('T23E'), V4L2_META_FMT_T23_ISP_STATS ('T23S')
*******************************************************************************

========================
Configuration parameters
========================

Userspace configures the Ingenic T23 ISP per input through the metadata output
node (``T23 params0`` or ``T23 params1``) using the extensible block format
:c:type:`t23_isp_ext_params_cfg` (``V4L2_META_FMT_T23_ISP_EXT_PARAMS``)
defined in ``<linux/media/ingenic/t23-isp-config.h>``.

Each block begins with a :c:type:`t23_isp_ext_params_block_header` containing
the block ``type``, ``flags`` (``T23_ISP_EXT_PARAMS_FL_BLOCK_ENABLE`` or
``T23_ISP_EXT_PARAMS_FL_BLOCK_DISABLE``), and 8-byte aligned ``size``. All
``reserved`` fields in block headers and payloads must be zero. Valid parameter
value ranges are enforced on ``VIDIOC_QBUF``:

- **BLC** (:c:type:`t23_isp_blc_config`): ``r``, ``gr``, ``gb``, ``b`` in
  ``0..4095``.
- **WB** (:c:type:`t23_isp_wb_config`): ``gain_r``, ``gain_b`` in ``0..0x3fff``
  (U2.10 format, ``1024 == 1.0x``).
- **CCM** (:c:type:`t23_isp_ccm_config`): signed Q10 coefficients in
  ``-8192..8191``.
- **Gamma** (:c:type:`t23_isp_gamma_config`): 129 entries in ``0..4095``.
- **LSC** (:c:type:`t23_isp_lsc_config`): ``words <= 2046`` and a multiple of 3,
  ``scale <= 3``, ``mean_en <= 1``.
- **MDNS** (:c:type:`t23_isp_mdns_config`): ``block_size`` a multiple of 4 in
  ``16..255``; ``top[0]`` limited to bits ``0xd1111111``, ``top[1]`` limited to
  bits ``0x00f1310f`` (preventing activation of unallocated history planes), and
  ``top[2] == 0``.

At most one queued parameter buffer is dequeued per active frame immediately
before arming the ISP hardware for that input, and its blocks are merged into
that input's persistent parameter state (unchanged blocks retain their previous
settings until disabled via ``T23_ISP_EXT_PARAMS_FL_BLOCK_DISABLE`` or the
parameter queue stops streaming).

===========================
3A and histogram statistics
===========================

Per-frame AE, histogram, and pre-WB AWB statistics are captured from the
metadata capture node (``T23 stats0`` or ``T23 stats1``) as
:c:type:`t23_isp_stat_buffer` (``V4L2_META_FMT_T23_ISP_STATS``).

- ``sequence`` matches the ``v4l2_buffer.sequence`` of the corresponding video
  capture frame on that input.
- ``ae`` contains a 15x15 grid of 21-bit luma sums and 13-bit pixel class
  counts.
- ``hist[0]`` contains the 256-bin full-frame pre-gamma luma histogram
  (21-bit bin counts, ``0..0x1fffff``); ``hist[1]`` exposes the second 256-bin
  hardware histogram bank collected in the same DMA pass.
- ``awb`` contains a 15x15 grid of 21-bit RGB sums and 13-bit sample counts.

========================
Privacy masks and lines
========================

Each input's ISP subdevice (``t23-isp-core0`` / ``t23-isp-core1``) exposes four
custom controls defined in ``<linux/media/ingenic/t23-isp-config.h>`` under
``V4L2_CID_USER_T23_BASE``:

- ``V4L2_CID_T23_PRIVACY_MASK``: ``4x4`` ``__u16`` matrix of
  ``{left, top, width, height}`` rectangles in sensor coordinates (``0..4095``;
  zero width or height disables the mask). Applied to all scaled outputs.
- ``V4L2_CID_T23_PRIVACY_MASK_COLOR``: ``0xRRGGBB`` integer fill colour.
- ``V4L2_CID_T23_DRAW_LINE``: ``6x5`` ``__u16`` matrix of
  ``{x0, y0, x1, y1, width}`` vectors in sensor coordinates (``0..4095``;
  zero width or equal endpoints disables the line). Drawn on MSCA channel 0.
- ``V4L2_CID_T23_DRAW_LINE_COLOR``: ``0xRRGGBB`` integer stroke colour.

t23-isp uAPI data types
=======================

.. kernel-doc:: include/uapi/linux/media/ingenic/t23-isp-config.h

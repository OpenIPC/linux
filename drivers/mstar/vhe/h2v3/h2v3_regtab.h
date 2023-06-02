#define RO  1
#define WO  2
#define RW  3
#ifndef H2REG
#define H2REG(name,loc,sb,tr,io,comment)    name
#endif
H2REG(HWIF_PRODUCT_ID                                       ,  0,31:16,0,RO,\
    "Product ID                                                            "\
    ),
H2REG(HWIF_MAJOR_NUMBER                                     ,  0,15: 8,0,RO,\
    "Major number                                                          "\
    ),
H2REG(HWIF_MINOR_NUMBER                                     ,  0, 7: 0,0,RO,\
    "Minor number                                                          "\
    ),
H2REG(HWIF_ENC_TIMEOUT_INT                                  ,  1,11:11,0,RW,\
    "enable timeout interrupt when 1                                       "\
    ),
H2REG(HWIF_ENC_SLICE_RDY_STATUS                             ,  1, 8: 8,0,RW,\
    "Interrupt status bit encoder. When this bit is high encoder has encode"\
    "d a slice.                                                            "\
    ),
H2REG(HWIF_ENC_TIMEOUT                                      ,  1, 6: 6,0,RW,\
    "Interrupt status bit encoder timeout. When high the encoder has been i"\
    "dling for too long. Possible only if timeout interrupt is enabled     "\
    ),
H2REG(HWIF_ENC_BUFFER_FULL                                  ,  1, 5: 5,1,RW,\
    "IRQ buffer full status bit. bufferFullInterrupt                       "\
    ),
H2REG(HWIF_ENC_SW_RESET                                     ,  1, 4: 4,0,RW,\
    "IRQ SW reset status bit.                                              "\
    ),
H2REG(HWIF_ENC_BUS_ERROR_STATUS                             ,  1, 3: 3,0,RW,\
    "Interrupt status bit bus. Error response from bus.                    "\
    ),
H2REG(HWIF_ENC_FRAME_RDY_STATUS                             ,  1, 2: 2,0,RW,\
    "Interrupt status bit encoder. When this bit is high encoder has encode"\
    "d a picture.                                                          "\
    ),
H2REG(HWIF_ENC_IRQ_DIS                                      ,  1, 1: 1,0,RW,\
    "Encoder IRQ disable. When high  there are no interrupts concerning enc"\
    "oder from HW. Polling must be used to see the interrupt statuses.     "\
    ),
H2REG(HWIF_ENC_IRQ                                          ,  1, 0: 0,0,RW,\
    "Encoder IRQ. When high  encoder requests an interrupt. SW will reset t"\
    "his after interrupt is handled.                                       "\
    ),
H2REG(HWIF_ENC_AXI_WRITE_ID                                 ,  2,31:24,0,RW,\
    "AXI Write ID                                                          "\
    ),
H2REG(HWIF_ENC_AXI_READ_ID                                  ,  2,23:16,0,RW,\
    "AXI Read ID                                                           "\
    ),
H2REG(HWIF_ENC_STREAM_SWAP                                  ,  2,15:12,0,RW,\
    "byte swap config for output stream data.4 Bit byte order vector to con"\
    "trol byte locations inside HW internal 128 bit data vector. For 64 and"\
    " 32 bit external bus widths the data is forst gathered to 128 bit widt"\
    "h and then bytes swapped accordingly                                  "\
    ),
H2REG(HWIF_ENC_PIX_IN_SWAP                                  ,  2,11: 8,0,RW,\
    "Byte swap configuration for picture data (encoder input)              "\
    ),
H2REG(HWIF_ENC_ROI_DQ_SWAP                                  ,  2, 7: 4,0,RW,\
    "Byte swap configuration for qp delta of ROI map                       "\
    ),
H2REG(HWIF_ENC_CTB_RC_SWAP                                  ,  2, 3: 0,0,RW,\
    "Byte swap configuration for ctb rate control memory out               "\
    ),
H2REG(HWIF_ENC_SCALER_SWAP                                  , 45,31:28,0,RW,\
    "Byte swap configuration for scaledout data (scaledout)                "\
    ),
H2REG(HWIF_ENC_CHROMA_SWAP                                  , 45,27:27,1,RW,\
    "Swap order of chroma bytes in semiplanar input format.                "\
    ),
H2REG(HWIF_ENC_NALUSZ_SWAP                                  , 45, 6: 3,0,RW,\
    "Byte swap configuration for Nal Unit size output                      "\
    ),
H2REG(HWIF_ENC_BURST_DISABLE                                ,  3,31:31,0,RW,\
    "Disable burst mode for AXI                                            "\
    ),
H2REG(HWIF_ENC_BURST_INCR                                   ,  3,30:30,0,RW,\
    "Burst incremental. 1=INCR burst allowed. 0=use SINGLE burst           "\
    ),
H2REG(HWIF_ENC_DATA_DISCARD                                 ,  3,29:29,0,RW,\
    "Enable burst data discard. 2 or 3 long reads are using BURST4         "\
    ),
H2REG(HWIF_ENC_CLOCK_GATING                                 ,  3,28:28,0,RW,\
    "Enable clock gating                                                   "\
    ),
H2REG(HWIF_ENC_INPUT_READ_CHUNK                             ,  3,27:27,0,RW,\
    "Chunk size for input picture read 0=4 MBs. 1=1 MB                     "\
    ),
H2REG(HWIF_ENC_AXI_DUAL_CH                                  ,  3,26:26,0,RW,\
    "Disable dual channel AXI. 0=use two channels. 1=use single channel.   "\
    ),
H2REG(HWIF_ENC_AXI_WR_ID_E                                  ,  3,25:25,0,RW,\
    "axi write enable 0=disable. 1=enable.                                 "\
    ),
H2REG(HWIF_ENC_AXI_RD_ID_E                                  ,  3,24:24,0,RW,\
    "axi read enable 0=disable. 1=enable.                                  "\
    ),
H2REG(HWIF_ENC_SLICE_INT                                    ,  3, 3: 3,1,RW,\
    "enable slice interrupt when 1                                         "\
    ),
H2REG(HWIF_ENC_MODE                                         ,  4,31:29,1,RW,\
    "Encoding mode. streamType.1= hevc                                     "\
    ),
H2REG(HWIF_ENC_MIN_CB_SIZE                                  ,  4,26:25,0,RO,\
    "min cb size:0-8x8:1-16x16:2-32x32:3-64x64:we only support 8x8         "\
    ),
H2REG(HWIF_ENC_MAX_CB_SIZE                                  ,  4,24:23,0,RO,\
    "max cb size:0-8x8:1-16x16:2-32x32:3-64x64:we only support 64x64       "\
    ),
H2REG(HWIF_ENC_MIN_TRB_SIZE                                 ,  4,22:21,0,RO,\
    "min tr block size:0-4x4:1-8x8:2-16x16:3-32x32:we only support 4x4     "\
    ),
H2REG(HWIF_ENC_MAX_TRB_SIZE                                 ,  4,20:19,0,RO,\
    "max tr block size:0-4x4:1-8x8:2-16x16:3-32x32:we only support 16x16   "\
    ),
H2REG(HWIF_ENC_OUTBS_MODE                                   ,  4,18:18,1,RW,\
    "output stream mode:0-byte stream:1-Nal stream                         "\
    ),
H2REG(HWIF_ENC_CHROMA_QP_OFFSET                             ,  4,17:13,0,RW,\
    "chroma qp offset[-12~12]                                              "\
    ),
H2REG(HWIF_ENC_STRONG_INTRA_SMOOTHING_ENABLED_FLAG          ,  4,12:12,0,RW,\
    "strong intra smoothing enabled flag                                   "\
    ),
H2REG(HWIF_ENC_CONSTRAINED_INTRA_PRED_FLAG                  ,  4,11:11,0,RW,\
    "constrained intra pred flag                                           "\
    ),
H2REG(HWIF_ENC_SCALING_LIST_ENABLED_FLAG                    ,  4, 8: 8,0,RW,\
    "scaling_list_enabled_flag                                             "\
    ),
H2REG(HWIF_ENC_ACTIVE_OVERRIDE_FLAG                         ,  4, 7: 7,0,RW,\
    "active override flag                                                  "\
    ),
H2REG(HWIF_ENC_SAO_ENABLE                                   ,  4, 6: 6,0,RW,\
    "SAO enable                                                            "\
    ),
H2REG(HWIF_ENC_MAX_TRANS_HIERARCHY_DEPTH_INTRA              ,  4, 5: 3,0,RO,\
    "max transform hierarchy depth of intra                                "\
    ),
H2REG(HWIF_ENC_MAX_TRANS_HIERARCHY_DEPTH_INTER              ,  4, 2: 0,0,RO,\
    "max transform hierarchy depth of inter                                "\
    ),
H2REG(HWIF_ENC_PIC_WIDTH                                    ,  5,31:22,1,RW,\
    "Encoded width. lumWidth(unit 8 pixels)                                "\
    ),
H2REG(HWIF_ENC_PIC_HEIGHT                                   ,  5,21:11,1,RW,\
    "Encoded height. lumHeight(unit 8 pixels):Max width x height is 4096 x "\
    "4096                                                                  "\
    ),
H2REG(HWIF_ENC_PPS_DEBLOCKING_FILTER_OVERRIDE_ENABLED_FLAG  ,  5, 9: 9,0,RW,\
    "deblocking filter override enable flag.0:disable 1:enable             "\
    ),
H2REG(HWIF_ENC_SLICE_DEBLOCKING_FILTER_OVERRIDE_FLAG        ,  5, 8: 8,0,RW,\
    "slice deblocking filter override flag.0:no 1:yes                      "\
    ),
H2REG(HWIF_ENC_BUFFER_FULL_CONTINUE                         ,  5, 5: 5,0,RW,\
    "continue to encode the same frame after buffer full.0:not continue 1:c"\
    "ontinue                                                               "\
    ),
H2REG(HWIF_ENC_REF_FRAMES                                   ,  5, 4: 3,0,RW,\
    "HEVC: num_ref_frames  maximum number of short and long term reference "\
    "frames in encoded picture buffer                                      "\
    ),
H2REG(HWIF_ENC_FRAME_CODING_TYPE                            ,  5, 2: 1,1,RW,\
    "Encoded picture type. frameType.:1-I:0-P:2:B                          "\
    ),
H2REG(HWIF_ENC_E                                            ,  5, 0: 0,0,RW,\
    "encoder enable. Setting this bit high will start the encoding operatio"\
    "n. HW will reset this when picture is processed or bus error or timeou"\
    "t interrupt is given.                                                 "\
    ),
H2REG(HWIF_ENC_SLICE_SIZE                                   ,  6,31:25,1,RW,\
    "slice size in ctu row                                                 "\
    ),
H2REG(HWIF_ENC_NUM_SHORT_TERM_REF_PIC_SETS_V2               ,  6,24:20,0,RW,\
    "number of short term reference picture sets                           "\
    ),
H2REG(HWIF_ENC_NUM_NEGATIVE_PICS                            ,  6,19:18,0,RW,\
    "number of negative pictures                                           "\
    ),
H2REG(HWIF_ENC_NUM_POSITIVE_PICS                            ,  6,17:16,0,RW,\
    "number of positive pictures                                           "\
    ),
H2REG(HWIF_ENC_DEBLOCKING_FILTER_CTRL                       ,  6,15:15,0,RW,\
    "deblocking filter control.De-block filtering disable: 1 = filtering is"\
    " disabled for current picture. 0 = filtering is enabled for current pi"\
    "cture                                                                 "\
    ),
H2REG(HWIF_ENC_DEBLOCKING_TC_OFFSET                         ,  6,14:11,0,RW,\
    "deblocking tc offset:-6~6                                             "\
    ),
H2REG(HWIF_ENC_DEBLOCKING_BETA_OFFSET                       ,  6,10: 7,0,RW,\
    "deblocking beta offset:-6~6                                           "\
    ),
H2REG(HWIF_ENC_RPS_ID_V2                                    ,  6, 6: 2,0,RW,\
    "rps Id:used for slice header                                          "\
    ),
H2REG(HWIF_ENC_NAL_SIZE_WRITE                               ,  6, 1: 1,1,RW,\
    "Enable writing size of each NAL unit to BaseControl                   "\
    ),
H2REG(HWIF_ENC_CU_QP_DELTA_ENABLED                          ,  6, 0: 0,0,RW,\
    "cu qp delta encoding is enabled:used for ROI                          "\
    ),
H2REG(HWIF_ENC_PIC_INIT_QP                                  ,  7,31:26,1,RW,\
    "picture header qp                                                     "\
    ),
H2REG(HWIF_ENC_CABAC_INIT_FLAG                              ,  7,25:25,0,RW,\
    "cabac init flag                                                       "\
    ),
H2REG(HWIF_ENC_NUM_SLICES_READY                             ,  7,24:17,0,RW,\
    "HEVC amount of completed slices.                                      "\
    ),
H2REG(HWIF_ENC_DIFF_CU_QP_DELTA_DEPTH                       ,  7,15:14,0,RW,\
    "difference of cu qp delta depth                                       "\
    ),
H2REG(HWIF_ENC_PIC_QP                                       ,  7,13: 8,1,RW,\
    "qp of current picture                                                 "\
    ),
H2REG(HWIF_ENC_ROI1_DQP                                     ,  7, 7: 4,0,RW,\
    "ROI1 delta qp                                                         "\
    ),
H2REG(HWIF_ENC_ROI2_DQP                                     ,  7, 3: 0,0,RW,\
    "ROI2 delta qp                                                         "\
    ),
H2REG(HWIF_ENC_OUTBS_BUF_LIMIT                              ,  9,31: 0,1,RW,\
    "Stream buffer limit (64bit addresses) / output stream size (bytes). HW"\
    "StreamDataCount. If limit is reached buffer full IRQ is given.        "\
    ),
H2REG(HWIF_ENC_POC                                          , 11,31: 0,0,RW,\
    "encoded Picture order count                                           "\
    ),
H2REG(HWIF_ENC_L0_DELTA_POC0                                , 17,31:22,0,RW,\
    "delta poc of list0 pic0. For H2V2 or later version.                   "\
    ),
H2REG(HWIF_ENC_L0_USED_BY_CURR_PIC0                         , 17,20:20,0,RW,\
    "list0 pic0 used by current. For H2V2 or later version.                "\
    ),
H2REG(HWIF_ENC_L0_DELTA_POC1                                , 17,19:10,0,RW,\
    "delta poc of list0 pic1. For H2V2 or later version.                   "\
    ),
H2REG(HWIF_ENC_L0_USED_BY_CURR_PIC1                         , 17, 8: 8,0,RW,\
    "list0 pic1 used by current. For H2V2 or later version.                "\
    ),
H2REG(HWIF_ENC_ACTIVE_L0_CNT                                , 17, 7: 6,0,RW,\
    "active l0 count                                                       "\
    ),
H2REG(HWIF_ENC_L1_DELTA_POC0                                , 91,31:22,0,RW,\
    "delta poc of list1 pic0                                               "\
    ),
H2REG(HWIF_ENC_L1_USED_BY_CURR_PIC0                         , 91,20:20,0,RW,\
    "list1 pic0 used by current                                            "\
    ),
H2REG(HWIF_ENC_L1_DELTA_POC1                                , 91,19:10,0,RW,\
    "delta poc of list1 pic1                                               "\
    ),
H2REG(HWIF_ENC_L1_USED_BY_CURR_PIC1                         , 91, 8: 8,0,RW,\
    "list1 pic1 used by current                                            "\
    ),
H2REG(HWIF_ENC_ACTIVE_L1_CNT                                , 91, 7: 6,0,RW,\
    "active l0 count                                                       "\
    ),
H2REG(HWIF_ENC_INPUT_FORMAT                                 , 38,31:28,1,RW,\
    "Input image format. inputFormat. YUV420P/YUV420SP/YUYV422/UYVY422/RGB5"\
    "65/RGB555/RGB444/RGB888/RGB101010/I010/P010/PACKED10BITPLANAR/Y0L2    "\
    ),
H2REG(HWIF_ENC_INPUT_ROTATION                               , 38,27:26,1,RW,\
    "Input image rotation. 0=disabled. 1=90 degrees right. 2=90 degrees lef"\
    "t.3=180 degree right.                                                 "\
    ),
H2REG(HWIF_ENC_OUTPUT_BITWIDTH_Y                            , 38,25:24,0,RW,\
    "luma output bitwidth 0=8 bit. 1=9 bit. 2=10 bit.                      "\
    ),
H2REG(HWIF_ENC_OUTPUT_BITWIDTH_C                            , 36, 1: 0,0,RW,\
    "chroma output bitwidth 0=8 bit. 1=9 bit. 2=10 bit.                    "\
    ),
H2REG(HWIF_ENC_YOFFSET                                      , 38,23:20,0,RW,\
    "Input luminance offset (bytes) [0..15]                                "\
    ),
H2REG(HWIF_ENC_COFFSET                                      , 37, 3: 0,0,RW,\
    "Input chrominance offset (bytes) [0..15]                              "\
    ),
H2REG(HWIF_ENC_ROWLENGTH                                    , 38,19: 6,1,RW,\
    "Input luminance row length. lumWidthSrc (pixels) [96..8192]           "\
    ),
H2REG(HWIF_ENC_XFILL                                        , 38, 5: 4,0,RW,\
    "Overfill pixels on right edge of image div2 [0.1.2.3]                 "\
    ),
H2REG(HWIF_ENC_YFILL                                        , 38, 3: 1,1,RW,\
    "Overfill pixels on bottom edge of image. YFill. [0...7]               "\
    ),
H2REG(HWIF_ENC_RGBCOEFFA                                    , 39,31:16,0,RW,\
    "RGB to YUV conversion coefficient A                                   "\
    ),
H2REG(HWIF_ENC_RGBCOEFFB                                    , 39,15: 0,0,RW,\
    "RGB to YUV conversion coefficient B                                   "\
    ),
H2REG(HWIF_ENC_RGBCOEFFC                                    , 40,31:16,0,RW,\
    "RGB to YUV conversion coefficient C                                   "\
    ),
H2REG(HWIF_ENC_RGBCOEFFE                                    , 40,15: 0,0,RW,\
    "RGB to YUV conversion coefficient D                                   "\
    ),
H2REG(HWIF_ENC_RGBCOEFFF                                    , 41,31:16,0,RW,\
    "RGB to YUV conversion coefficient E                                   "\
    ),
H2REG(HWIF_ENC_RMASKMSB                                     , 41,15:11,0,RW,\
    "RGB R-component mask MSB bit position [0..31]                         "\
    ),
H2REG(HWIF_ENC_GMASKMSB                                     , 41,10: 6,0,RW,\
    "RGB G-component mask MSB bit position [0..31]                         "\
    ),
H2REG(HWIF_ENC_BMASKMSB                                     , 41, 5: 1,0,RW,\
    "RGB B-component mask MSB bit position [0..31]                         "\
    ),
H2REG(HWIF_ENC_SCALE_OUTW                                   , 43,31:19,1,RW,\
    "Scaling width of down-scaled image. ScaledWidth. [96..4076]           "\
    ),
H2REG(HWIF_ENC_SCALE_OUTW_RATIO                             , 43,18: 3,0,RW,\
    "Scaling ratio for width of down-scaled image. Fixed point integer 1.16"\
    ".                                                                     "\
    ),
H2REG(HWIF_ENC_SCALE_MODE                                   , 43, 1: 0,1,RW,\
    "Scaling mode. ScalingMode. 0=disabled. 1=scaling only. 2=scale+encode "\
    ),
H2REG(HWIF_ENC_SCALE_OUTH                                   , 44,31:18,1,RW,\
    "Scaling width of down-scaled image. ScaledHeight. [2..4078]           "\
    ),
H2REG(HWIF_ENC_SCALE_OUTH_RATIO                             , 44,17: 2,0,RW,\
    "Scaling ratio for height of down-scaled image. Fixed point integer 1.1"\
    "6.                                                                    "\
    ),
H2REG(HWIF_ENC_ENCODED_CTB_NUMBER                           , 45,26:14,0,RW,\
    "MB count output. max 64*64                                            "\
    ),
H2REG(HWIF_ENC_SCALE_SKIP_LFT_PIX_COL                       , 45,13:12,0,RW,\
    "skip left pixel column                                                "\
    ),
H2REG(HWIF_ENC_SCALE_SKIP_TOP_PIX_ROW                       , 45,11:10,0,RW,\
    "skip top pixel row                                                    "\
    ),
H2REG(HWIF_ENC_VSCALE_WEIGHT_EN                             , 45, 9: 9,0,RW,\
    "vertical scale weight enable.                                         "\
    ),
H2REG(HWIF_ENC_SCALE_HOR_COPY                               , 45, 8: 8,0,RW,\
    "horizontal data copy directly                                         "\
    ),
H2REG(HWIF_ENC_SCALE_VER_COPY                               , 45, 7: 7,0,RW,\
    "vertical data copy directly                                           "\
    ),
H2REG(HWIF_ENC_HWSCALINGSUPPORT                             , 80,30:30,0,RO,\
    "Down-scaling supported by HW. 0=not supported. 1=supported            "\
    ),
H2REG(HWIF_ENC_HWBFRAMESUPPORT                              , 80,29:29,0,RO,\
    "HW bframe support. 0=not support bframe. 1=support bframe             "\
    ),
H2REG(HWIF_ENC_HWRGBSUPPORT                                 , 80,28:28,0,RO,\
    "RGB to YUV conversion supported by HW. 0=not supported. 1=supported   "\
    ),
H2REG(HWIF_ENC_HWHEVCSUPPORT                                , 80,27:27,0,RO,\
    "HEVC encoding supported by HW. 0=not supported. 1=supported           "\
    ),
H2REG(HWIF_ENC_HWVP9SUPPORT                                 , 80,26:26,0,RO,\
    "VP9 encoding supported by HW. 0=not supported. 1=supported            "\
    ),
H2REG(HWIF_ENC_HWDENOISESUPPORT                             , 80,25:25,0,RO,\
    "denoise supported by HW. 0=not supported. 1=supported                 "\
    ),
H2REG(HWIF_ENC_HWMAIN10SUPPORT                              , 80,24:24,0,RO,\
    "main10 supported by HW. 0=main8 supported. 1=main10 supported         "\
    ),
H2REG(HWIF_ENC_HWBUS                                        , 80,23:21,0,RO,\
    "Bus connection of HW. 1=AHB. 2=OCP. 3=AXI. 4=PCI. 5=AXIAHB. 6=AXIAPB. "\
    ),
H2REG(HWIF_ENC_HWSYNTHESISLAN                               , 80,18:17,0,RO,\
    "Synthesis language. 1=vhdl. 2=verilog                                 "\
    ),
H2REG(HWIF_ENC_HWBUSWIDTH                                   , 80,14:13,0,RO,\
    "Bus width of HW. 0=32b. 1=64b. 2=128b                                 "\
    ),
H2REG(HWIF_ENC_HWMAXVIDEOWIDTH                              , 80,12: 0,0,RO,\
    "Maximum video width supported by HW (pixels)                          "\
    ),
H2REG(HWIF_ENC_MAX_BURST                                    , 81,31:24,0,RW,\
    "for support AXI4.0 burst length is programmable Default value:0x20    "\
    ),
H2REG(HWIF_TIMEOUT_OVERRIDE_E                               , 81,23:23,0,RW,\
    "enable signal if timeout period is controlled by software Default valu"\
    "e: 0x0                                                                "\
    ),
H2REG(HWIF_TIMEOUT_CYCLES                                   , 81,22: 0,0,RW,\
    "timeout cycles number default value: 0x0                              "\
    ),
H2REG(HWIF_ENC_HW_PERFORMANCE                               , 82,31: 0,0,RO,\
    "record hardware  performance(cycles) of current picture               "\
    ),
H2REG(HWIF_ENC_LISTS_MODI_PRESENT_FLAG                      ,104,31:31,0,RW,\
    "lists_modification_present_flag from pps                              "\
    ),
H2REG(HWIF_ENC_LIST_ENTRY_L1_PIC1                           ,104,24:21,0,RW,\
    "list1 picture1 index                                                  "\
    ),
H2REG(HWIF_ENC_LIST_ENTRY_L1_PIC0                           ,104,20:17,0,RW,\
    "list1 picture0 index                                                  "\
    ),
H2REG(HWIF_ENC_REF_PIC_LIST_MODI_FLAG_L1                    ,104,16:16,0,RW,\
    "reference picture list1 modification flag                             "\
    ),
H2REG(HWIF_ENC_LIST_ENTRY_L0_PIC1                           ,104, 8: 5,0,RW,\
    "list0 picture1 index                                                  "\
    ),
H2REG(HWIF_ENC_LIST_ENTRY_L0_PIC0                           ,104, 4: 1,0,RW,\
    "list0 picture0 index                                                  "\
    ),
H2REG(HWIF_ENC_REF_PIC_LIST_MODI_FLAG_L0                    ,104, 0: 0,0,RW,\
    "reference picture list0 modification flag                             "\
    ),
H2REG(HWIF_ENC_TARGETPICSIZE                                ,105,31: 0,0,RW,\
    "Frame level target picture size.                                      "\
    ),
H2REG(HWIF_ENC_MINPICSIZE                                   ,106,31: 0,0,RW,\
    "Frame level Min picture size.                                         "\
    ),
H2REG(HWIF_ENC_MAXPICSIZE                                   ,107,31: 0,0,RW,\
    "Frame level Max picture size.                                         "\
    ),
H2REG(HWIF_ENC_AVERAGEQP                                    ,108,31:24,0,RW,\
    "average QP                                                            "\
    ),
H2REG(HWIF_ENC_NONZEROCOUNT                                 ,108,23: 0,0,RW,\
    "non zero coefficient count div4 output.                               "\
    ),
H2REG(HWIF_ENC_INTRACU8NUM                                  ,111,31:12,0,RW,\
    "intra cu8 number                                                      "\
    ),
H2REG(HWIF_ENC_SKIPCU8NUM                                   ,112,31:12,0,RW,\
    "skip cu8 number                                                       "\
    ),
H2REG(HWIF_ENC_PBFRAME4NRDCOST                              ,113,31: 0,0,RW,\
    "PBFrame4NRdCost                                                       "\
    ),
H2REG(HWIF_ENC_CTBRCTHRDMIN                                 ,118,31:16,0,RW,\
    "ctb rate control threshold min                                        "\
    ),
H2REG(HWIF_ENC_CTBRCTHRDMAX                                 ,118,15: 0,0,RW,\
    "ctb rate control threshold max                                        "\
    ),
H2REG(HWIF_ENC_CTBBITSMIN                                   ,119,31:16,0,RW,\
    "minimum lcu bits number of last picture                               "\
    ),
H2REG(HWIF_ENC_CTBBITSMAX                                   ,119,15: 0,0,RW,\
    "maximum lcu bits number of last picture                               "\
    ),
H2REG(HWIF_ENC_TOTALLCUBITS                                 ,120,31: 0,0,RW,\
    "total bits number of all lcus of last picture not including slice head"\
    "er bits                                                               "\
    ),
H2REG(HWIF_ENC_BITSRATIO                                    ,121,31: 0,0,RW,\
    "128* TargetCurPicSize/ TargetLastPicSize for lcu target bits calculati"\
    "on                                                                    "\
    ),
H2REG(HWIF_ENC_SSE_DIV_256                                  ,133,31: 0,0,RW,\
    "sse divide 256                                                        "\
    ),
H2REG(HWIF_ENC_NOISE_REDUCTION_ENABLE                       ,134,31:30,0,RW,\
    " enable/disable the de-noise function                                 "\
    ),
H2REG(HWIF_ENC_NOISE_LOW                                    ,134,29:24,0,RW,\
    " the low boundary for noise estimation                                "\
    ),
H2REG(HWIF_ENC_NR_MBNUM_INVERT_REG                          ,134,15: 0,0,RW,\
    "inverter of luma16 num *N                                             "\
    ),
H2REG(HWIF_ENC_SLICEQP_PREV                                 ,135,31:26,0,RW,\
    "previous frame slice QP                                               "\
    ),
H2REG(HWIF_ENC_THRESH_SIGMA_CUR                             ,135,25: 5,0,RW,\
    "sigma threshold for current frame                                     "\
    ),
H2REG(HWIF_ENC_SIGMA_CUR                                    ,136,31:16,0,RW,\
    "noise sigma for current frame                                         "\
    ),
H2REG(HWIF_ENC_FRAME_SIGMA_CALCED                           ,136,15: 0,0,RW,\
    "calculated sigma of coding frame                                      "\
    ),
H2REG(HWIF_ENC_THRESH_SIGMA_CALCED                          ,137,31:11,0,RW,\
    "noise sigma for current frame                                         "\
    ),
H2REG(HWIF_ENC_QP_FRAC                                      ,158,31:16,0,RW,\
    "qp fractional part                                                    "\
    ),
H2REG(HWIF_ENC_QP_DELTA_GAIN                                ,158,15: 0,0,RW,\
    "qp delta gain                                                         "\
    ),
H2REG(HWIF_ENC_QP_SUM                                       ,159,31: 8,0,RW,\
    "qp sum                                                                "\
    ),
H2REG(HWIF_ENC_QP_NUM                                       ,160,30:14,0,RW,\
    "qp num                                                                "\
    ),
H2REG(HWIF_ENC_PIC_COMPLEXITY                               ,161,31: 9,0,RW,\
    "picture complexity                                                    "\
    ),
H2REG(HWIF_ENC_RC_BLOCK_SIZE                                ,162,31:30,0,RW,\
    "block rc block size 0=64x64. 1=32x32. 2=16x16                         "\
    ),
H2REG(HWIF_ENC_RC_SLICE_QPOFFSET                            ,162,29:24,0,RW,\
    "block rc slice qp offset -30~+30                                      "\
    ),
H2REG(HWIF_ENC_RCDQP_RANGE                                  ,162,21:18,0,RW,\
    "rc qp delta range                                                     "\
    ),
H2REG(HWIF_ENC_COMPLEXITY_OFFSET                            ,162,11: 7,0,RW,\
    "block rc complexity offset                                            "\
    ),
H2REG(HWIF_ENC_RC_CTBRC_SLICEQPOFFSET                       ,162, 6: 1,0,RW,\
    "slice Qp offset for ROI                                               "\
    ),
H2REG(HWIF_ENC_ROI1_DQP_RC                                  ,163,31:27,0,RW,\
    "ROI1 delta qp with rc                                                 "\
    ),
H2REG(HWIF_ENC_ROI2_DQP_RC                                  ,163,26:22,0,RW,\
    "ROI2 delta qp with rc                                                 "\
    ),
H2REG(HWIF_ENC_QP_MAX                                       ,163,21:16,0,RW,\
    "max value of qp                                                       "\
    ),
H2REG(HWIF_ENC_QP_MIN                                       ,163,15:10,0,RW,\
    "min value of qp                                                       "\
    ),
/* compressor enable/disable */
H2REG(HWIF_ENC_RECON_COMPR_Y_EN                             , 17, 5: 5,1,RW,\
    "recon frame luma compressor enable flag 0=disable 1=enable.           "\
    ),
H2REG(HWIF_ENC_RECON_COMPR_C_EN                             , 17, 4: 4,1,RW,\
    "recon frame chroma compressor enable flag 0=disable 1=enable.         "\
    ),
H2REG(HWIF_ENC_L0RP0_COMPR_Y_EN                             , 17, 3: 3,1,RW,\
    "list0 ref0 frame luma compressor enable flag 0=disable 1=enable.      "\
    ),
H2REG(HWIF_ENC_L0RP0_COMPR_C_EN                             , 17, 2: 2,1,RW,\
    "list0 ref0 frame chroma compressor enable flag 0=disable 1=enable.    "\
    ),
H2REG(HWIF_ENC_L0RP1_COMPR_Y_EN                             , 17, 1: 1,0,RW,\
    "list0 ref1 frame luma compressor enable flag 0=disable 1=enable.      "\
    ),
H2REG(HWIF_ENC_L0RP1_COMPR_C_EN                             , 17, 0: 0,0,RW,\
    "list0 ref1 frame chroma compressor enable flag 0=disable 1=enable.    "\
    ),
H2REG(HWIF_ENC_L1RP0_COMPR_Y_EN                             , 91, 3: 3,1,RW,\
    "list1 ref0 frame luma compressor enable flag 0=disable 1=enable.      "\
    ),
H2REG(HWIF_ENC_L1RP0_COMPR_C_EN                             , 91, 2: 2,1,RW,\
    "list1 ref0 frame chroma compressor enable flag 0=disable 1=enable.    "\
    ),
H2REG(HWIF_ENC_L1RP1_COMPR_Y_EN                             , 91, 1: 1,0,RW,\
    "list1 ref1 frame luma compressor enable flag 0=disable 1=enable.      "\
    ),
H2REG(HWIF_ENC_L1RP1_COMPR_C_EN                             , 91, 0: 0,0,RW,\
    "list1 ref1 frame chroma compressor enable flag 0=disable 1=enable.    "\
    ),
/* intra block cycles */
H2REG(HWIF_ENC_CIR_START                                    , 22,31:18,0,RW,\
    "cir start                                                             "\
    ),
H2REG(HWIF_ENC_CIR_PITCH                                    , 22,17: 4,0,RW,\
    "Cyclic Intra Refresh                                                  "\
    ),
/* roi/ctbrc enable/disable */
H2REG(HWIF_ENC_RCROI_ENABLE                                 , 22, 2: 0,1,RW,\
    " bit2 : rc enable or disable.                                         "\
    " bit1 : roi map enable or disable.                                    "\
    " bit0 : roi area enable or disable.                                   "\
    ),
/* intra area */
H2REG(HWIF_ENC_INTRA_AREA_LFT                               , 23,31:24,0,RW,\
    "intra Area Left                                                       "\
    ),
H2REG(HWIF_ENC_INTRA_AREA_RHT                               , 23,23:16,0,RW,\
    "intra Area Right                                                      "\
    ),
H2REG(HWIF_ENC_INTRA_AREA_TOP                               , 23,15: 8,0,RW,\
    "intra Area Top                                                        "\
    ),
H2REG(HWIF_ENC_INTRA_AREA_BOT                               , 23, 7: 0,0,RW,\
    "intra Area Bottom                                                     "\
    ),
/* roi area */
H2REG(HWIF_ENC_ROI1_LFT                                     , 24,31:24,0,RW,\
    "ROI1 Area Left                                                        "\
    ),
H2REG(HWIF_ENC_ROI1_RHT                                     , 24,23:16,0,RW,\
    "ROI1 Area Right                                                       "\
    ),
H2REG(HWIF_ENC_ROI1_TOP                                     , 24,15: 8,0,RW,\
    "ROI1 Area Top                                                         "\
    ),
H2REG(HWIF_ENC_ROI1_BOT                                     , 24, 7: 0,0,RW,\
    "ROI1 Area Bottom                                                      "\
    ),
H2REG(HWIF_ENC_ROI2_LFT                                     , 25,31:24,0,RW,\
    "ROI2 Area Left                                                        "\
    ),
H2REG(HWIF_ENC_ROI2_RHT                                     , 25,23:16,0,RW,\
    "ROI2 Area Right                                                       "\
    ),
H2REG(HWIF_ENC_ROI2_TOP                                     , 25,15: 8,0,RW,\
    "ROI2 Area Top                                                         "\
    ),
H2REG(HWIF_ENC_ROI2_BOT                                     , 25, 7: 0,0,RW,\
    "ROI2 Area Bottom                                                      "\
    ),
/* factors */
H2REG(HWIF_ENC_INTRA_SIZE_FACTOR_0                          , 26,31:22,0,RW,\
    "intra size factor 0                                                   "\
    ),
H2REG(HWIF_ENC_INTRA_SIZE_FACTOR_1                          , 26,21:12,0,RW,\
    "intra size factor 1                                                   "\
    ),
H2REG(HWIF_ENC_INTRA_SIZE_FACTOR_2                          , 26,11: 2,0,RW,\
    "intra size factor 2                                                   "\
    ),
H2REG(HWIF_ENC_INTRA_SIZE_FACTOR_3                          , 27,31:22,0,RW,\
    "intra size factor 3                                                   "\
    ),
H2REG(HWIF_ENC_INTRA_MODE_FACTOR_0                          , 27,21:17,0,RW,\
    "intra mode factor 0                                                   "\
    ),
H2REG(HWIF_ENC_INTRA_MODE_FACTOR_1                          , 27,16:11,0,RW,\
    "intra mode factor 1                                                   "\
    ),
H2REG(HWIF_ENC_INTRA_MODE_FACTOR_2                          , 27,10: 4,0,RW,\
    "intra mode factor 2                                                   "\
    ),
/* penalties */
H2REG(HWIF_ENC_BITS_EST_TU_SPLIT_PENALTY                    , 35,17:15,0,RW,\
    "bits estimation for tu split penalty                                  "\
    ),
H2REG(HWIF_ENC_BITS_EST_BIAS_INTRA_CU_8                     , 35,14: 8,0,RW,\
    "bits estimation bias for intra cu 8                                   "\
    ),
H2REG(HWIF_ENC_BITS_EST_BIAS_INTRA_CU_16                    , 35, 7: 0,0,RW,\
    "bits estimation bias for intra cu 16                                  "\
    ),
H2REG(HWIF_ENC_BITS_EST_BIAS_INTRA_CU_32                    , 36,31:23,0,RW,\
    "bits estimation bias for intra cu 32                                  "\
    ),
H2REG(HWIF_ENC_BITS_EST_BIAS_INTRA_CU_64                    , 36,22:13,0,RW,\
    "bits estimation bias for intra cu 64                                  "\
    ),
H2REG(HWIF_ENC_INTER_SKIP_BIAS                              , 36,12: 6,0,RW,\
    "inter skip bias                                                       "\
    ),
H2REG(HWIF_ENC_BITS_EST_1N_CU_PENALTY                       , 36, 5: 2,0,RW,\
    "bits estimation 1N cu penalty                                         "\
    ),
/* nalunit type */
H2REG(HWIF_ENC_NAL_UNIT_TYPE                                ,164,31:26,0,RW,\
    "NAL unit type                                                         "\
    ),
H2REG(HWIF_ENC_NUH_TEMPORAL_ID                              ,164,25:23,0,RW,\
    "NAL temporal id                                                       "\
    ),
/* base address */
H2REG(HWIF_ENC_BASE_OUTBS_ADDR                              ,  8,31: 0,0,RW,\
    "output stream base address                                            "\
    ),
H2REG(HWIF_ENC_BASE_SLICE_NALS                              , 10,31: 0,0,RW,\
    "sizeTblBase                                                           "\
    ),
H2REG(HWIF_ENC_BASE_COMPR_COEF                              , 46,31: 0,0,RW,\
    "Base address for compressed coefficients.                             "\
    ),
H2REG(HWIF_ENC_BASE_INPUT_PIXY                              , 12,31: 0,0,RW,\
    "input image lum base address                                          "\
    ),
H2REG(HWIF_ENC_BASE_INPUT_PIXC                              , 13,31: 0,0,RW,\
    "input image cb base address                                           "\
    ),
H2REG(HWIF_ENC_BASE_INPUT_2NDC                              , 14,31: 0,0,RW,\
    "input image cr base address                                           "\
    ),
H2REG(HWIF_ENC_BASE_RECON_PIXY                              , 15,31: 0,0,RW,\
    "recon image lum base address                                          "\
    ),
H2REG(HWIF_ENC_BASE_RECON_PIXC                              , 16,31: 0,0,RW,\
    "recon image chroma base address                                       "\
    ),
H2REG(HWIF_ENC_BASE_RECON_Y4NX                              , 72,31: 0,0,RW,\
    "Base address for recon luma 4n base LSB.                              "\
    ),
H2REG(HWIF_ENC_BASE_RECON_CMPY                              , 60,31: 0,0,RW,\
    "Base address for recon luma compress table LSB.                       "\
    ),
H2REG(HWIF_ENC_BASE_RECON_CMPC                              , 62,31: 0,0,RW,\
    "Base address for recon Chroma compress table LSB.                     "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_PIXY                              , 18,31: 0,0,RW,\
    "reference picture reconstructed list0 luma0                           "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_PIXC                              , 19,31: 0,0,RW,\
    "reference picture reconstructed list0 chroma0                         "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_Y4NX                              , 74,31: 0,0,RW,\
    "reference picture reconstructed list0 4n 0                            "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_CMPY                              , 64,31: 0,0,RW,\
    "Base address for list 0 ref 0 luma compress table LSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_CMPC                              , 66,31: 0,0,RW,\
    "Base address for list 0 ref 0 Chroma compress table LSB.              "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_PIXY                              , 20,31: 0,0,RW,\
    "reference picture reconstructed list0 luma1                           "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_PIXC                              , 21,31: 0,0,RW,\
    "reference picture reconstructed list0 chroma1                         "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_Y4NX                              , 76,31: 0,0,RW,\
    "reference picture reconstructed list0 4n 1                            "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_CMPY                              , 68,31: 0,0,RW,\
    "Base address for list 0 ref 1 luma compress table LSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_CMPC                              , 70,31: 0,0,RW,\
    "Base address for list 0 ref 1 Chroma compress table LSB.              "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_PIXY                              , 83,31: 0,0,RW,\
    "reference picture reconstructed list1 luma0                           "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_PIXC                              , 84,31: 0,0,RW,\
    "reference picture reconstructed list1 chroma0                         "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_Y4NX                              , 92,31: 0,0,RW,\
    "reference picture reconstructed list1 4n 0                            "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_CMPY                              , 96,31: 0,0,RW,\
    "Base address for list 1 ref 0 luma compress table LSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_CMPC                              , 98,31: 0,0,RW,\
    "Base address for list 1 ref 0 Chroma compress table LSB.              "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_PIXY                              , 85,31: 0,0,RW,\
    "reference picture reconstructed list1 luma1                           "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_PIXC                              , 86,31: 0,0,RW,\
    "reference picture reconstructed list1 chroma1                         "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_Y4NX                              , 94,31: 0,0,RW,\
    "reference picture reconstructed list1 4n 1                            "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_CMPY                              ,100,31: 0,0,RW,\
    "Base address for list 1 ref 1 luma compress table LSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_CMPC                              ,102,31: 0,0,RW,\
    "Base address for list 1 ref 1 Chroma compress table LSB.              "\
    ),
H2REG(HWIF_ENC_BASE_ROIDQ_ADDR                              ,109,31: 0,0,RW,\
    "Qp delta map                                                          "\
    ),
H2REG(HWIF_ENC_BASE_CTBRC_CURR                              ,114,31: 0,0,RW,\
    "ctb rate control bit memory address of current frame                  "\
    ),
H2REG(HWIF_ENC_BASE_CTBRC_PREV                              ,116,31: 0,0,RW,\
    "ctb rate control bit memory address of previous frame                 "\
    ),
H2REG(HWIF_ENC_BASE_SCALE_OUTY                              , 42,31: 0,0,RW,\
    "Base address for output of down-scaled encoder image in YUYV 4:2:2 for"\
    "mat                                                                   "\
    ),
H2REG(HWIF_ENC_BASE_OUTBS_ADDR_MSB                          , 59,31: 0,0,RW,\
    "output stream base address MSB                                        "\
    ),
H2REG(HWIF_ENC_BASE_SLICE_NALS_MSB                          , 58,31: 0,0,RW,\
    "sizeTblBase MSB                                                       "\
    ),
H2REG(HWIF_ENC_BASE_COMPR_COEF_MSB                          , 47,31: 0,0,RW,\
    "Base address MSB for compressed coefficients.                         "\
    ),
H2REG(HWIF_ENC_BASE_INPUT_PIXY_MSB                          , 53,31: 0,0,RW,\
    "input image lum base address MSB                                      "\
    ),
H2REG(HWIF_ENC_BASE_INPUT_PIXC_MSB                          , 54,31: 0,0,RW,\
    "input image cb base address MSB                                       "\
    ),
H2REG(HWIF_ENC_BASE_INPUT_2NDC_MSB                          , 55,31: 0,0,RW,\
    "input image cr base address MSB                                       "\
    ),
H2REG(HWIF_ENC_BASE_RECON_PIXY_MSB                          , 56,31: 0,0,RW,\
    "recon image lum base address MSB                                      "\
    ),
H2REG(HWIF_ENC_BASE_RECON_PIXC_MSB                          , 57,31: 0,0,RW,\
    "recon image chroma base address MSB                                   "\
    ),
H2REG(HWIF_ENC_BASE_RECON_Y4NX_MSB                          , 73,31: 0,0,RW,\
    "Base address for recon luma 4n base MSB.                              "\
    ),
H2REG(HWIF_ENC_BASE_RECON_CMPY_MSB                          , 61,31: 0,0,RW,\
    "Base address for recon luma compress table MSB.                       "\
    ),
H2REG(HWIF_ENC_BASE_RECON_CMPC_MSB                          , 63,31: 0,0,RW,\
    "Base address for recon Chroma compress table MSB.                     "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_PIXY_MSB                          , 49,31: 0,0,RW,\
    "reference picture reconstructed list0 luma0 MSB                       "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_PIXC_MSB                          , 50,31: 0,0,RW,\
    "reference picture reconstructed list0 chroma0 MSB                     "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_Y4NX_MSB                          , 75,31: 0,0,RW,\
    "reference picture reconstructed list0 4n 0 MSB                        "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_CMPY_MSB                          , 65,31: 0,0,RW,\
    "Base address for list 0 ref 0 luma compress table MSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L0RP0_CMPC_MSB                          , 67,31: 0,0,RW,\
    "Base address for list 0 ref 0 Chroma compress table MSB.              "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_PIXY_MSB                          , 51,31: 0,0,RW,\
    "reference picture reconstructed list0 luma1 MSB                       "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_PIXC_MSB                          , 52,31: 0,0,RW,\
    "reference picture reconstructed list0 chroma1 MSB                     "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_Y4NX_MSB                          , 77,31: 0,0,RW,\
    "reference picture reconstructed list0 4n 1 MSB                        "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_CMPY_MSB                          , 69,31: 0,0,RW,\
    "Base address for list 0 ref 1 luma compress table MSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L0RP1_CMPC_MSB                          , 71,31: 0,0,RW,\
    "Base address for list 0 ref 1 Chroma compress table MSB.              "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_PIXY_MSB                          , 87,31: 0,0,RW,\
    "reference picture reconstructed list1 luma0 MSB                       "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_PIXC_MSB                          , 88,31: 0,0,RW,\
    "reference picture reconstructed list1 chroma0 MSB                     "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_Y4NX_MSB                          , 93,31: 0,0,RW,\
    "reference picture reconstructed list1 4n 0 MSB                        "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_CMPY_MSB                          , 97,31: 0,0,RW,\
    "Base address for list 1 ref 0 luma compress table MSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L1RP0_CMPC_MSB                          , 99,31: 0,0,RW,\
    "Base address for list 1 ref 0 Chroma compress table MSB.              "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_PIXY_MSB                          , 89,31: 0,0,RW,\
    "reference picture reconstructed list1 luma1 MSB                       "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_PIXC_MSB                          , 90,31: 0,0,RW,\
    "reference picture reconstructed list1 chroma1 MSB                     "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_Y4NX_MSB                          , 95,31: 0,0,RW,\
    "reference picture reconstructed list1 4n 1 MSB                        "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_CMPY_MSB                          ,101,31: 0,0,RW,\
    "Base address for list 1 ref 1 luma compress table MSB.                "\
    ),
H2REG(HWIF_ENC_BASE_L1RP1_CMPC_MSB                          ,103,31: 0,0,RW,\
    "Base address for list 1 ref 1 Chroma compress table MSB.              "\
    ),
H2REG(HWIF_ENC_BASE_ROIDQ_ADDR_MSB                          ,110,31: 0,0,RW,\
    "Qp delta map MSB                                                      "\
    ),
H2REG(HWIF_ENC_BASE_CTBRC_CURR_MSB                          ,115,31: 0,0,RW,\
    "ctb rate control bit memory address of current frame msb              "\
    ),
H2REG(HWIF_ENC_BASE_CTBRC_PREV_MSB                          ,117,31: 0,0,RW,\
    "ctb rate control bit memory address of previous frame msb             "\
    ),
H2REG(HWIF_ENC_BASE_SCALE_OUTY_MSB                          , 48,31: 0,0,RW,\
    "Base address MSB for output of down-scaled encoder image in YUYV 4:2:2"\
    " format                                                               "\
    ),
/* lambdas */
H2REG(HWIF_ENC_LAMBDA_MOTION_SSE                            , 35,31:18,0,RW,\
    "lambda for motion SSE                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SAO_C                                 , 37,31:18,0,RW,\
    "lambda for SAO chroma                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SAO_Y                                 , 37,17: 4,0,RW,\
    "lambda for SAO luma                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_0                          ,125,31:18,0,RW,\
    "intra SATD lambda 0                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_1                          ,125,17: 4,0,RW,\
    "intra SATD lambda 1                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_2                          ,126,31:18,0,RW,\
    "intra SATD lambda 2                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_3                          ,126,17: 4,0,RW,\
    "intra SATD lambda 3                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_4                          ,127,31:18,0,RW,\
    "intra SATD lambda 4                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_5                          ,127,17: 4,0,RW,\
    "intra SATD lambda 5                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_6                          ,128,31:18,0,RW,\
    "intra SATD lambda 6                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_7                          ,128,17: 4,0,RW,\
    "intra SATD lambda 7                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_8                          ,129,31:18,0,RW,\
    "intra SATD lambda 8                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA_9                          ,129,17: 4,0,RW,\
    "intra SATD lambda 9                                                   "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA10                          ,130,31:18,0,RW,\
    "intra SATD lambda 10                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA11                          ,130,17: 4,0,RW,\
    "intra SATD lambda 11                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA12                          ,131,31:18,0,RW,\
    "intra SATD lambda 12                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA13                          ,131,17: 4,0,RW,\
    "intra SATD lambda 13                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA14                          ,132,31:18,0,RW,\
    "intra SATD lambda 14                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA15                          ,132,17: 4,0,RW,\
    "intra SATD lambda 15                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA16                          ,150,31:18,0,RW,\
    "intra SATD lambda 16                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA17                          ,150,17: 4,0,RW,\
    "intra SATD lambda 17                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA18                          ,151,31:18,0,RW,\
    "intra SATD lambda 18                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA19                          ,151,17: 4,0,RW,\
    "intra SATD lambda 19                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA20                          ,152,31:18,0,RW,\
    "intra SATD lambda 20                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA21                          ,152,17: 4,0,RW,\
    "intra SATD lambda 21                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA22                          ,153,31:18,0,RW,\
    "intra SATD lambda 22                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA23                          ,153,17: 4,0,RW,\
    "intra SATD lambda 23                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA24                          ,154,31:18,0,RW,\
    "intra SATD lambda 24                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA25                          ,154,17: 4,0,RW,\
    "intra SATD lambda 25                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA26                          ,155,31:18,0,RW,\
    "intra SATD lambda 26                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA27                          ,155,17: 4,0,RW,\
    "intra SATD lambda 27                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA28                          ,156,31:18,0,RW,\
    "intra SATD lambda 28                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA29                          ,156,17: 4,0,RW,\
    "intra SATD lambda 29                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA30                          ,157,31:18,0,RW,\
    "intra SATD lambda 30                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTRA31                          ,157,17: 4,0,RW,\
    "intra SATD lambda 31                                                  "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_0                          , 28,31:24,0,RW,\
    "lambda satd me 0                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_1                          , 28,23:16,0,RW,\
    "lambda satd me 1                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_2                          , 28,15: 8,0,RW,\
    "lambda satd me 2                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_3                          , 28, 7: 0,0,RW,\
    "lambda satd me 3                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_4                          , 29,31:24,0,RW,\
    "lambda satd me 4                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_5                          , 29,23:16,0,RW,\
    "lambda satd me 5                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_6                          , 29,15: 8,0,RW,\
    "lambda satd me 6                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_7                          , 29, 7: 0,0,RW,\
    "lambda satd me 7                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_8                          , 30,31:24,0,RW,\
    "lambda satd me 8                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER_9                          , 30,23:16,0,RW,\
    "lambda satd me 9                                                      "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER10                          , 30,15: 8,0,RW,\
    "lambda satd me 10                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER11                          , 30, 7: 0,0,RW,\
    "lambda satd me 11                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER12                          , 31,31:24,0,RW,\
    "lambda satd me 12                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER13                          , 31,23:16,0,RW,\
    "lambda satd me 13                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER14                          , 31,15: 8,0,RW,\
    "lambda satd me 14                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER15                          , 31, 7: 0,0,RW,\
    "lambda satd me 15                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER16                          ,138,31:24,0,RW,\
    "lambda satd me 16                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER17                          ,138,23:16,0,RW,\
    "lambda satd me 17                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER18                          ,138,15: 8,0,RW,\
    "lambda satd me 18                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER19                          ,138, 7: 0,0,RW,\
    "lambda satd me 19                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER20                          ,139,31:24,0,RW,\
    "lambda satd me 20                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER21                          ,139,23:16,0,RW,\
    "lambda satd me 21                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER22                          ,139,15: 8,0,RW,\
    "lambda satd me 22                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER23                          ,139, 7: 0,0,RW,\
    "lambda satd me 23                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER24                          ,140,31:24,0,RW,\
    "lambda satd me 24                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER25                          ,140,23:16,0,RW,\
    "lambda satd me 25                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER26                          ,140,15: 8,0,RW,\
    "lambda satd me 26                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER27                          ,140, 7: 0,0,RW,\
    "lambda satd me 27                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER28                          ,141,31:24,0,RW,\
    "lambda satd me 28                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER29                          ,141,23:16,0,RW,\
    "lambda satd me 29                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER30                          ,141,15: 8,0,RW,\
    "lambda satd me 30                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SATD_INTER31                          ,141, 7: 0,0,RW,\
    "lambda satd me 31                                                     "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_0                          , 32,31:17,0,RW,\
    "inter me SSE lambda 0                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_1                          , 32,16: 2,0,RW,\
    "inter me SSE lambda 1                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_2                          , 33,31:17,0,RW,\
    "inter me SSE lambda 2                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_3                          , 33,16: 2,0,RW,\
    "inter me SSE lambda 3                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_4                          , 34,31:17,0,RW,\
    "inter me SSE lambda 4                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_5                          , 34,16: 2,0,RW,\
    "inter me SSE lambda 5                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_6                          , 78,31:17,0,RW,\
    "inter me SSE lambda 6                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_7                          , 78,16: 2,0,RW,\
    "inter me SSE lambda 7                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_8                          , 79,31:17,0,RW,\
    "inter me SSE lambda 8                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION_9                          , 79,16: 2,0,RW,\
    "inter me SSE lambda 9                                                 "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION10                          ,122,31:17,0,RW,\
    "inter me SSE lambda 10                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION11                          ,122,16: 2,0,RW,\
    "inter me SSE lambda 11                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION12                          ,123,31:17,0,RW,\
    "inter me SSE lambda 12                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION13                          ,123,16: 2,0,RW,\
    "inter me SSE lambda 13                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION14                          ,124,31:17,0,RW,\
    "inter me SSE lambda 14                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION15                          ,124,16: 2,0,RW,\
    "inter me SSE lambda 15                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION16                          ,142,31:17,0,RW,\
    "inter me SSE lambda 16                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION17                          ,142,16: 2,0,RW,\
    "inter me SSE lambda 17                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION18                          ,143,31:17,0,RW,\
    "inter me SSE lambda 18                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION19                          ,143,16: 2,0,RW,\
    "inter me SSE lambda 19                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION20                          ,144,31:17,0,RW,\
    "inter me SSE lambda 20                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION21                          ,144,16: 2,0,RW,\
    "inter me SSE lambda 21                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION22                          ,145,31:17,0,RW,\
    "inter me SSE lambda 22                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION23                          ,145,16: 2,0,RW,\
    "inter me SSE lambda 23                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION24                          ,146,31:17,0,RW,\
    "inter me SSE lambda 24                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION25                          ,146,16: 2,0,RW,\
    "inter me SSE lambda 25                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION26                          ,147,31:17,0,RW,\
    "inter me SSE lambda 26                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION27                          ,147,16: 2,0,RW,\
    "inter me SSE lambda 27                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION28                          ,148,31:17,0,RW,\
    "inter me SSE lambda 28                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION29                          ,148,16: 2,0,RW,\
    "inter me SSE lambda 29                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION30                          ,149,31:17,0,RW,\
    "inter me SSE lambda 30                                                "\
    ),
H2REG(HWIF_ENC_LAMBDA_SSE_MOTION31                          ,149,16: 2,0,RW,\
    "inter me SSE lambda 31                                                "\
    ),
H2REG(HWIF_ENC_ENTRIES                                      ,  0, 0: 0,0, 0,\
    "terminate entry                                                       "\
    ),
#undef RO
#undef WO
#undef RW
#undef H2REG

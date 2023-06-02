/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/fh_otg_core_if.h $
 * $Revision: #20 $
 * $Date: 2015/10/12 $
 * $Change: 2972621 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */
#if !defined(__FH_CORE_IF_H__)
#define __FH_CORE_IF_H__

#include "../fh_common_port/fh_os.h"

/** @file
 * This file defines FH_OTG Core API
 */

struct fh_otg_core_if;
typedef struct fh_otg_core_if fh_otg_core_if_t;

/** Maximum number of Periodic FIFOs */
#define MAX_PERIO_FIFOS 15
/** Maximum number of Periodic FIFOs */
#define MAX_TX_FIFOS 15

/** Maximum number of Endpoints/HostChannels */
#define MAX_EPS_CHANNELS 16

extern fh_otg_core_if_t *fh_otg_cil_init(const uint32_t * _reg_base_addr);
extern void fh_otg_core_init(fh_otg_core_if_t * _core_if);
extern void fh_otg_cil_remove(fh_otg_core_if_t * _core_if);

extern void fh_otg_enable_global_interrupts(fh_otg_core_if_t * _core_if);
extern void fh_otg_disable_global_interrupts(fh_otg_core_if_t * _core_if);

extern uint8_t fh_otg_is_device_mode(fh_otg_core_if_t * _core_if);
extern uint8_t fh_otg_is_host_mode(fh_otg_core_if_t * _core_if);

extern uint8_t fh_otg_is_dma_enable(fh_otg_core_if_t * core_if);

/** This function should be called on every hardware interrupt. */
extern int32_t fh_otg_handle_common_intr(void *otg_dev);


/** @name OTG Core Parameters */
/** @{ */

/**
 * Specifies the OTG capabilities. The driver will automatically
 * detect the value for this parameter if none is specified.
 * 0 - HNP and SRP capable (default)
 * 1 - SRP Only capable
 * 2 - No HNP/SRP capable
 */
extern int fh_otg_set_param_otg_cap(fh_otg_core_if_t * core_if, int32_t val);
extern int32_t fh_otg_get_param_otg_cap(fh_otg_core_if_t * core_if);
#define FH_OTG_CAP_PARAM_HNP_SRP_CAPABLE 0
#define FH_OTG_CAP_PARAM_SRP_ONLY_CAPABLE 1
#define FH_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE 2
#define fh_param_otg_cap_default FH_OTG_CAP_PARAM_HNP_SRP_CAPABLE

extern int fh_otg_set_param_opt(fh_otg_core_if_t * core_if, int32_t val);
extern int32_t fh_otg_get_param_opt(fh_otg_core_if_t * core_if);
#define fh_param_opt_default 1

/**
 * Specifies whether to use slave or DMA mode for accessing the data
 * FIFOs. The driver will automatically detect the value for this
 * parameter if none is specified.
 * 0 - Slave
 * 1 - DMA (default, if available)
 */
extern int fh_otg_set_param_dma_enable(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_dma_enable(fh_otg_core_if_t * core_if);
#define fh_param_dma_enable_default 1

/**
 * When DMA mode is enabled specifies whether to use
 * address DMA or DMA Descritor mode for accessing the data
 * FIFOs in device mode. The driver will automatically detect
 * the value for this parameter if none is specified.
 * 0 - address DMA
 * 1 - DMA Descriptor(default, if available)
 */
extern int fh_otg_set_param_dma_desc_enable(fh_otg_core_if_t * core_if,
					     int32_t val);
extern int32_t fh_otg_get_param_dma_desc_enable(fh_otg_core_if_t * core_if);
#define fh_param_dma_desc_enable_default 1

/** The DMA Burst size (applicable only for External DMA
 * Mode). 1, 4, 8 16, 32, 64, 128, 256 (default 32)
 */
extern int fh_otg_set_param_dma_burst_size(fh_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t fh_otg_get_param_dma_burst_size(fh_otg_core_if_t * core_if);
#define fh_param_dma_burst_size_default 32

/**
 * Specifies the maximum speed of operation in host and device mode.
 * The actual speed depends on the speed of the attached device and
 * the value of phy_type. The actual speed depends on the speed of the
 * attached device.
 * 0 - High Speed (default)
 * 1 - Full Speed
 */
extern int fh_otg_set_param_speed(fh_otg_core_if_t * core_if, int32_t val);
extern int32_t fh_otg_get_param_speed(fh_otg_core_if_t * core_if);
#define fh_param_speed_default 0
#define FH_SPEED_PARAM_HIGH 0
#define FH_SPEED_PARAM_FULL 1

/** Specifies whether low power mode is supported when attached
 *	to a Full Speed or Low Speed device in host mode.
 * 0 - Don't support low power mode (default)
 * 1 - Support low power mode
 */
extern int fh_otg_set_param_host_support_fs_ls_low_power(fh_otg_core_if_t *
							  core_if, int32_t val);
extern int32_t fh_otg_get_param_host_support_fs_ls_low_power(fh_otg_core_if_t
							      * core_if);
#define fh_param_host_support_fs_ls_low_power_default 0

/** Specifies the PHY clock rate in low power mode when connected to a
 * Low Speed device in host mode. This parameter is applicable only if
 * HOST_SUPPORT_FS_LS_LOW_POWER is enabled. If PHY_TYPE is set to FS
 * then defaults to 6 MHZ otherwise 48 MHZ.
 *
 * 0 - 48 MHz
 * 1 - 6 MHz
 */
extern int fh_otg_set_param_host_ls_low_power_phy_clk(fh_otg_core_if_t *
						       core_if, int32_t val);
extern int32_t fh_otg_get_param_host_ls_low_power_phy_clk(fh_otg_core_if_t *
							   core_if);
#define fh_param_host_ls_low_power_phy_clk_default 0
#define FH_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ 0
#define FH_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ 1

#define FH_CC_CLK_FREQ_30MHZ 0x7530

/**
 * 0 - Use cC FIFO size parameters
 * 1 - Allow dynamic FIFO sizing (default)
 */
extern int fh_otg_set_param_enable_dynamic_fifo(fh_otg_core_if_t * core_if,
						 int32_t val);
extern int32_t fh_otg_get_param_enable_dynamic_fifo(fh_otg_core_if_t *
						     core_if);
#define fh_param_enable_dynamic_fifo_default 1

/** Total number of 4-byte words in the data FIFO memory. This
 * memory includes the Rx FIFO, non-periodic Tx FIFO, and periodic
 * Tx FIFOs.
 * 32 to 32768 (default 8192)
 * Note: The total FIFO memory depth in the FPGA configuration is 8192.
 */
extern int fh_otg_set_param_data_fifo_size(fh_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t fh_otg_get_param_data_fifo_size(fh_otg_core_if_t * core_if);
#define fh_param_data_fifo_size_default 2560

/** Number of 4-byte words in the Rx FIFO in device mode when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1064)
 */
extern int fh_otg_set_param_dev_rx_fifo_size(fh_otg_core_if_t * core_if,
					      int32_t val);
extern int32_t fh_otg_get_param_dev_rx_fifo_size(fh_otg_core_if_t * core_if);
#define fh_param_dev_rx_fifo_size_default 1064

/** Number of 4-byte words in the non-periodic Tx FIFO in device mode
 * when dynamic FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int fh_otg_set_param_dev_nperio_tx_fifo_size(fh_otg_core_if_t *
						     core_if, int32_t val);
extern int32_t fh_otg_get_param_dev_nperio_tx_fifo_size(fh_otg_core_if_t *
							 core_if);
#define fh_param_dev_nperio_tx_fifo_size_default 128

/** Number of 4-byte words in each of the periodic Tx FIFOs in device
 * mode when dynamic FIFO sizing is enabled.
 * 4 to 768 (default 256)
 */
extern int fh_otg_set_param_dev_perio_tx_fifo_size(fh_otg_core_if_t * core_if,
						    int32_t val, int fifo_num);
extern int32_t fh_otg_get_param_dev_perio_tx_fifo_size(fh_otg_core_if_t *
							core_if, int fifo_num);
#define fh_param_dev_perio_tx_fifo_size_default 256

/** Number of 4-byte words in the Rx FIFO in host mode when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int fh_otg_set_param_host_rx_fifo_size(fh_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t fh_otg_get_param_host_rx_fifo_size(fh_otg_core_if_t * core_if);
#define fh_param_host_rx_fifo_size_default 512

/** Number of 4-byte words in the non-periodic Tx FIFO in host mode
 * when Dynamic FIFO sizing is enabled in the core.
 * 16 to 32768 (default 1024)
 */
extern int fh_otg_set_param_host_nperio_tx_fifo_size(fh_otg_core_if_t *
						      core_if, int32_t val);
extern int32_t fh_otg_get_param_host_nperio_tx_fifo_size(fh_otg_core_if_t *
							  core_if);
#define fh_param_host_nperio_tx_fifo_size_default 128

/** Number of 4-byte words in the host periodic Tx FIFO when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int fh_otg_set_param_host_perio_tx_fifo_size(fh_otg_core_if_t *
						     core_if, int32_t val);
extern int32_t fh_otg_get_param_host_perio_tx_fifo_size(fh_otg_core_if_t *
							 core_if);
#define fh_param_host_perio_tx_fifo_size_default 256

/** The maximum transfer size supported in bytes.
 * 2047 to 65,535  (default 65,535)
 */
extern int fh_otg_set_param_max_transfer_size(fh_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t fh_otg_get_param_max_transfer_size(fh_otg_core_if_t * core_if);
#define fh_param_max_transfer_size_default 65535

/** The maximum number of packets in a transfer.
 * 15 to 511  (default 511)
 */
extern int fh_otg_set_param_max_packet_count(fh_otg_core_if_t * core_if,
					      int32_t val);
extern int32_t fh_otg_get_param_max_packet_count(fh_otg_core_if_t * core_if);
#define fh_param_max_packet_count_default 511

/** The number of host channel registers to use.
 * 1 to 16 (default 12)
 * Note: The FPGA configuration supports a maximum of 12 host channels.
 */
extern int fh_otg_set_param_host_channels(fh_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t fh_otg_get_param_host_channels(fh_otg_core_if_t * core_if);
#define fh_param_host_channels_default 16

/** The number of endpoints in addition to EP0 available for device
 * mode operations.
 * 1 to 15 (default 6 IN and OUT)
 * Note: The FPGA configuration supports a maximum of 6 IN and OUT
 * endpoints in addition to EP0.
 */
extern int fh_otg_set_param_dev_endpoints(fh_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t fh_otg_get_param_dev_endpoints(fh_otg_core_if_t * core_if);
#define fh_param_dev_endpoints_default 8

/**
 * Specifies the type of PHY interface to use. By default, the driver
 * will automatically detect the phy_type.
 *
 * 0 - Full Speed PHY
 * 1 - UTMI+ (default)
 * 2 - ULPI
 */
extern int fh_otg_set_param_phy_type(fh_otg_core_if_t * core_if, int32_t val);
extern int32_t fh_otg_get_param_phy_type(fh_otg_core_if_t * core_if);
#define FH_PHY_TYPE_PARAM_FS 0
#define FH_PHY_TYPE_PARAM_UTMI 1
#define FH_PHY_TYPE_PARAM_ULPI 2
#define fh_param_phy_type_default FH_PHY_TYPE_PARAM_UTMI

/**
 * Specifies the UTMI+ Data Width. This parameter is
 * applicable for a PHY_TYPE of UTMI+ or ULPI. (For a ULPI
 * PHY_TYPE, this parameter indicates the data width between
 * the MAC and the ULPI Wrapper.) Also, this parameter is
 * applicable only if the OTG_HSPHY_WIDTH cC parameter was set
 * to "8 and 16 bits", meaning that the core has been
 * configured to work at either data path width.
 *
 * 8 or 16 bits (default 16)
 */
extern int fh_otg_set_param_phy_utmi_width(fh_otg_core_if_t * core_if,
					    int32_t val);
extern int32_t fh_otg_get_param_phy_utmi_width(fh_otg_core_if_t * core_if);
#define fh_param_phy_utmi_width_default 16

/**
 * Specifies whether the ULPI operates at double or single
 * data rate. This parameter is only applicable if PHY_TYPE is
 * ULPI.
 *
 * 0 - single data rate ULPI interface with 8 bit wide data
 * bus (default)
 * 1 - double data rate ULPI interface with 4 bit wide data
 * bus
 */
extern int fh_otg_set_param_phy_ulpi_ddr(fh_otg_core_if_t * core_if,
					  int32_t val);
extern int32_t fh_otg_get_param_phy_ulpi_ddr(fh_otg_core_if_t * core_if);
#define fh_param_phy_ulpi_ddr_default 0

/**
 * Specifies whether to use the internal or external supply to
 * drive the vbus with a ULPI phy.
 */
extern int fh_otg_set_param_phy_ulpi_ext_vbus(fh_otg_core_if_t * core_if,
					       int32_t val);
extern int32_t fh_otg_get_param_phy_ulpi_ext_vbus(fh_otg_core_if_t * core_if);
#define FH_PHY_ULPI_INTERNAL_VBUS 0
#define FH_PHY_ULPI_EXTERNAL_VBUS 1
#define fh_param_phy_ulpi_ext_vbus_default FH_PHY_ULPI_INTERNAL_VBUS

/**
 * Specifies whether to use the I2Cinterface for full speed PHY. This
 * parameter is only applicable if PHY_TYPE is FS.
 * 0 - No (default)
 * 1 - Yes
 */
extern int fh_otg_set_param_i2c_enable(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_i2c_enable(fh_otg_core_if_t * core_if);
#define fh_param_i2c_enable_default 0

extern int fh_otg_set_param_ulpi_fs_ls(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_ulpi_fs_ls(fh_otg_core_if_t * core_if);
#define fh_param_ulpi_fs_ls_default 0

extern int fh_otg_set_param_ts_dline(fh_otg_core_if_t * core_if, int32_t val);
extern int32_t fh_otg_get_param_ts_dline(fh_otg_core_if_t * core_if);
#define fh_param_ts_dline_default 0

/**
 * Specifies whether dedicated transmit FIFOs are
 * enabled for non periodic IN endpoints in device mode
 * 0 - No
 * 1 - Yes
 */
extern int fh_otg_set_param_en_multiple_tx_fifo(fh_otg_core_if_t * core_if,
						 int32_t val);
extern int32_t fh_otg_get_param_en_multiple_tx_fifo(fh_otg_core_if_t *
						     core_if);
#define fh_param_en_multiple_tx_fifo_default 1

/** Number of 4-byte words in each of the Tx FIFOs in device
 * mode when dynamic FIFO sizing is enabled.
 * 4 to 768 (default 256)
 */
extern int fh_otg_set_param_dev_tx_fifo_size(fh_otg_core_if_t * core_if,
					      int fifo_num, int32_t val);
extern int32_t fh_otg_get_param_dev_tx_fifo_size(fh_otg_core_if_t * core_if,
						  int fifo_num);
#define fh_param_dev_tx_fifo_size_default 256

/** Thresholding enable flag-
 * bit 0 - enable non-ISO Tx thresholding
 * bit 1 - enable ISO Tx thresholding
 * bit 2 - enable Rx thresholding
 */
extern int fh_otg_set_param_thr_ctl(fh_otg_core_if_t * core_if, int32_t val);
extern int32_t fh_otg_get_thr_ctl(fh_otg_core_if_t * core_if, int fifo_num);
#define fh_param_thr_ctl_default 0

/** Thresholding length for Tx
 * FIFOs in 32 bit DWORDs
 */
extern int fh_otg_set_param_tx_thr_length(fh_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t fh_otg_get_tx_thr_length(fh_otg_core_if_t * core_if);
#define fh_param_tx_thr_length_default 64

/** Thresholding length for Rx
 *	FIFOs in 32 bit DWORDs
 */
extern int fh_otg_set_param_rx_thr_length(fh_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t fh_otg_get_rx_thr_length(fh_otg_core_if_t * core_if);
#define fh_param_rx_thr_length_default 64

/**
 * Specifies whether LPM (Link Power Management) support is enabled
 */
extern int fh_otg_set_param_lpm_enable(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_lpm_enable(fh_otg_core_if_t * core_if);
#define fh_param_lpm_enable_default 1

/**
 * Specifies whether LPM Errata (Link Power Management) support is enabled
 */
extern int fh_otg_set_param_besl_enable(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_besl_enable(fh_otg_core_if_t * core_if);
#define fh_param_besl_enable_default 0

/**
 * Specifies baseline_besl default value
 */
extern int fh_otg_set_param_baseline_besl(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_baseline_besl(fh_otg_core_if_t * core_if);
#define fh_param_baseline_besl_default 0

/**
 * Specifies deep_besl default value
 */
extern int fh_otg_set_param_deep_besl(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_deep_besl(fh_otg_core_if_t * core_if);
#define fh_param_deep_besl_default 15

/**
 * Specifies whether PTI enhancement is enabled
 */
extern int fh_otg_set_param_pti_enable(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_pti_enable(fh_otg_core_if_t * core_if);
#define fh_param_pti_enable_default 0

/**
 * Specifies whether MPI enhancement is enabled
 */
extern int fh_otg_set_param_mpi_enable(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_mpi_enable(fh_otg_core_if_t * core_if);
#define fh_param_mpi_enable_default 0

/**
 * Specifies whether ADP capability is enabled
 */
extern int fh_otg_set_param_adp_enable(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_adp_enable(fh_otg_core_if_t * core_if);
#define fh_param_adp_enable_default 0

/**
 * Specifies whether IC_USB capability is enabled
 */

extern int fh_otg_set_param_ic_usb_cap(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_ic_usb_cap(fh_otg_core_if_t * core_if);
#define fh_param_ic_usb_cap_default 0

extern int fh_otg_set_param_ahb_thr_ratio(fh_otg_core_if_t * core_if,
					   int32_t val);
extern int32_t fh_otg_get_param_ahb_thr_ratio(fh_otg_core_if_t * core_if);
#define fh_param_ahb_thr_ratio_default 0

extern int fh_otg_set_param_power_down(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_power_down(fh_otg_core_if_t * core_if);
#define fh_param_power_down_default 0

extern int fh_otg_set_param_reload_ctl(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_reload_ctl(fh_otg_core_if_t * core_if);
#define fh_param_reload_ctl_default 0

extern int fh_otg_set_param_dev_out_nak(fh_otg_core_if_t * core_if,
					 int32_t val);
extern int32_t fh_otg_get_param_dev_out_nak(fh_otg_core_if_t * core_if);
#define fh_param_dev_out_nak_default 0

extern int fh_otg_set_param_cont_on_bna(fh_otg_core_if_t * core_if,
					 int32_t val);
extern int32_t fh_otg_get_param_cont_on_bna(fh_otg_core_if_t * core_if);
#define fh_param_cont_on_bna_default 0

extern int fh_otg_set_param_ahb_single(fh_otg_core_if_t * core_if,
					int32_t val);
extern int32_t fh_otg_get_param_ahb_single(fh_otg_core_if_t * core_if);
#define fh_param_ahb_single_default 0

extern int fh_otg_set_param_otg_ver(fh_otg_core_if_t * core_if, int32_t val);
extern int32_t fh_otg_get_param_otg_ver(fh_otg_core_if_t * core_if);
#define fh_param_otg_ver_default 0

/** @} */

/** @name Access to registers and bit-fields */

/**
 * Dump core registers and SPRAM
 */
extern void fh_otg_dump_dev_registers(fh_otg_core_if_t * _core_if);
extern void fh_otg_dump_spram(fh_otg_core_if_t * _core_if);
extern void fh_otg_dump_host_registers(fh_otg_core_if_t * _core_if);
extern void fh_otg_dump_global_registers(fh_otg_core_if_t * _core_if);

/**
 * Get host negotiation status.
 */
extern uint32_t fh_otg_get_hnpstatus(fh_otg_core_if_t * core_if);

/**
 * Get srp status
 */
extern uint32_t fh_otg_get_srpstatus(fh_otg_core_if_t * core_if);

/**
 * Set hnpreq bit in the GOTGCTL register.
 */
extern void fh_otg_set_hnpreq(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get Content of SNPSID register.
 */
extern uint32_t fh_otg_get_gsnpsid(fh_otg_core_if_t * core_if);

/**
 * Get current mode.
 * Returns 0 if in device mode, and 1 if in host mode.
 */
extern uint32_t fh_otg_get_mode(fh_otg_core_if_t * core_if);

/**
 * Get value of hnpcapable field in the GUSBCFG register
 */
extern uint32_t fh_otg_get_hnpcapable(fh_otg_core_if_t * core_if);
/**
 * Set value of hnpcapable field in the GUSBCFG register
 */
extern void fh_otg_set_hnpcapable(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of srpcapable field in the GUSBCFG register
 */
extern uint32_t fh_otg_get_srpcapable(fh_otg_core_if_t * core_if);
/**
 * Set value of srpcapable field in the GUSBCFG register
 */
extern void fh_otg_set_srpcapable(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of devspeed field in the DCFG register
 */
extern uint32_t fh_otg_get_devspeed(fh_otg_core_if_t * core_if);
/**
 * Set value of devspeed field in the DCFG register
 */
extern void fh_otg_set_devspeed(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get the value of busconnected field from the HPRT0 register
 */
extern uint32_t fh_otg_get_busconnected(fh_otg_core_if_t * core_if);

/**
 * Gets the device enumeration Speed.
 */
extern uint32_t fh_otg_get_enumspeed(fh_otg_core_if_t * core_if);

/**
 * Get value of prtpwr field from the HPRT0 register
 */
extern uint32_t fh_otg_get_prtpower(fh_otg_core_if_t * core_if);

/**
 * Get value of flag indicating core state - hibernated or not
 */
extern uint32_t fh_otg_get_core_state(fh_otg_core_if_t * core_if);

/**
 * Set value of prtpwr field from the HPRT0 register
 */
extern void fh_otg_set_prtpower(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of prtsusp field from the HPRT0 regsiter
 */
extern uint32_t fh_otg_get_prtsuspend(fh_otg_core_if_t * core_if);
/**
 * Set value of prtpwr field from the HPRT0 register
 */
extern void fh_otg_set_prtsuspend(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of ModeChTimEn field from the HCFG regsiter
 */
extern uint32_t fh_otg_get_mode_ch_tim(fh_otg_core_if_t * core_if);
/**
 * Set value of ModeChTimEn field from the HCFG regsiter
 */
extern void fh_otg_set_mode_ch_tim(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of Fram Interval field from the HFIR regsiter
 */
extern uint32_t fh_otg_get_fr_interval(fh_otg_core_if_t * core_if);
/**
 * Set value of Frame Interval field from the HFIR regsiter
 */
extern void fh_otg_set_fr_interval(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Set value of prtres field from the HPRT0 register
 *FIXME Remove?
 */
extern void fh_otg_set_prtresume(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of rmtwkupsig bit in DCTL register
 */
extern uint32_t fh_otg_get_remotewakesig(fh_otg_core_if_t * core_if);

/**
 * Get value of besl_reject bit in DCTL register
 */
 
extern uint32_t fh_otg_get_beslreject(fh_otg_core_if_t * core_if);

/**
 * Set value of besl_reject bit in DCTL register
 */
 
extern void fh_otg_set_beslreject(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of prt_sleep_sts field from the GLPMCFG register
 */
extern uint32_t fh_otg_get_lpm_portsleepstatus(fh_otg_core_if_t * core_if);

/**
 * Get value of rem_wkup_en field from the GLPMCFG register
 */
extern uint32_t fh_otg_get_lpm_remotewakeenabled(fh_otg_core_if_t * core_if);

/**
 * Get value of appl_resp field from the GLPMCFG register
 */
extern uint32_t fh_otg_get_lpmresponse(fh_otg_core_if_t * core_if);
/**
 * Set value of appl_resp field from the GLPMCFG register
 */
extern void fh_otg_set_lpmresponse(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of hsic_connect field from the GLPMCFG register
 */
extern uint32_t fh_otg_get_hsic_connect(fh_otg_core_if_t * core_if);
/**
 * Set value of hsic_connect field from the GLPMCFG register
 */
extern void fh_otg_set_hsic_connect(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * Get value of inv_sel_hsic field from the GLPMCFG register.
 */
extern uint32_t fh_otg_get_inv_sel_hsic(fh_otg_core_if_t * core_if);
/**
 * Set value of inv_sel_hsic field from the GLPMFG register.
 */
extern void fh_otg_set_inv_sel_hsic(fh_otg_core_if_t * core_if, uint32_t val);
/**
 * Set value of hird_thresh field from the GLPMFG register.
 */
extern void fh_otg_set_hirdthresh(fh_otg_core_if_t * core_if, uint32_t val);
/**
 * Get value of hird_thresh field from the GLPMFG register.
 */
extern uint32_t fh_otg_get_hirdthresh(fh_otg_core_if_t * core_if);


/*
 * Some functions for accessing registers
 */

/**
 *  GOTGCTL register
 */
extern uint32_t fh_otg_get_gotgctl(fh_otg_core_if_t * core_if);
extern void fh_otg_set_gotgctl(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * GUSBCFG register
 */
extern uint32_t fh_otg_get_gusbcfg(fh_otg_core_if_t * core_if);
extern void fh_otg_set_gusbcfg(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * GRXFSIZ register
 */
extern uint32_t fh_otg_get_grxfsiz(fh_otg_core_if_t * core_if);
extern void fh_otg_set_grxfsiz(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * GNPTXFSIZ register
 */
extern uint32_t fh_otg_get_gnptxfsiz(fh_otg_core_if_t * core_if);
extern void fh_otg_set_gnptxfsiz(fh_otg_core_if_t * core_if, uint32_t val);

extern uint32_t fh_otg_get_gpvndctl(fh_otg_core_if_t * core_if);
extern void fh_otg_set_gpvndctl(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * GGPIO register
 */
extern uint32_t fh_otg_get_ggpio(fh_otg_core_if_t * core_if);
extern void fh_otg_set_ggpio(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * GUID register
 */
extern uint32_t fh_otg_get_guid(fh_otg_core_if_t * core_if);
extern void fh_otg_set_guid(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * HPRT0 register
 */
extern uint32_t fh_otg_get_hprt0(fh_otg_core_if_t * core_if);
extern void fh_otg_set_hprt0(fh_otg_core_if_t * core_if, uint32_t val);

/**
 * GHPTXFSIZE
 */
extern uint32_t fh_otg_get_hptxfsiz(fh_otg_core_if_t * core_if);

/** @} */

#endif /* __FH_CORE_IF_H__ */

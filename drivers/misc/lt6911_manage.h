/* LT6911: GPL-2.0+
 *
 * linux/drivers/misc/ibmvmc.h
 *
 * IBM Power Systems Virtual Management Channel Support.
 *
 * Copyright (c) 2004, 2018 IBM Corp.
 *   Dave Engebretsen engebret@us.ibm.com
 *   Steven Royer seroyer@linux.vnet.ibm.com
 *   Adam Reznechek adreznec@linux.vnet.ibm.com
 *   Bryant G. Ly <bryantly@linux.vnet.ibm.com>
 */
#ifndef LT6911_MANAGE_H
#define LT6911_MANAGE_H

// #define CHECK_BOARD_VERSION         // Comment to select the default NanoKVM Pro
// #define RE_WRITE_VERSION            // Uncomment to enable version re-write function

// AX Pi GPIO
#define AX_PI_INT_PIN               97      // LT6911UXC INIT_Pin/GPIO5
#define AX_PI_PWR_PIN               19      // LT6911UXC PWR_Pin
// NanoKVM Pro GPIO
#define NANOKVM_PRO_INT_PIN         60      // LT6911UXC INIT_Pin/GPIO5
#define NANOKVM_PRO_PWR_PIN         5       // LT6911UXC PWR_Pin
#define NANOKVM_PRO_HDMI_PIN        6       // LT86102UXC HDMI_Pin
#define NANOKVM_PRO_HDMI_RXI_PIN    82      // HDMI RX Interface State Pin
#define NANOKVM_PRO_HDMI_TXI_PIN    83      // HDMI TX/LOOPOUT Interface State Pin
#define NANOKVM_PRO_HDMI_TXO_PIN    21      // HDMI TX/LOOPOUT Interface Ctrl Pin
#define NANOKVM_PRO_LOOP_CTRL_PIN   81      // HDMI LOOPOUT Interface Ctrl Pin2

#define I2C_BUS                     0       // I2C BUS Number
#define LT6911_ADDR                 0x2b    // LT6911UXC I2C Addr
#define LT86102UXE_ADDR             0x44    // LT6911UXC I2C Addr
#define Int_Action                  IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING
#define REG_ADDR                    0xff    // Target register address
#define WRITE_DATA                  0x80    // To write data command

// LT6911UXC Re
#define I2C_ADDRESS                 0x2b    // I2C device address (adjust according to actual device)
#define LT6911_REG_OFFSET           0xFF    // LT6911UXC register offset address
#define LT6911_SYS_OFFSET           0x80    // LT6911UXC register offset address
#define LT6911_SYS2_OFFSET          0x90    // LT6911UXC register offset address
#define LT6911_SYS3_OFFSET          0x81    // LT6911UXC register offset address
#define LT6911_SYS4_OFFSET          0xA0    // LT6911UXC register offset address
#define LT6911_CSI_INFO_OFFSET      0x85    // LT6911UXC CSI interface info register offset
#define LT6911_HDMI_INFO_OFFSET     0x86    // LT6911UXC HDMI info register offset
#define LT6911_CSI_TOTAL_OFFSET     0xD4    // LT6911UXC CSI bus statistics
#define LT6911_AUDIO_INFO_OFFSET    0xB0    // LT6911UXC audio info register offset
#define LT6911C_HDMI_INFO_OFFSET    0xD2    // LT6911C HDMI info register offset
#define LT6911C_AUDIO_INFO_OFFSET   0xD1    // LT6911C audio info register offset
#define LT6911C_CSI_INFO_OFFSET     0xC2    // LT6911C CSI info register offset
#define LT6911D_MANAGE_OFFSET       0xE0    // LT6911D management register offset
#define LT6911D_DATA_OFFSET         0xE1    // LT6911D data register offset

#define EDID_BUFFER_SIZE            256     // Maximum supported bytes
#define LT6911UXC_WR_SIZE           32      // LT6911UXC max read/write bytes per operation
#define LT6911C_WR_SIZE             16      // LT6911C max read/write bytes per operation
#define LT6911D_WR_SIZE             32      // LT6911D max read/write bytes per operation

#define NORMAL_RES                  0
#define NEW_RES                     1
#define UNSUPPORT_RES               2
#define UNKNOWN_RES                 3
#define ERROR_RES                   4

// SOC Register
#define REG_REMAP_SIZE              0x1000

// proc file names
#define PROC_LT6911_DIR             "lt6911_info"
#define PROC_HDMI_STATUS            "status"
#define PROC_HDMI_WIDTH             "width"
#define PROC_HDMI_HEIGHT            "height"
#define PROC_CSI_PWR                "power"
#define PROC_HDMI_PWR               "hdmi_power"
#define PROC_LOOPOUT_PWR            "loopout_power"
#define PROC_HDMI_FPS               "fps"
#define PROC_HDMI_HDCP              "hdcp"
#define PROC_AUDIO_SAMPLE_RATE      "asr"
#define PROC_HDMI_RX_STATUS         "hdmi_rx_status"
#define PROC_HDMI_TX_STATUS         "hdmi_tx_status"
#define PROC_HDMI_EDID              "edid"
#define PROC_HDMI_EDID_SNAPSHOT     "edid_snapshot"
#define PROC_VERSION                "version"
#define PROC_CHIP_ID                "chip_id"

#endif /* __LT6911_MANAGE_H */

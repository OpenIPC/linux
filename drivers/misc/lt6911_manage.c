/*
 * change log :
 * 0.0.1  - test i2c&gpio
 * 0.0.2  - fix i2c write function
 * 0.0.3  - add INT&IRQ
 * 0.0.4  - use handler work
 * 0.0.5  - add proc file system
 * 0.0.6  - add hdmi&audio info to proc file
 * 0.0.7  - add AX Pi & NanoKVM Pro support
 * 0.0.8  - auto detect chip & board version
 * 0.0.9  - add support for LT6911C
 * 0.0.9  - fix status & asr for LT6911C
 * 0.0.10 - add support for LT86102UXC Power
 * 0.0.11 - fix no HDMI input issue when insmod
 * 0.0.12 - fix nnknown "Audio signal": 0x01
 * 0.0.13 - optimize the Proc recycling mechanism
 * 0.0.14 - add HDMI loopback control
 * 0.0.15 - delete "unknown res", "unsupport res"
 * 0.0.16 - fix chip version error
 * 0.0.17 - added proc file for EDID read/write
 * 0.0.18 - fix int pin pull up issue for new board (30126D)
 * 0.0.19 - add version w/r functions
 * 0.0.20 - optimize the logic for version w/r
 * 0.0.21 - optimize version w/r functions
 * 0.0.22 - add HDMI loopout power control
 * 0.0.23 - add HDMI EDID snapshot support
 * 0.0.24 - add more resolution support
 * 0.0.25 - fix loopout power control issue
 * 0.0.26 - add support for 30126F
 * 0.0.27 - support poll for hdmi_status, hdmi_rx_status, hdmi_tx_status
 * 0.0.28 - add support for LT6911D
 * 0.0.29 - add feature: Specify output status and resolution with input parameter
 * 0.0.30 - rename parameters and proc files, add force fps parameter
 * 0.0.31 - fix LT6911D issue
 * 0.0.32 - add chip_id proc file
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include <linux/slab.h>

#include "lt6911_manage.h"

static int force_width = -1;
static int force_height = -1;
static int force_fps = -1;

void lt6911_force_resolution(u16 width, u16 height, int fps);
static struct work_struct get_hdmi_info_work;

static int force_res_param_set(const char *val, const struct kernel_param *kp)
{
    int old_val = *(int *)kp->arg;
    int ret = param_set_int(val, kp);
    if (ret)
        return ret;

    if (old_val != -1 && *(int *)kp->arg == -1) {
        schedule_work(&get_hdmi_info_work);
        return 0;
    }

    if (force_width > 0 && force_height > 0)
        lt6911_force_resolution((u16)force_width, (u16)force_height, force_fps);

    return 0;
}

static const struct kernel_param_ops force_res_param_ops = {
    .set = force_res_param_set,
    .get = param_get_int,
};

// module use parameters with "insmod lt6911_manage.ko force_width=1920 force_height=1080 force_fps=30"
module_param_cb(force_width, &force_res_param_ops, &force_width, 0644);
MODULE_PARM_DESC(force_width, "Force HDMI width");
module_param_cb(force_height, &force_res_param_ops, &force_height, 0644);
MODULE_PARM_DESC(force_height, "Force HDMI height");
module_param_cb(force_fps, &force_res_param_ops, &force_fps, 0644);
MODULE_PARM_DESC(force_fps, "Force HDMI fps");

static int irq_number;
static struct i2c_client *client;

enum {
    LT6911_CHIP_UNKNOWN,
    LT6911_CHIP_LT6911C,
    LT6911_CHIP_LT6911UXC,
    LT6911_CHIP_LT6911D,
} typedef chip_platform_t;

enum {
    BOARD_VERSION_UNKNOWN,
    BOARD_VERSION_AX_PI,
    BOARD_VERSION_NanoKVM_PRO,
} typedef board_version_t;

enum {
    HDMI_RXI_STATE,
    HDMI_TXI_STATE,
} typedef hdmi_state_t;

static chip_platform_t chip_platform = LT6911_CHIP_UNKNOWN;
static board_version_t board_version = BOARD_VERSION_NanoKVM_PRO;

static int INT_PIN;
static int PWR_PIN;
static int HDMI_PIN;
static int HDMI_RXI_PIN;
static int HDMI_TXI_PIN;
static int HDMI_TXO_PIN;
static int LOOP_CTRL_PIN;

// old offset set to 0xff first
static u8 old_offset = 0xff;

u16 hdmi_res_list[][2] = {
    {3840, 2400},
    {3840, 2160},
    {3440, 1440},
    {2560, 1600},
    {2560, 1440},
    {2560, 1080},
    {2048, 1536},
    {2048, 1152},
    {1920, 1440},
    {1920, 1200},
    {1920, 1080},
    {1680, 1050},
    {1600, 1200},
    {1600, 900},
    {1440, 1080},
    {1440, 900},
    {1440, 1050},
    {1368, 768},
    {1280, 1024},
    {1280, 960},
    {1280, 800},
    {1280, 720},
    {1152, 864},
    {1024, 768},
    {800, 600},
};

u16 hdmi_unsupported_res_list[][2] = {
    {1366, 768},
};

static struct proc_dir_entry *proc_lt6911_dir;
static struct proc_dir_entry *proc_hdmi_status_file;
static struct proc_dir_entry *proc_hdmi_width_file;
static struct proc_dir_entry *proc_hdmi_height_file;
static struct proc_dir_entry *proc_csi_power_file;
static struct proc_dir_entry *proc_hdmi_power_file;
static struct proc_dir_entry *proc_loopout_power_file;
static struct proc_dir_entry *proc_hdmi_fps_file;
static struct proc_dir_entry *proc_hdmi_hdcp_file;
static struct proc_dir_entry *proc_audio_sample_rate_file;
static struct proc_dir_entry *proc_hdmi_rx_status_file;
static struct proc_dir_entry *proc_hdmi_tx_status_file;
static struct proc_dir_entry *proc_hdmi_edid_file;
static struct proc_dir_entry *proc_hdmi_edid_snapshot_file;
static struct proc_dir_entry *proc_version_file;
static struct proc_dir_entry *proc_chip_id_file;
// static char buffer[128];
// static int buffer_length;
static char hdmi_status_buffer[16];
static char hdmi_status_write_buffer[16];
static char hdmi_width_buffer[16];
static char hdmi_height_buffer[16];
static char csi_power_buffer[16];
static char hdmi_power_buffer[16];
static char loopout_power_buffer[16];
static char csi_power_write_buffer[16];
static char hdmi_power_write_buffer[16];
static char loopout_power_write_buffer[16];
static char hdmi_fps_buffer[16];
static char hdmi_hdcp_buffer[16];
static char audio_sample_rate_buffer[16];
static char hdmi_rx_status_buffer[16];
static char hdmi_tx_status_buffer[16];
static char hdmi_edid_buffer[256];
static char hdmi_edid_snapshot_buffer[256];
static char hdmi_version_buffer[32] = {0};

static int hdmi_status_buffer_length;
// static int hdmi_status_write_buffer_length;
static int hdmi_width_buffer_length;
static int hdmi_height_buffer_length;
static int csi_power_buffer_length;
static int hdmi_power_buffer_length;
static int loopout_power_buffer_length;
static int csi_power_write_buffer_length;
static int hdmi_power_write_buffer_length;
static int loopout_power_write_buffer_length;
static int hdmi_fps_buffer_length;
static int hdmi_hdcp_buffer_length;
static int audio_sample_rate_buffer_length;
static int hdmi_rx_status_buffer_length;
static int hdmi_tx_status_buffer_length;
static int hdmi_edid_buffer_length;
static int hdmi_edid_snapshot_buffer_length;
static int hdmi_version_buffer_length;

#define READ_WIDTH_FLAG 0x01
#define READ_HEIGHT_FLAG 0x02

static irqreturn_t gpio_irq_handler(int irq, void *dev_id);
int lt6911_pwr_ctrl(int pwr_en);
int lt86102_pwr_ctrl(int pwr_en);
int set_hdmi_loopout(int enable);
int get_hdmi_interface_state(hdmi_state_t interface);
int check_edid(u8 *edid_data, u16 edid_size);
int lt6911_edid_write(u8 *edid_data, u16 edid_size);
int lt6911_edid_read(u8 *edid_data, u16 edid_size);
int lt6911_str_write(u8 *str, u16 len);
int lt6911_str_read(u8 *str);

struct gpio_irq_ctx {
    struct delayed_work work;
    int rxi_irq;
    int txi_irq;
};

static struct gpio_irq_ctx irq_ctx = {
    .rxi_irq = -1,
    .txi_irq = -1
};

struct lt6911_event_ctx {
    wait_queue_head_t wait_queue;
    atomic_t status;
    atomic_t rx_status;
    atomic_t tx_status;
};

static struct lt6911_event_ctx lt6911_ctx = {
    .wait_queue = __WAIT_QUEUE_HEAD_INITIALIZER(lt6911_ctx.wait_queue),
    .status     = ATOMIC_INIT(0),
    .rx_status  = ATOMIC_INIT(0),
    .tx_status  = ATOMIC_INIT(0),
};

struct lt6911_priv_data {
    int last_status;
    int last_rx_status;
    int last_tx_status;
};

// void update_status(u8 read_flag)
// {
//     static u8 read_flags = 0;
//     read_flags |= read_flag;
//     if (read_flags == 0x03) {
//         read_flags = 0; // reset read flags
//         snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "stable\n");
//         hdmi_status_buffer_length = strlen(hdmi_status_buffer);
//     }
// }
void update_status(void)
{
    // if buffer is :"new res"/"normal res"/"unsupport res"/"unknown res"/"error res" , buffer = stable
    if (hdmi_status_buffer_length > 0 &&
        (strncmp(hdmi_status_buffer, "new res", 7) == 0 ||
         strncmp(hdmi_status_buffer, "normal res", 11) == 0 ||
         strncmp(hdmi_status_buffer, "unsupport res", 13) == 0 ||
         strncmp(hdmi_status_buffer, "unknown res", 12) == 0 ||
         strncmp(hdmi_status_buffer, "error res", 9) == 0)) {
        snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "stable\n");
        hdmi_status_buffer_length = strlen(hdmi_status_buffer);

        atomic_inc(&lt6911_ctx.status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);
        return;
    }
}

ssize_t proc_csi_power_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    return simple_read_from_buffer(user_buffer, count, offset, csi_power_buffer, csi_power_buffer_length);
}

ssize_t proc_csi_power_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{

    if (count > sizeof(csi_power_write_buffer) - 1) {
        return -EINVAL; // Buffer overflow
    }

    if (copy_from_user(csi_power_write_buffer, user_buffer, count)) {
        return -EFAULT; // Copy failed
    }

    csi_power_write_buffer[count] = '\0'; // Null-terminate the string
    csi_power_write_buffer_length = count; // Update buffer length

    // check if "on" or "off"
    if ((strncmp(csi_power_write_buffer, "on", 2) == 0) || (strncmp(csi_power_write_buffer, "1", 1) == 0)) {
        if (lt6911_pwr_ctrl(1) < 0) {
            return -EIO; // Power control failed
        }
        // printk(KERN_INFO "Turning HDMI power on, buffer: %s\n", hdmi_power_buffer);
    } else if ((strncmp(csi_power_write_buffer, "off", 3) == 0) || (strncmp(csi_power_write_buffer, "0", 1) == 0)) {
        if (lt6911_pwr_ctrl(0) < 0) {
            return -EIO; // Power control failed
        }
        // update hdmi_status_buffer
        snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "disappear\n");
        hdmi_status_buffer_length = strlen(hdmi_status_buffer);

        atomic_inc(&lt6911_ctx.status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);
        // printk(KERN_INFO "Turning HDMI power off, buffer: %s\n", hdmi_power_buffer);
    } else {
        // printk(KERN_INFO "Turning HDMI power error\n");
        return -EINVAL; // Invalid input
    }

    return count; // Return number of bytes written
}

ssize_t proc_hdmi_power_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    return simple_read_from_buffer(user_buffer, count, offset, hdmi_power_buffer, hdmi_power_buffer_length);
}

ssize_t proc_hdmi_power_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{

    if (count > sizeof(hdmi_power_write_buffer) - 1) {
        return -EINVAL; // Buffer overflow
    }

    if (copy_from_user(hdmi_power_write_buffer, user_buffer, count)) {
        return -EFAULT; // Copy failed
    }

    hdmi_power_write_buffer[count] = '\0'; // Null-terminate the string
    hdmi_power_write_buffer_length = count; // Update buffer length

    // check if "on" or "off"
    if ((strncmp(hdmi_power_write_buffer, "on", 2) == 0) || (strncmp(hdmi_power_write_buffer, "1", 1) == 0)) {
        if (lt86102_pwr_ctrl(1) < 0) {
            return -EIO; // Power control failed
        }
        printk(KERN_INFO "Turning HDMI power on, buffer: %s\n", hdmi_power_buffer);
    } else if ((strncmp(hdmi_power_write_buffer, "off", 3) == 0) || (strncmp(hdmi_power_write_buffer, "0", 1) == 0)) {
        if (lt86102_pwr_ctrl(0) < 0) {
            return -EIO; // Power control failed
        }
        printk(KERN_INFO "Turning HDMI power off, buffer: %s\n", hdmi_power_buffer);
    } else {
        // printk(KERN_INFO "Turning HDMI power error\n");
        return -EINVAL; // Invalid input
    }

    return count; // Return number of bytes written
}

ssize_t proc_loopout_power_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    return simple_read_from_buffer(user_buffer, count, offset, loopout_power_buffer, loopout_power_buffer_length);
}

ssize_t proc_loopout_power_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{

    if (count > sizeof(loopout_power_write_buffer) - 1) {
        return -EINVAL; // Buffer overflow
    }

    if (copy_from_user(loopout_power_write_buffer, user_buffer, count)) {
        return -EFAULT; // Copy failed
    }

    loopout_power_write_buffer[count] = '\0'; // Null-terminate the string
    loopout_power_write_buffer_length = count; // Update buffer length

    // printk(KERN_INFO "Turning HDMI loopout power on, buffer: %s\n", loopout_power_write_buffer);
    // check if "on" or "off"
    if ((strncmp(loopout_power_write_buffer, "on", 2) == 0) || (strncmp(loopout_power_write_buffer, "1", 1) == 0)) {
        if (set_hdmi_loopout(1) < 0) {
            return -EIO; // Power control failed
        }
    } else if ((strncmp(loopout_power_write_buffer, "off", 3) == 0) || (strncmp(loopout_power_write_buffer, "0", 1) == 0)) {
        if (set_hdmi_loopout(0) < 0) {
            return -EIO; // Power control failed
        }
    } else {
        return -EINVAL; // Invalid input
    }

    return count; // Return number of bytes written
}

ssize_t proc_hdmi_status_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    struct lt6911_priv_data *priv = file->private_data;
    ssize_t ret;

    // printk(KERN_INFO "Reading from proc file\n");
    ret = simple_read_from_buffer(user_buffer, count, offset, hdmi_status_buffer, hdmi_status_buffer_length);

    if (ret > 0 && priv) {
        priv->last_status = atomic_read(&lt6911_ctx.status);
    }

    return ret;
}

ssize_t proc_hdmi_status_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{
    if (count > sizeof(hdmi_status_write_buffer) - 1) {
        return -EINVAL; // Buffer overflow
    }

    if (copy_from_user(hdmi_status_write_buffer, user_buffer, count)) {
        return -EFAULT; // Copy failed
    }

    // check input is "ok"
    if (strncmp(hdmi_status_write_buffer, "ok", 2) == 0) {
        update_status();
    }

    return count; // Return number of bytes written
}

unsigned int proc_hdmi_status_poll(struct file *file, poll_table *wait)
{
    struct lt6911_priv_data *priv = file->private_data;
    unsigned int mask = 0;

    poll_wait(file, &lt6911_ctx.wait_queue, wait);

    if (!priv) return POLLERR;

    if (atomic_read(&lt6911_ctx.status) != priv->last_status) {
        mask |= POLLPRI;
    }

    return mask;
}

ssize_t proc_hdmi_width_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    // update_status(READ_WIDTH_FLAG);
    return simple_read_from_buffer(user_buffer, count, offset, hdmi_width_buffer, hdmi_width_buffer_length);
}

ssize_t proc_hdmi_height_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    // update_status(READ_HEIGHT_FLAG);
    return simple_read_from_buffer(user_buffer, count, offset, hdmi_height_buffer, hdmi_height_buffer_length);
}

ssize_t proc_hdmi_fps_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    return simple_read_from_buffer(user_buffer, count, offset, hdmi_fps_buffer, hdmi_fps_buffer_length);
}

ssize_t proc_hdmi_hdcp_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    return simple_read_from_buffer(user_buffer, count, offset, hdmi_hdcp_buffer, hdmi_hdcp_buffer_length);
}

ssize_t proc_audio_sample_rate_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // printk(KERN_INFO "Reading from proc file\n");
    return simple_read_from_buffer(user_buffer, count, offset, audio_sample_rate_buffer, audio_sample_rate_buffer_length);
}

ssize_t proc_hdmi_rx_status_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    struct lt6911_priv_data *priv = file->private_data;
    ssize_t ret;

    // printk(KERN_INFO "Reading from proc file\n");
    ret = simple_read_from_buffer(user_buffer, count, offset, hdmi_rx_status_buffer, hdmi_rx_status_buffer_length);

    if (ret > 0 && priv) {
        priv->last_rx_status = atomic_read(&lt6911_ctx.rx_status);
    }

    return ret;
}

unsigned int proc_hdmi_rx_status_poll(struct file *file, poll_table *wait)
{
    struct lt6911_priv_data *priv = file->private_data;
    unsigned int mask = 0;

    poll_wait(file, &lt6911_ctx.wait_queue, wait);

    if (!priv) return POLLERR;

    if (atomic_read(&lt6911_ctx.rx_status) != priv->last_rx_status) {
        mask |= POLLPRI;
    }

    return mask;
}

ssize_t proc_hdmi_tx_status_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    struct lt6911_priv_data *priv = file->private_data;
    ssize_t ret;

    // printk(KERN_INFO "Reading from proc file\n");
    ret = simple_read_from_buffer(user_buffer, count, offset, hdmi_tx_status_buffer, hdmi_tx_status_buffer_length);

    if (ret > 0 && priv) {
        priv->last_tx_status = atomic_read(&lt6911_ctx.tx_status);
    }

    return ret;
}

ssize_t proc_hdmi_tx_status_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{
    // check input is "0"/"off" or "1"/"on" then ctrl HDMI TXO_PIN
    if (strncmp(user_buffer, "0", 1) == 0 || strncmp(user_buffer, "off", 3) == 0) {
        set_hdmi_loopout(0);
    } else if (strncmp(user_buffer, "1", 1) == 0 || strncmp(user_buffer, "on", 2) == 0) {
        set_hdmi_loopout(1);
    } else {
        return -EINVAL;
    }

    return count;
}

unsigned int proc_hdmi_tx_status_poll(struct file *file, poll_table *wait)
{
    struct lt6911_priv_data *priv = file->private_data;
    unsigned int mask = 0;

    poll_wait(file, &lt6911_ctx.wait_queue, wait);

    if (!priv) return POLLERR;

    if (atomic_read(&lt6911_ctx.tx_status) != priv->last_tx_status) {
        mask |= POLLPRI;
    }

    return mask;
}

static ktime_t last_read_time = 0;
ssize_t proc_hdmi_edid_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    // Get the current time
    ktime_t current_time = ktime_get_real();
    s64 interval = ktime_to_ms(ktime_sub(current_time, last_read_time));

    // Check if the interval since the last read is greater than 500 ms
    if (interval > 500) {
        // Ensure that the chip is in the on state
        // Power on the LT6911UXC first
        lt6911_pwr_ctrl(1);

        // read EDID from chip
        hdmi_edid_buffer_length = EDID_BUFFER_SIZE;
        printk(KERN_INFO "Reading EDID from LT6911UXC...\n");
        if (lt6911_edid_read(hdmi_edid_buffer, hdmi_edid_buffer_length) < 0) {
            return -EIO; // EDID read failed
        }

        // copy to hdmi_edid_snapshot_buffer
        memcpy(hdmi_edid_snapshot_buffer, hdmi_edid_buffer, hdmi_edid_buffer_length);
        hdmi_edid_snapshot_buffer_length = hdmi_edid_buffer_length;

        // restart the chip
        printk(KERN_INFO "Restarting LT6911UXC...\n");
        lt6911_pwr_ctrl(0);
        msleep(100);
        lt6911_pwr_ctrl(1);
    }

    // Update the last read time
    last_read_time = current_time;

    return simple_read_from_buffer(user_buffer, count, offset, hdmi_edid_buffer, hdmi_edid_buffer_length);
}

ssize_t proc_hdmi_edid_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{
    // copy to edid_write_buffer
    int i;
    u8 edid_write_buffer[count]; // 256 bytes for EDID or +32 bytes for EDID name
    u8 edid_read_buffer[count]; // 256 bytes for EDID or +32 bytes for EDID name
    if (copy_from_user(edid_write_buffer, user_buffer, count)) {
        return -EFAULT;
    }

    // print buffer by hex
    printk(KERN_INFO "Writing EDID: ");
    for (i = 0; i < count; i++) {
        printk(KERN_CONT "%02x ", edid_write_buffer[i]);
    }
    printk(KERN_CONT "\n");

    // check edid_write_buffer
    if (check_edid(edid_write_buffer, count) < 0) {
        return -EINVAL; // Invalid EDID data
    }

    // Ensure that the chip is in the on state
    // Power on the LT6911UXC first
    lt6911_pwr_ctrl(1);

    // write EDID to chip
    if (lt6911_edid_write(edid_write_buffer, count) < 0) {
        return -EIO; // EDID write failed
    }

    // read EDID from chip
    if (lt6911_edid_read(edid_read_buffer, count) < 0) {
        return -EIO; // EDID read failed
    }

    // copy to hdmi_edid_snapshot_buffer
    memcpy(hdmi_edid_snapshot_buffer, edid_read_buffer, count);
    hdmi_edid_snapshot_buffer_length = count;

    // check if read EDID is same as write EDID
    if (memcmp(edid_write_buffer, edid_read_buffer, count) != 0) {
        printk(KERN_ERR "EDID write verification failed\n");
        return -EIO; // EDID write verification failed
    }

    // restart the chip
    printk(KERN_INFO "Restarting LT6911UXC...\n");
    lt6911_pwr_ctrl(0);
    msleep(100);
    lt6911_pwr_ctrl(1);

    return count;
}

ssize_t proc_hdmi_edid_snapshot_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    if (hdmi_edid_snapshot_buffer_length == EDID_BUFFER_SIZE) {
        snprintf(hdmi_edid_snapshot_buffer, sizeof(hdmi_edid_snapshot_buffer), "unknown\n");
    }
    return simple_read_from_buffer(user_buffer, count, offset, hdmi_edid_snapshot_buffer, hdmi_edid_snapshot_buffer_length);
}

ssize_t proc_version_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    if (hdmi_version_buffer[0] == '\0') {
        snprintf(hdmi_version_buffer, sizeof(hdmi_version_buffer), "unknown\n");
    }
    hdmi_version_buffer_length = strlen(hdmi_version_buffer);
    return simple_read_from_buffer(user_buffer, count, offset, hdmi_version_buffer, hdmi_version_buffer_length);
}

ssize_t proc_version_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{
    // copy to version_write_buffer
    u8 version_write_buffer[count]; // 256 bytes for EDID or +32 bytes for EDID name
    if (copy_from_user(version_write_buffer, user_buffer, count)) {
        return -EFAULT;
    }

    // print buffer by hex
    printk(KERN_INFO "Writing version: ");
    printk(KERN_CONT "%s\n", version_write_buffer);

    // Ensure that the chip is in the on state
    // Power on the LT6911UXC first
    lt6911_pwr_ctrl(1);

    // write version to chip
    if (lt6911_str_write(version_write_buffer, count) < 0) {
        return -EIO; // version write failed
    }

    // read version from chip
    if (lt6911_str_read(hdmi_version_buffer) < 0) {
        return -EIO; // version read failed
    }

    // check if read version is same as write version
    if (memcmp(version_write_buffer, hdmi_version_buffer, count) != 0) {
        printk(KERN_ERR "version write verification failed\n");
        return -EIO; // version write verification failed
    }

    // restart the chip
    printk(KERN_INFO "Restarting LT6911UXC...\n");
    lt6911_pwr_ctrl(0);
    msleep(100);
    lt6911_pwr_ctrl(1);

    return count;
}

static const char *lt6911_chip_id_name(void)
{
    switch (chip_platform) {
    case LT6911_CHIP_LT6911C:
        return "lt6911c\n";
    case LT6911_CHIP_LT6911D:
        return "lt6911d\n";
    case LT6911_CHIP_LT6911UXC:
        return "lt6911uxc\n";
    case LT6911_CHIP_UNKNOWN:
    default:
        return "unknown\n";
    }
}

ssize_t proc_chip_id_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    const char *chip_id = lt6911_chip_id_name();

    return simple_read_from_buffer(user_buffer, count, offset, chip_id, strlen(chip_id));
}

static int proc_open(struct inode *inode, struct file *file)
{
    struct lt6911_priv_data *priv;

    if (!try_module_get(THIS_MODULE))
        return -ENODEV;

    priv = kzalloc(sizeof(struct lt6911_priv_data), GFP_KERNEL);
    if (!priv) {
        module_put(THIS_MODULE);
        return -ENOMEM;
    }

    priv->last_status = atomic_read(&lt6911_ctx.status);
    priv->last_rx_status = atomic_read(&lt6911_ctx.rx_status);
    priv->last_tx_status = atomic_read(&lt6911_ctx.tx_status);

    file->private_data = priv;
    return 0;
}

static int proc_release(struct inode *inode, struct file *file)
{
    struct lt6911_priv_data *priv = file->private_data;
    if (priv)
        kfree(priv);

    module_put(THIS_MODULE);
    return 0;
}

static const struct file_operations proc_csi_power_fops = {
    .owner = THIS_MODULE,
    .read = proc_csi_power_read,
    .write = proc_csi_power_write,
};

static const struct file_operations proc_hdmi_power_fops = {
    .owner = THIS_MODULE,
    .read = proc_hdmi_power_read,
    .write = proc_hdmi_power_write,
};

static const struct file_operations proc_loopout_power_fops = {
    .owner = THIS_MODULE,
    .read = proc_loopout_power_read,
    .write = proc_loopout_power_write,
};

static const struct file_operations proc_hdmi_status_fops = {
    .owner = THIS_MODULE,
    .open    = proc_open,
    .release = proc_release,
    .read = proc_hdmi_status_read,
    .write = proc_hdmi_status_write,
    .poll = proc_hdmi_status_poll,
};

static const struct file_operations proc_hdmi_width_fops = {
    .owner = THIS_MODULE,
    .read = proc_hdmi_width_read,
};

static const struct file_operations proc_hdmi_height_fops = {
    .owner = THIS_MODULE,
    .read = proc_hdmi_height_read,
};

static const struct file_operations proc_hdmi_fps_fops = {
    .owner = THIS_MODULE,
    .read = proc_hdmi_fps_read,
};

static const struct file_operations proc_hdmi_hdcp_fops = {
    .owner = THIS_MODULE,
    .read = proc_hdmi_hdcp_read,
};

static const struct file_operations proc_audio_sample_rate_fops = {
    .owner = THIS_MODULE,
    .read = proc_audio_sample_rate_read,
};

static const struct file_operations proc_hdmi_rx_status_fops = {
    .owner = THIS_MODULE,
    .open    = proc_open,
    .release = proc_release,
    .read = proc_hdmi_rx_status_read,
    .poll = proc_hdmi_rx_status_poll,
};

static const struct file_operations proc_hdmi_tx_status_fops = {
    .owner = THIS_MODULE,
    .open    = proc_open,
    .release = proc_release,
    .read = proc_hdmi_tx_status_read,
    .write = proc_hdmi_tx_status_write,
    .poll = proc_hdmi_tx_status_poll,
};

static const struct file_operations proc_hdmi_edid_fops = {
    .owner = THIS_MODULE,
    .read = proc_hdmi_edid_read,
    .write = proc_hdmi_edid_write,
};

static const struct file_operations proc_hdmi_edid_snapshot_fops = {
    .owner = THIS_MODULE,
    .read = proc_hdmi_edid_snapshot_read,
};

static const struct file_operations proc_version_fops = {
    .owner = THIS_MODULE,
    .read = proc_version_read,
    .write = proc_version_write,
};

static const struct file_operations proc_chip_id_fops = {
    .owner = THIS_MODULE,
    .read = proc_chip_id_read,
};

void proc_buffer_init(void)
{
    // hdmi_status_buffer = "disappear"
    snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "disappear\n");
    hdmi_status_buffer_length = strlen(hdmi_status_buffer);
    // hdmi_width_buffer = "0"
    snprintf(hdmi_width_buffer, sizeof(hdmi_width_buffer), "0\n");
    hdmi_width_buffer_length = strlen(hdmi_width_buffer);
    // hdmi_height_buffer = "0"
    snprintf(hdmi_height_buffer, sizeof(hdmi_height_buffer), "0\n");
    hdmi_height_buffer_length = strlen(hdmi_height_buffer);
    // hdmi_fps_buffer = "0"
    snprintf(hdmi_fps_buffer, sizeof(hdmi_fps_buffer), "0\n");
    hdmi_fps_buffer_length = strlen(hdmi_fps_buffer);
    // hdmi_hdcp_buffer = "0"
    snprintf(hdmi_hdcp_buffer, sizeof(hdmi_hdcp_buffer), "no hdcp\n");
    hdmi_hdcp_buffer_length = strlen(hdmi_hdcp_buffer);
    // audio_sample_rate_buffer = "0"
    snprintf(audio_sample_rate_buffer, sizeof(audio_sample_rate_buffer), "disappear\n");
    audio_sample_rate_buffer_length = strlen(audio_sample_rate_buffer);
}

int proc_info_init(void)
{
    // create proc dir
    proc_lt6911_dir = proc_mkdir(PROC_LT6911_DIR, NULL);
    if (!proc_lt6911_dir) {
        goto err;
    }

    // create proc chip id file
    proc_chip_id_file = proc_create(PROC_CHIP_ID, 0444, proc_lt6911_dir, &proc_chip_id_fops); // R
    if (!proc_chip_id_file) {
        goto err;
    }

    // create proc pwr ctrl files
    proc_csi_power_file = proc_create(PROC_CSI_PWR, 0666, proc_lt6911_dir, &proc_csi_power_fops);    // W&R
    if (!proc_csi_power_file) {
        goto err;
    }

    // create proc hdmi pwr ctrl files
    proc_hdmi_power_file = proc_create(PROC_HDMI_PWR, 0666, proc_lt6911_dir, &proc_hdmi_power_fops);    // W&R
    if (!proc_hdmi_power_file) {
        goto err;
    }

    // create proc hdmi loopout pwr ctrl files
    proc_loopout_power_file = proc_create(PROC_LOOPOUT_PWR, 0666, proc_lt6911_dir, &proc_loopout_power_fops);    // W&R
    if (!proc_loopout_power_file) {
        goto err;
    }

    // create proc hdmi info files
    proc_hdmi_status_file = proc_create(PROC_HDMI_STATUS, 0666, proc_lt6911_dir, &proc_hdmi_status_fops); // W&R
    if (!proc_hdmi_status_file) {
        goto err;
    }

    // create proc hdmi width and height files
    proc_hdmi_width_file = proc_create(PROC_HDMI_WIDTH, 0444, proc_lt6911_dir, &proc_hdmi_width_fops); // R
    if (!proc_hdmi_width_file) {
        goto err;
    }

    // create proc hdmi height file
    proc_hdmi_height_file = proc_create(PROC_HDMI_HEIGHT, 0444, proc_lt6911_dir, &proc_hdmi_height_fops); // R
    if (!proc_hdmi_height_file) {
        goto err;
    }

    // create proc hdmi fps file
    proc_hdmi_fps_file = proc_create(PROC_HDMI_FPS, 0444, proc_lt6911_dir, &proc_hdmi_fps_fops); // R
    if (!proc_hdmi_fps_file) {
        goto err;
    }

    // create proc hdmi hdcp file
    proc_hdmi_hdcp_file = proc_create(PROC_HDMI_HDCP, 0444, proc_lt6911_dir, &proc_hdmi_hdcp_fops); // R
    if (!proc_hdmi_hdcp_file) {
        goto err;
    }

    // create proc audio sample rate file
    proc_audio_sample_rate_file = proc_create(PROC_AUDIO_SAMPLE_RATE, 0444, proc_lt6911_dir, &proc_audio_sample_rate_fops); // R
    if (!proc_audio_sample_rate_file) {
        goto err;
    }

    // create proc hdmi rx status file
    proc_hdmi_rx_status_file = proc_create(PROC_HDMI_RX_STATUS, 0444, proc_lt6911_dir, &proc_hdmi_rx_status_fops); // R
    if (!proc_hdmi_rx_status_file) {
        goto err;
    }

    // create proc hdmi tx status file
    /*
    Temporarily retain the write function to maintain compatibility with legacy applications.
    */
    proc_hdmi_tx_status_file = proc_create(PROC_HDMI_TX_STATUS, 0666, proc_lt6911_dir, &proc_hdmi_tx_status_fops); // W&R
    if (!proc_hdmi_tx_status_file) {
        goto err;
    }

    // create proc hdmi edid file
    proc_hdmi_edid_file = proc_create(PROC_HDMI_EDID, 0666, proc_lt6911_dir, &proc_hdmi_edid_fops); // W&R
    if (!proc_hdmi_edid_file) {
        goto err;
    }

    // create proc hdmi edid snapshot file
    proc_hdmi_edid_snapshot_file = proc_create(PROC_HDMI_EDID_SNAPSHOT, 0444, proc_lt6911_dir, &proc_hdmi_edid_snapshot_fops); // R
    if (!proc_hdmi_edid_snapshot_file) {
        goto err;
    }

    // create proc version file
    proc_version_file = proc_create(PROC_VERSION, 0666, proc_lt6911_dir, &proc_version_fops); // W&R
    if (!proc_version_file) {
        goto err;
    }

    return 0;

err:
    printk(KERN_ERR "Failed to create proc entries\n");
    if (proc_hdmi_tx_status_file) remove_proc_entry(PROC_HDMI_TX_STATUS, proc_lt6911_dir);
    if (proc_hdmi_rx_status_file) remove_proc_entry(PROC_HDMI_RX_STATUS, proc_lt6911_dir);
    if (proc_audio_sample_rate_file) remove_proc_entry(PROC_AUDIO_SAMPLE_RATE, proc_lt6911_dir);
    if (proc_hdmi_hdcp_file) remove_proc_entry(PROC_HDMI_HDCP, proc_lt6911_dir);
    if (proc_hdmi_fps_file) remove_proc_entry(PROC_HDMI_FPS, proc_lt6911_dir);
    if (proc_hdmi_width_file) remove_proc_entry(PROC_HDMI_WIDTH, proc_lt6911_dir);
    if (proc_hdmi_height_file) remove_proc_entry(PROC_HDMI_HEIGHT, proc_lt6911_dir);
    if (proc_hdmi_status_file) remove_proc_entry(PROC_HDMI_STATUS, proc_lt6911_dir);
    if (proc_hdmi_power_file) remove_proc_entry(PROC_HDMI_PWR, proc_lt6911_dir);
    if (proc_csi_power_file) remove_proc_entry(PROC_CSI_PWR, proc_lt6911_dir);
    if (proc_loopout_power_file) remove_proc_entry(PROC_LOOPOUT_PWR, proc_lt6911_dir);
    if (proc_hdmi_edid_file) remove_proc_entry(PROC_HDMI_EDID, proc_lt6911_dir);
    if (proc_hdmi_edid_snapshot_file) remove_proc_entry(PROC_HDMI_EDID_SNAPSHOT, proc_lt6911_dir);
    if (proc_version_file) remove_proc_entry(PROC_VERSION, proc_lt6911_dir);
    if (proc_chip_id_file) remove_proc_entry(PROC_CHIP_ID, proc_lt6911_dir);

    if (proc_lt6911_dir) remove_proc_entry(PROC_LT6911_DIR, NULL);
    return -1;
}

int proc_info_exit(void)
{
    if (proc_hdmi_tx_status_file) remove_proc_entry(PROC_HDMI_TX_STATUS, proc_lt6911_dir);
    if (proc_hdmi_rx_status_file) remove_proc_entry(PROC_HDMI_RX_STATUS, proc_lt6911_dir);
    if (proc_audio_sample_rate_file) remove_proc_entry(PROC_AUDIO_SAMPLE_RATE, proc_lt6911_dir);
    if (proc_hdmi_hdcp_file) remove_proc_entry(PROC_HDMI_HDCP, proc_lt6911_dir);
    if (proc_hdmi_fps_file) remove_proc_entry(PROC_HDMI_FPS, proc_lt6911_dir);
    if (proc_hdmi_width_file) remove_proc_entry(PROC_HDMI_WIDTH, proc_lt6911_dir);
    if (proc_hdmi_height_file) remove_proc_entry(PROC_HDMI_HEIGHT, proc_lt6911_dir);
    if (proc_hdmi_status_file) remove_proc_entry(PROC_HDMI_STATUS, proc_lt6911_dir);
    if (proc_hdmi_power_file) remove_proc_entry(PROC_HDMI_PWR, proc_lt6911_dir);
    if (proc_csi_power_file) remove_proc_entry(PROC_CSI_PWR, proc_lt6911_dir);
    if (proc_loopout_power_file) remove_proc_entry(PROC_LOOPOUT_PWR, proc_lt6911_dir);
    if (proc_hdmi_edid_file) remove_proc_entry(PROC_HDMI_EDID, proc_lt6911_dir);
    if (proc_hdmi_edid_snapshot_file) remove_proc_entry(PROC_HDMI_EDID_SNAPSHOT, proc_lt6911_dir);
    if (proc_version_file) remove_proc_entry(PROC_VERSION, proc_lt6911_dir);
    if (proc_chip_id_file) remove_proc_entry(PROC_CHIP_ID, proc_lt6911_dir);

    if (proc_lt6911_dir) remove_proc_entry(PROC_LT6911_DIR, NULL);
    return 0;
}

void update_interface_status_buffer(void)
{
    int value;

    if (HDMI_RXI_PIN >= 0) {
        value = gpio_get_value(HDMI_RXI_PIN);
        if (value == 0) {
            snprintf(hdmi_rx_status_buffer, sizeof(hdmi_rx_status_buffer), "access\n");
        } else {
            snprintf(hdmi_rx_status_buffer, sizeof(hdmi_rx_status_buffer), "extract\n");
        }
        hdmi_rx_status_buffer_length = strlen(hdmi_rx_status_buffer);
        atomic_inc(&lt6911_ctx.rx_status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);
    } else {
        snprintf(hdmi_rx_status_buffer, sizeof(hdmi_rx_status_buffer), "unsupport\n");
        hdmi_rx_status_buffer_length = strlen(hdmi_rx_status_buffer);
        atomic_inc(&lt6911_ctx.rx_status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);
    }

    if (HDMI_TXI_PIN >= 0) {
        value = gpio_get_value(HDMI_TXI_PIN);
        if (value == 0) {
            snprintf(hdmi_tx_status_buffer, sizeof(hdmi_tx_status_buffer), "access\n");
        } else {
            snprintf(hdmi_tx_status_buffer, sizeof(hdmi_tx_status_buffer), "extract\n");
        }
        hdmi_tx_status_buffer_length = strlen(hdmi_tx_status_buffer);
        atomic_inc(&lt6911_ctx.tx_status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);
    } else {
        snprintf(hdmi_tx_status_buffer, sizeof(hdmi_tx_status_buffer), "unsupport\n");
        hdmi_tx_status_buffer_length = strlen(hdmi_tx_status_buffer);
        atomic_inc(&lt6911_ctx.tx_status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);
    }
}

static void interface_change_handler(struct work_struct *work)
{
    update_interface_status_buffer();
}

static irqreturn_t interface_irq_handler(int irq, void *dev_id)
{
    schedule_delayed_work(&irq_ctx.work, msecs_to_jiffies(50));
    return IRQ_HANDLED;
}

int lt6911_pwr_ctrl(int pwr_en)
{
    int ret;

    if (pwr_en) {
        // Power on the LT6911UXC
        ret = gpio_direction_output(PWR_PIN, 1);
        if (ret < 0) {
            printk(KERN_ERR "Failed to set GPIO %d to high\n", PWR_PIN);
            return -1;
        }
        // update the proc file
        snprintf(csi_power_buffer, sizeof(csi_power_buffer), "on\n");
        csi_power_buffer_length = strlen(csi_power_buffer);
    } else {
        // Power off the LT6911UXC
        ret = gpio_direction_output(PWR_PIN, 0);
        if (ret < 0) {
            printk(KERN_ERR "Failed to set GPIO %d to low\n", PWR_PIN);
            return -1;
        }
        // update the proc file
        snprintf(csi_power_buffer, sizeof(csi_power_buffer), "off\n");
        csi_power_buffer_length = strlen(csi_power_buffer);
    }

    return 0;
}

int lt86102_pwr_ctrl(int pwr_en)
{
    int ret;

    if (HDMI_PIN < 0) {
        printk(KERN_ERR "HDMI_PIN is not set, cannot control LT86102UXC power\n");
        return -1;
    }

    if (pwr_en) {
        // Power on the LT86102UXC
        ret = gpio_direction_output(HDMI_PIN, 1);
        if (ret < 0) {
            printk(KERN_ERR "Failed to set GPIO %d to high\n", HDMI_PIN);
            return -1;
        }
        // update the proc file
        snprintf(hdmi_power_buffer, sizeof(hdmi_power_buffer), "on\n");
        hdmi_power_buffer_length = strlen(hdmi_power_buffer);
    } else {
        // Power off the LT86102UXC
        ret = gpio_direction_output(HDMI_PIN, 0);
        if (ret < 0) {
            printk(KERN_ERR "Failed to set GPIO %d to low\n", HDMI_PIN);
            return -1;
        }
        // update the proc file
        snprintf(hdmi_power_buffer, sizeof(hdmi_power_buffer), "off\n");
        hdmi_power_buffer_length = strlen(hdmi_power_buffer);
    }

    return 0;
}

int get_hdmi_interface_state(hdmi_state_t interface)
{
    int value;
    switch (interface)
    {
    case HDMI_RXI_STATE:
        if (HDMI_RXI_PIN < 0) {
            printk(KERN_ERR "Board Vision is not support this interface\n");
            return -1;
        }
        value = gpio_get_value(HDMI_RXI_PIN);
        if (value == 0) {
            printk(KERN_INFO "HDMI RXI Interface is connected\n");
            return 1; // Connected
        } else if (value == 1) {
            printk(KERN_INFO "HDMI RXI Interface is not connected\n");
            return 0; // Not connected
        } else {
            printk(KERN_ERR "Failed to get HDMI RXI state\n");
            return -1;
        }
        break;
    case HDMI_TXI_STATE:
        if (HDMI_TXI_PIN < 0) {
            printk(KERN_ERR "Board Vision is not support this interface\n");
            return -1;
        }
        value = gpio_get_value(HDMI_TXI_PIN);
        if (value == 0) {
            printk(KERN_INFO "HDMI TXI Interface is connected\n");
            return 1; // Connected
        } else if (value == 1) {
            printk(KERN_INFO "HDMI TXI Interface is not connected\n");
            return 0; // Not connected
        } else {
            printk(KERN_ERR "Failed to get HDMI TXI state\n");
            return -1;
        }
        break;

    default:
        return -1;
        break;
    }
    return -1;
}

int set_hdmi_loopout(int enable)
{
    if (HDMI_TXO_PIN < 0 || LOOP_CTRL_PIN < 0) {
        printk(KERN_ERR "HDMI_TXO_PIN or LOOP_CTRL_PIN is not set, cannot control HDMI loopout\n");
        return -1;
    }

    if (enable) {
        // Enable HDMI loopout
        gpio_direction_output(HDMI_TXO_PIN, 0);
        gpio_direction_output(LOOP_CTRL_PIN, 0);
        printk(KERN_INFO "HDMI loopout enabled\n");
        // update the proc file
        snprintf(loopout_power_buffer, sizeof(loopout_power_buffer), "on\n");
        loopout_power_buffer_length = strlen(loopout_power_buffer);
    } else {
        // Disable HDMI loopout
        gpio_direction_output(HDMI_TXO_PIN, 1);
        gpio_direction_output(LOOP_CTRL_PIN, 1);
        printk(KERN_INFO "HDMI loopout disabled\n");
        // update the proc file
        snprintf(loopout_power_buffer, sizeof(loopout_power_buffer), "off\n");
        loopout_power_buffer_length = strlen(loopout_power_buffer);
    }

    return 0;
}

static void write_device_register(unsigned long phys_addr, unsigned int offset, unsigned int value) {
    void __iomem *base;

    //  Map the device register
    base = ioremap(phys_addr, REG_REMAP_SIZE);
    if (!base) {
        printk("Failed to map device memory\n");
        return;
    }

    // Write to the register
    iowrite32(value, base + offset);
    // printk("Wrote 0x%x to register at offset 0x%x\n", value, offset);

    // Unmap the device register
    iounmap(base);
}

void pinmux_register_init(void)
{
    if (board_version == BOARD_VERSION_AX_PI) {
        // Initialize pinmux for AX_Pi
        write_device_register(0x02302000, 0x18, 0x000000C3);    // INT GPIO3_A1
        write_device_register(0x02302000, 0x6C, 0x00060003);    // RST GPIO0_A18
    } else if (board_version == BOARD_VERSION_NanoKVM_PRO) {
        // Initialize pinmux for NanoKVM Pro
        // add pull up
        write_device_register(0x104F0000, 0x6C, 0x00060083);    // INT GPIO1_A28
        write_device_register(0x02300000, 0x48, 0x00060003);    // CSI RST GPIO0_A5
        write_device_register(0x02300000, 0x54, 0x00060003);    // HDMI RST GPIO0_A6
        write_device_register(0x0230A000, 0x6C, 0x00060003);    // HDMI RX Interface
        write_device_register(0x0230A000, 0x78, 0x00060003);    // HDMI TX Interface
        write_device_register(0x02302000, 0x90, 0x00060003);    // HDMI TX CTL Interface
        write_device_register(0x0230A000, 0x60, 0x00060003);    // HDMI LOOP CTRL Interface
    } else {
        printk(KERN_ERR "Unsupported board version\n");
    }
}

int gpio_init(void)
{
    int ret;

    if (board_version == BOARD_VERSION_NanoKVM_PRO) {
        INT_PIN = NANOKVM_PRO_INT_PIN;
        PWR_PIN = NANOKVM_PRO_PWR_PIN;
        HDMI_PIN = NANOKVM_PRO_HDMI_PIN;
        HDMI_RXI_PIN = NANOKVM_PRO_HDMI_RXI_PIN;
        HDMI_TXI_PIN = NANOKVM_PRO_HDMI_TXI_PIN;
        HDMI_TXO_PIN = NANOKVM_PRO_HDMI_TXO_PIN;
        LOOP_CTRL_PIN = NANOKVM_PRO_LOOP_CTRL_PIN;
    } else if (board_version == BOARD_VERSION_AX_PI) {
        INT_PIN = AX_PI_INT_PIN;
        PWR_PIN = AX_PI_PWR_PIN;
        HDMI_PIN = -1; // AX Pi does not use HDMI_PIN
        HDMI_RXI_PIN = -1;
        HDMI_TXI_PIN = -1;
        HDMI_TXO_PIN = -1;
        LOOP_CTRL_PIN = -1;
    } else {
        printk(KERN_ERR "Unsupported board version\n");
        return -1;
    }

    // before init irq
    INIT_DELAYED_WORK(&irq_ctx.work, interface_change_handler);

    // Initialize GPIO for LT6911UXC INT pin
    // init GPIO Interrupt
    if (!gpio_is_valid(INT_PIN)) {
        printk(KERN_ERR "Invalid GPIO pin\n");
        return -1;
    }

    if (gpio_request(INT_PIN, "init_gpio") < 0) {
        printk(KERN_ERR "GPIO request failed\n");
        return -1;
    }

    if ((irq_number = gpio_to_irq(INT_PIN)) < 0) {
        printk(KERN_ERR "Unable to get IRQ number\n");
        gpio_free(INT_PIN);
        return -1;
    }

    ret = request_irq(irq_number, gpio_irq_handler,
                     Int_Action, "init_gpio_irq", NULL);
    if (ret) {
        printk(KERN_ERR "Unable to request IRQ\n");
        gpio_free(INT_PIN);
        return ret;
    }

    // Initialize GPIO for LT6911UXC PWR pin
    ret = gpio_request(PWR_PIN, "LT6911UXC_PWR");
    if (ret) {
        printk(KERN_ERR "Failed to request GPIO %d for LT6911UXC PWR pin\n", PWR_PIN);
        gpio_free(INT_PIN);
        return ret;
    }

    if (HDMI_PIN >= 0) {
        // Initialize GPIO for LT86102UXC HDMI pin
        ret = gpio_request(HDMI_PIN, "LT86102UXC_HDMI_PWR");
        if (ret) {
            printk(KERN_ERR "Failed to request GPIO %d for LT86102UXC HDMI pin\n", HDMI_PIN);
            gpio_free(PWR_PIN);
            gpio_free(INT_PIN);
            return ret;
        }
    }

    update_interface_status_buffer();
    if (HDMI_RXI_PIN >= 0) {
        // Initialize GPIO for HDMI RX Interface pin
        ret = gpio_request(HDMI_RXI_PIN, "LT86102UXC_HDMI_RXI");
        ret = gpio_direction_input(HDMI_RXI_PIN);
        if (ret) {
            printk(KERN_ERR "Failed to request GPIO %d for HDMI RX Interface pin\n", HDMI_RXI_PIN);
            gpio_free(PWR_PIN);
            gpio_free(INT_PIN);
            gpio_free(HDMI_PIN);
            return ret;
        }

        irq_ctx.rxi_irq = gpio_to_irq(HDMI_RXI_PIN);
        if (irq_ctx.rxi_irq >= 0) {
            ret = request_irq(irq_ctx.rxi_irq, interface_irq_handler,
                              IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                              "hdmi_rxi_irq", NULL);
            if (ret) {
                printk(KERN_ERR "Unable to request HDMI RXI IRQ\n");
                irq_ctx.rxi_irq = -1;
            }
        }
    }

    if (HDMI_TXI_PIN >= 0) {
        // Initialize GPIO for HDMI TX Interface pin
        ret = gpio_request(HDMI_TXI_PIN, "LT86102UXC_HDMI_TXI");
        ret = gpio_direction_input(HDMI_TXI_PIN);
        if (ret) {
            printk(KERN_ERR "Failed to request GPIO %d for HDMI TX Interface pin\n", HDMI_TXI_PIN);
            gpio_free(PWR_PIN);
            gpio_free(INT_PIN);
            gpio_free(HDMI_PIN);
            gpio_free(HDMI_RXI_PIN);
            return ret;
        }

        irq_ctx.txi_irq = gpio_to_irq(HDMI_TXI_PIN);
        if (irq_ctx.txi_irq >= 0) {
            ret = request_irq(irq_ctx.txi_irq, interface_irq_handler,
                              IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                              "hdmi_txi_irq", NULL);
            if (ret) {
                printk(KERN_ERR "Unable to request HDMI TXI IRQ\n");
                irq_ctx.txi_irq = -1;
            }
        }
    }

    if (HDMI_TXO_PIN >= 0) {
        // Initialize GPIO for HDMI TX Control pin
        ret = gpio_request(HDMI_TXO_PIN, "LT86102UXC_HDMI_TXO");
        if (ret) {
            printk(KERN_ERR "Failed to request GPIO %d for HDMI TX Control pin\n", HDMI_TXO_PIN);
            gpio_free(PWR_PIN);
            gpio_free(INT_PIN);
            gpio_free(HDMI_PIN);
            gpio_free(HDMI_RXI_PIN);
            gpio_free(HDMI_TXI_PIN);
            return ret;
        }
        // default allow HDMI LOOP OUT
        set_hdmi_loopout(1);
    }

    if (LOOP_CTRL_PIN >= 0) {
        // Initialize GPIO for HDMI LOOP CTRL pin
        ret = gpio_request(LOOP_CTRL_PIN, "LT86102UXC_LOOP_CTRL");
        if (ret) {
            printk(KERN_ERR "Failed to request GPIO %d for HDMI LOOP CTRL pin\n", LOOP_CTRL_PIN);
            gpio_free(PWR_PIN);
            gpio_free(INT_PIN);
            gpio_free(HDMI_PIN);
            gpio_free(HDMI_RXI_PIN);
            gpio_free(HDMI_TXI_PIN);
            gpio_free(HDMI_TXO_PIN);
            return ret;
        }
    }

    pinmux_register_init();

    // Setup for version detect
    lt86102_pwr_ctrl(1); // Power on the LT86102UXC first
    lt6911_pwr_ctrl(1); // Power on the LT6911UXC

    return 0;
}

int gpio_exit(void)
{
    // Free the IRQ and GPIO resources
    free_irq(irq_number, NULL);
    if (irq_ctx.rxi_irq >= 0)
        free_irq(irq_ctx.rxi_irq, NULL);
    if (irq_ctx.txi_irq >= 0)
        free_irq(irq_ctx.txi_irq, NULL);

    cancel_delayed_work_sync(&irq_ctx.work);

    gpio_free(INT_PIN);
    gpio_free(PWR_PIN);
    if (HDMI_PIN >= 0) gpio_free(HDMI_PIN);
    if (HDMI_RXI_PIN >= 0) gpio_free(HDMI_RXI_PIN);
    if (HDMI_TXI_PIN >= 0) gpio_free(HDMI_TXI_PIN);
    if (HDMI_TXO_PIN >= 0) gpio_free(HDMI_TXO_PIN);
    if (LOOP_CTRL_PIN >= 0) gpio_free(LOOP_CTRL_PIN);

    return 0;
}

/* return 0 : normal res;
 * return 1 : new res;
 * return 2 : unsupport res;
 * return 3 : unknow res;
 */
u8 check_res(u16 _width, u16 _height)
{
    u8 i;
	static u16 old_width = 0xFFFF;
	static u16 old_height = 0xFFFF;

    for(i = 0; i < sizeof(hdmi_res_list)/4; i++){
        // printk(KERN_INFO "hdmi_res_list[%d] = %dx%d\n", i, hdmi_res_list[i][0], hdmi_res_list[i][1]);
        if (_width == hdmi_res_list[i][0] && _height == hdmi_res_list[i][1]) {
            if (old_width != _width || old_height != _height) {
                old_width = _width;
                old_height = _height;
                return NEW_RES;
            }
            return NORMAL_RES;
        }
    }
    for(i = 0; i < sizeof(hdmi_unsupported_res_list)/4; i++){
        if (_width == hdmi_unsupported_res_list[i][0] && _height == hdmi_unsupported_res_list[i][1]) return UNSUPPORT_RES;
    }
    return UNKNOWN_RES;
}

// check if the I2C device exists
bool i2c_device_exists(u8 device_address) {
    struct i2c_client *f_client;
    struct i2c_adapter *f_adapter;
    bool exists = false;
    int ret;

    // 获取 I2C 适配器
    f_adapter = i2c_get_adapter(I2C_BUS);
    if (!f_adapter) {
        printk("Failed to get I2C adapter\n");
        return false;
    }

    // 创建 I2C 客户端
    f_client = i2c_new_dummy(f_adapter, device_address);
    if (!f_client) {
        printk(KERN_ERR "Failed to create I2C device\n");
        return -ENODEV;
    }

    // 尝试读取设备
    ret = i2c_smbus_read_byte(f_client);
    exists = (ret >= 0);  // 如果返回值非负，设备存在

    // 释放资源
    i2c_put_adapter(f_adapter);
    i2c_unregister_device(f_client);

    return exists;
}

void board_version_check(void)
{
#ifdef CHECK_BOARD_VERSION
    // check board version
    if (i2c_device_exists(LT86102UXE_ADDR)) {
        // NanoKVM Pro
        board_version = BOARD_VERSION_NanoKVM_PRO;
        printk(KERN_INFO "Board: NanoKVM_PRO\n");
    } else {
        // AX PI
        board_version = BOARD_VERSION_AX_PI;
        printk(KERN_INFO "Board: AX_PI\n");
    }
#else
    // default board version
    board_version = BOARD_VERSION_NanoKVM_PRO;
    printk(KERN_INFO "Board: NanoKVM_PRO\n");
#endif
}

// I2C write function
int i2c_write_byte(u8 offset, u8 reg, u8 data)
{
    int ret;

    // if offset is changed, write it first
    if (offset != old_offset) {
        ret = i2c_smbus_write_byte_data(client, LT6911_REG_OFFSET, offset);
        if (ret < 0) {
            printk(KERN_ERR "[W]Failed to write offset to the i2c bus; ret = %d\n", ret);
            return -1;
        }
        old_offset = offset;
    }

    // write the data to the i2c bus
    ret = i2c_smbus_write_byte_data(client, reg, data);
    if (ret < 0) {
        printk(KERN_ERR "[W]Failed to write data to the i2c bus; ret = %d\n", ret);
        return -1;
    }

    return 0;
}

// I2C read function
int i2c_read_byte(u8 offset, u8 reg, u8 *data)
{
    int ret;

    // if offset is changed, write it first
    if (offset != old_offset) {
        ret = i2c_smbus_write_byte_data(client, LT6911_REG_OFFSET, offset);
        if (ret < 0) {
            printk(KERN_ERR "[W]Failed to write offset to the i2c bus; ret = %d\n", ret);
            return -1;
        }
        old_offset = offset;
    }

    ret = i2c_smbus_read_byte_data(client, reg);
    if (ret < 0) {
        printk(KERN_ERR "[R]Failed to read data from the i2c bus; ret = %d\n", ret);
        return -1;
    }

    // ret > 0
    *data = (u8)ret;

    return 0;
}

// #define i2c_wr_single_func
#ifdef i2c_wr_single_func
// I2C write function
int i2c_write_bytes(u8 offset, u8 reg, u8 *data, u16 len)
{
    u8 buf[2] = {0};
    int ret;
    u16 count;

    // if offset is changed, write it first
    if (offset != old_offset) {
        ret = i2c_smbus_write_byte_data(client, LT6911_REG_OFFSET, offset);
        if (ret < 0) {
            printk(KERN_ERR "[W]Failed to write offset to the i2c bus; ret = %d\n", ret);
            return -1;
        }
        old_offset = offset;
    }

    // write the data to the i2c bus
    for (count = 0; count < len; count ++){
        ret = i2c_smbus_write_byte_data(client, reg, *(data+count));
        if (ret < 0) {
            printk(KERN_ERR "[W]Failed to write data to the i2c bus; ret = %d\n", ret);
            return -1;
        }
    }

    return 0;
}

// I2C read function
int i2c_read_bytes(u8 offset, u8 reg, u8 *data, u16 len)
{
    int ret;
    u16 count;

    // if offset is changed, write it first
    if (offset != old_offset) {
        ret = i2c_smbus_write_byte_data(client, LT6911_REG_OFFSET, offset);
        if (ret < 0) {
            printk(KERN_ERR "[W]Failed to write offset to the i2c bus; ret = %d\n", ret);
            return -1;
        }
        old_offset = offset;
    }

    // read the data from the i2c bus
    for (count = 0; count < len; count ++) {
        ret = i2c_smbus_read_byte_data(client, reg);
        if (ret < 0) {
            printk(KERN_ERR "[R]Failed to read data from the i2c bus; ret = %d\n", ret);
            return -1;
        }   // ret > 0
        *(data+count) = (u8)ret;
    }

    return 0;
}
#else

// I2C write function
int i2c_write_bytes(u8 offset, u8 reg, u8 *data, u8 len)
{
    int ret;

    // if offset is changed, write it first
    if (offset != old_offset) {
        ret = i2c_smbus_write_byte_data(client, LT6911_REG_OFFSET, offset);
        if (ret < 0) {
            printk(KERN_ERR "[W]Failed to write offset to the i2c bus; ret = %d\n", ret);
            return -1;
        }
        old_offset = offset;
    }

    // write the data to the i2c bus
    ret = i2c_smbus_write_i2c_block_data(client, reg, len, data);
    if (ret < 0) {
        printk(KERN_ERR "[W]Failed to write data to the i2c bus; ret = %d\n", ret);
        return -1;
    }

    return 0;
}

// I2C read function
int i2c_read_bytes(u8 offset, u8 reg, u8 *data, u8 len)
{
    int ret;

    // if offset is changed, write it first
    if (offset != old_offset) {
        ret = i2c_smbus_write_byte_data(client, LT6911_REG_OFFSET, offset);
        if (ret < 0) {
            printk(KERN_ERR "[W]Failed to write offset to the i2c bus; ret = %d\n", ret);
            return -1;
        }
        old_offset = offset;
    }

    ret = i2c_smbus_read_i2c_block_data(client, reg, len, data);
    if (ret < 0) {
        printk(KERN_ERR "[R]Failed to read data from the i2c bus; ret = %d\n", ret);
        return -1;
    }


    return 0;
}
#endif

int lt6911_enable(void) {
    // Enable the LT6911 by writing to the appropriate register
    if (chip_platform == LT6911_CHIP_LT6911D) {
        return 0; // LT6911D does not require enable
    }

    if (i2c_write_byte(LT6911_SYS_OFFSET, 0xEE, 0x01) != 0) {
        printk(KERN_ERR "Failed to enable LT6911UXC\n");
        return -1;
    }
    return 0;
}

int lt6911_disable(void) {
    // Disable the LT6911UXC by writing to the appropriate register
    if (chip_platform == LT6911_CHIP_LT6911D) {
        return 0; // LT6911D does not require disable
    }

    if (i2c_write_byte(LT6911_SYS_OFFSET, 0xEE, 0x00) != 0) {
        printk(KERN_ERR "Failed to disable LT6911UXC\n");
        return -1;
    }
    return 0;
}

int lt6911_disable_watchdog(void) {
    // Disable the LT6911 watchdog timer
    if (chip_platform == LT6911_CHIP_LT6911UXC) {
        // LT6911UXC
        if (i2c_write_byte(LT6911_SYS2_OFFSET, 0x10, 0x00) != 0) {
            printk(KERN_ERR "Failed to disable LT6911UXC watchdog\n");
            return -1;
        }
    } else if (chip_platform == LT6911_CHIP_LT6911C) {
        // LT6911C does not require watchdog disable
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        // LT6911D does not require watchdog disable
    } else {
        printk(KERN_ERR "Unknown chip platform\n");
        return -1;
    }
    return 0;
}

// function for checking the chip i2c register
int check_chip_register(void)
{
    u8 chip_id[4] = {0};

    lt6911_enable();

    // Read the chip ID register LT6911UXC
    if (i2c_read_byte(LT6911_SYS3_OFFSET, 0x00, chip_id) < 0) return -1;
    if (i2c_read_byte(LT6911_SYS3_OFFSET, 0x01, chip_id + 1) < 0) return -1;
    if (i2c_read_byte(LT6911_SYS3_OFFSET, 0x02, chip_id + 2) < 0) return -1;

    if (chip_id[0] == 0x17 && chip_id[1] == 0x04 && chip_id[2] == 0x83) {
        chip_platform = LT6911_CHIP_LT6911UXC;
        printk(KERN_INFO "Chip: LT6911UXC\n");
        return 0; // LT6911UXC
    }

    // Read the chip ID register LT6911C
    if (i2c_read_byte(LT6911_SYS4_OFFSET, 0x00, chip_id) < 0) return -1;
    if (i2c_read_byte(LT6911_SYS4_OFFSET, 0x01, chip_id + 1) < 0) return -1;

    if (chip_id[0] == 0x16 && chip_id[1] == 0x05) {
        chip_platform = LT6911_CHIP_LT6911C;
        printk(KERN_INFO "Chip: LT6911C\n");
        return 1; // LT6911C
    }

    // Read the chip ID register LT6911D
    if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0xEE, 0x01) != 0) return -1;
    if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x80, chip_id) < 0) return -1;
    if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x81, chip_id + 1) < 0) return -1;
    if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x82, chip_id + 2) < 0) return -1;
    if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x83, chip_id + 3) < 0) return -1;

    if (chip_id[0] == 0x25 && chip_id[1] == 0x10 && chip_id[2] == 0x23 && chip_id[3] == 0x01) {
        chip_platform = LT6911_CHIP_LT6911D;
        printk(KERN_INFO "Chip: LT6911D\n");
        return 2; // LT6911D
    }

    chip_platform = LT6911_CHIP_UNKNOWN;
    return -1;
}

int lt6911_get_signal_state(u8 *p_state)
{
	u8 hdmi_signal, audio_signal;
    u8 state = 0;
    u8 vactive[2];
    u8 hactive[2];
    int ret = 0;

    if (chip_platform == LT6911_CHIP_LT6911C) {
        // LT6911C
        // start to measure
        if (i2c_write_byte(LT6911C_HDMI_INFO_OFFSET, 0x83, 0x11) != 0) return -1;
        // delay 5ms
        msleep(5);
        // read HDMI signal
        if (i2c_read_byte(LT6911C_HDMI_INFO_OFFSET, 0x96, vactive  ) != 0) return -1;
        if (i2c_read_byte(LT6911C_HDMI_INFO_OFFSET, 0x97, vactive+1) != 0) return -1;
        if (i2c_read_byte(LT6911C_HDMI_INFO_OFFSET, 0x8B, hactive  ) != 0) return -1;
        if (i2c_read_byte(LT6911C_HDMI_INFO_OFFSET, 0x8C, hactive+1) != 0) return -1;
        // check HDMI signal
        if (vactive[0] == 0 || vactive[1] == 0 ||
            hactive[0] == 0 || hactive[1] == 0) {
            state &= ~0x01; // HDMI signal disappear
        } else {
            state |= 0x01;  // HDMI signal stable
        }
        // LT6911C Audio default is stable
        state |= 0x02;
    } else if (chip_platform == LT6911_CHIP_LT6911UXC) {
        // LT6911UXC
        if (i2c_write_byte(LT6911_HDMI_INFO_OFFSET, 0xEE, 0x00) != 0) return -1;

        // Check HDMI signal
        if (i2c_read_byte(LT6911_HDMI_INFO_OFFSET, 0xA3, &hdmi_signal) != 0) return -1;
        switch (hdmi_signal) {
        case 0x88:
            state &= ~0x01; // HDMI signal disappear
            break;
        case 0x55:
            state |= 0x01;  // HDMI signal stable
            break;
        default:
            printk(KERN_ERR "Unknown HDMI signal: 0x%02x\n", hdmi_signal);
            // ret = -1;    // Unknown signal
            break;
        }

        // Check Audio signal
        if (i2c_read_byte(LT6911_AUDIO_INFO_OFFSET, 0xA5, &audio_signal) != 0) return -1;
        switch (audio_signal)
        {
        case 0x88:
            state &= ~0x02;  // Audio signal disappear
            break;
        case 0x55:
            state |= 0x02;
            state |= 0x04;  // Audio sample rate go higher
            break;
        case 0xAA:
            state |= 0x02;
            state &= ~0x04;  // Audio sample rate go lower
            break;
        case 0x01:           // unknown audio signal but stable
        case 0x03:           // unknown audio signal but stable
            state |= 0x02;
            state &= ~0x04;  // Audio sample rate go lower
            break;
        default:
            printk(KERN_ERR "Unknown Audio signal: 0x%02x\n", audio_signal);
            // ret = -1;    // Unknown signal
            break;
        }
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        u8 audio_fs_h, audio_fs_l;
        u16 hactive_val, vactive_val, audio_fs;

        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8C, &hactive[0]) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8D, &hactive[1]) != 0) return -1;
        hactive_val = ((hactive[0] << 8) | hactive[1]) * 2;

        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8E, &vactive[0]) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8F, &vactive[1]) != 0) return -1;
        vactive_val = (vactive[0] << 8) | vactive[1];

        if (hactive_val != 0 || vactive_val != 0) {
            state |= 0x01;  // HDMI signal stable
        } else {
            state &= ~0x01; // HDMI signal disappear
        }

        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x90, &audio_fs_h) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x91, &audio_fs_l) != 0) return -1;
        audio_fs = (audio_fs_h << 8) | audio_fs_l;

        if (audio_fs != 0) {
            state |= 0x02;  // Audio signal stable
        } else {
            state &= ~0x02; // Audio signal disappear
        }
    } else {
        printk(KERN_ERR "Unknown chip platform\n");
        return -1;
    }

    *p_state = state;
    return ret;
}

int lt6911_get_csi_res(u16 *p_width, u16 *p_height)
{
    u8 val[4];
    u8 res_type;
	u16 height;
	u16 width;

    // Getting Hactive & Vactive
    if (chip_platform == LT6911_CHIP_LT6911C) {
        // LT6911C
        if (i2c_read_byte(LT6911C_CSI_INFO_OFFSET, 0x06, val  ) != 0) return -1;
        if (i2c_read_byte(LT6911C_CSI_INFO_OFFSET, 0x07, val+1) != 0) return -1;
        // width
        if (i2c_read_byte(LT6911C_CSI_INFO_OFFSET, 0x38, val+2) != 0) return -1;
        if (i2c_read_byte(LT6911C_CSI_INFO_OFFSET, 0x39, val+3) != 0) return -1;
    } else if (chip_platform == LT6911_CHIP_LT6911UXC) {
        // LT6911UXC
        // height
        if (i2c_read_byte(LT6911_CSI_INFO_OFFSET, 0xF0, val  ) != 0) return -1;
        if (i2c_read_byte(LT6911_CSI_INFO_OFFSET, 0xF1, val+1) != 0) return -1;
        // width
        if (i2c_read_byte(LT6911_CSI_INFO_OFFSET, 0xEA, val+2) != 0) return -1;
        if (i2c_read_byte(LT6911_CSI_INFO_OFFSET, 0xEB, val+3) != 0) return -1;
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        // LT6911D
        // height
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8E, val  ) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8F, val+1) != 0) return -1;
        // width
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8C, val+2) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8D, val+3) != 0) return -1;
    } else {
        printk(KERN_ERR "Unknown chip platform\n");
        return -1;
    }

    height = (val[0] << 8) | val[1];
    width = (val[2] << 8) | val[3];

    if (chip_platform == LT6911_CHIP_LT6911D) width *= 2;

    *p_width = width;
    *p_height = height;

    res_type = check_res(width, height);

	return res_type;
}

int lt6911_get_csi_fps(u16 *p_fps, u16 width, u16 height)
{
    u8 val[4];
    u16 HTotal, VTotal;
    u32 clk;
    u32 fps;

    if (chip_platform == LT6911_CHIP_LT6911C) {
        // LT6911C not support fps
        printk(KERN_ERR "LT6911C chip platform does not support FPS calculation\n");
        fps = 0;
        // return 0;

    } else if (chip_platform == LT6911_CHIP_LT6911UXC) {
        // LT6911UXC
        // After testing, there was a significant deviation between the manual scheme and the actual frame rate.
        // get H&V total
        if (i2c_read_byte(LT6911_CSI_TOTAL_OFFSET, 0x26, val) != 0) return -1;
        if (i2c_read_byte(LT6911_CSI_TOTAL_OFFSET, 0x27, val+1) != 0) return -1;
        if (i2c_read_byte(LT6911_CSI_TOTAL_OFFSET, 0x32, val+2) != 0) return -1;
        if (i2c_read_byte(LT6911_CSI_TOTAL_OFFSET, 0x33, val+3) != 0) return -1;

        HTotal = (val[0] << 8) | val[1];
        VTotal = (val[2] << 8) | val[3];

        printk(KERN_INFO "HDMI HTotal: %d, VTotal: %d\n", HTotal, VTotal);
        msleep(20); // ensure the fps is ready
        // begin measurement
        if (i2c_write_byte(LT6911_CSI_INFO_OFFSET, 0x40, 0x21) != 0) return -1;

        // wait for measurement to complete
        msleep(10);

        // Read Half Pixel Clock
        if (i2c_read_byte(LT6911_CSI_INFO_OFFSET, 0x48, val) != 0) return -1;
        if (i2c_read_byte(LT6911_CSI_INFO_OFFSET, 0x49, val+1) != 0) return -1;
        if (i2c_read_byte(LT6911_CSI_INFO_OFFSET, 0x4A, val+2) != 0) return -1;

        // printk(KERN_INFO "HDMI Half Pixel Clock: %02x %02x %02x\n", val[0], val[1], val[2]);

        clk = ((val[0] & 0x0f) << 16) | (val[1] << 8) | val[2];
        fps = (clk * 2000) / (HTotal * VTotal);
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x88, val) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x89, val+1) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8A, val+2) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x8B, val+3) != 0) return -1;

        HTotal = (val[0] << 8) | val[1];
        VTotal = (val[2] << 8) | val[3];
        HTotal *= 2;

        printk(KERN_INFO "HDMI HTotal: %d, VTotal: %d\n", HTotal, VTotal);
        msleep(20); // ensure the fps is ready

        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x85, val) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x86, val+1) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x87, val+2) != 0) return -1;

        clk = ((val[0] & 0x0f) << 16) | (val[1] << 8) | val[2];
        fps = (clk * 2000) / (HTotal * VTotal);
    } else {
        printk(KERN_ERR "Unknown chip platform\n");
        return -1;
    }


    // printk(KERN_INFO "HDMI FPS: %d, clk: %d\n", fps, clk);
    *p_fps = (u16)fps;

    return 0;
}

int lt6911_get_hdcp_mode(u8 *p_hdcp)
{
    u8 val = 0;

    if (chip_platform == LT6911_CHIP_LT6911C) {
        // LT6911C not support HDCP
        printk(KERN_ERR "LT6911C chip platform does not support HDCP\n");
        val = 0;
        // return 0;
    } else if (chip_platform == LT6911_CHIP_LT6911UXC) {
        // LT6911UXC
        // Read HDCP mode
        if (i2c_read_byte(LT6911_HDMI_INFO_OFFSET, 0xAB, &val) != 0) return -1;
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        // LT6911D not support HDCP detection
        val = 0;
    } else {
        printk(KERN_ERR "Unknown chip platform\n");
        return -1;
    }
    *p_hdcp = val;
    /*
    HDCP mode:
    0x00: No HDCP
    0x01: HDCP 1.4
    0x02: HDCP 2.2
    */
    return 0;
}

int lt6911_get_audio_sample_rate(u16 *p_sample_rate)
{
    u8 val;
    u8 val_buf[2];
    if (chip_platform == LT6911_CHIP_LT6911C) {
        // LT6911C
        // Read audio sample rate
        if (i2c_read_byte(LT6911C_AUDIO_INFO_OFFSET, 0x55, &val) != 0) return -1;
        *p_sample_rate = val;
    } else if (chip_platform == LT6911_CHIP_LT6911UXC) {
        // LT6911UXC
        // Read audio sample rate
        if (i2c_read_byte(LT6911_AUDIO_INFO_OFFSET, 0xAB, &val) != 0) return -1;
        *p_sample_rate = val;
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        // LT6911D
        // Read audio sample rate
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x90, &val_buf[0]) != 0) return -1;
        if (i2c_read_byte(LT6911D_MANAGE_OFFSET, 0x91, &val_buf[1]) != 0) return -1;
        *p_sample_rate = (val_buf[0] << 8) | val_buf[1];
    } else {
        return -1;
    }
    return 0;
}

bool compare_arrays(const unsigned char arr1[], const unsigned char arr2[], size_t length)
{
    u8 i;
    for (i = 0; i < length; i++) {
        if (arr1[i] != arr2[i]) {
            return false; // 一旦发现不一致，返回 false
        }
    }
    return true; // 全部元素一致，返回 true
}

int check_edid(u8 *edid_data, u16 edid_size)
{
    u16 i;
    u8 checksum1 = 0;
    u8 checksum2 = 0;

    // Check if the EDID data length is valid
    if (edid_size != EDID_BUFFER_SIZE) {
        printk(KERN_ERR "EDID data length is not %d bytes\n", EDID_BUFFER_SIZE);
        return -1; // EDID data length is not enough
    }

    // Check EDID header
    if (edid_data[0] != 0x00 || edid_data[1] != 0xFF ||
        edid_data[2] != 0xFF || edid_data[3] != 0xFF ||
        edid_data[4] != 0xFF || edid_data[5] != 0xFF ||
        edid_data[6] != 0xFF || edid_data[7] != 0x00) {
        printk(KERN_ERR "EDID header is invalid\n");
        return -1; // EDID header is invalid
    }

    // First 128 Bytes checksum
    for (i = 0; i < 127; i++) {
        checksum1 += edid_data[i];
    }
    checksum1 = 0x100 - checksum1; // Reverse
    if (checksum1 != edid_data[127]) {
        // Checksum for first 128 bytes is incorrect
        printk(KERN_ERR "Checksum for first 128 bytes is incorrect\n");
        return -1;
    }

    // Second 128 Bytes checksum
    for (i = 128; i < 255; i++) {
        checksum2 += edid_data[i];
    }
    checksum2 = 0x100 - checksum2; // Reverse
    if (checksum2 != edid_data[255]) {
        // Checksum for second 128 bytes is incorrect
        printk(KERN_ERR "Checksum for second 128 bytes is incorrect\n");
        return -1;
    }

    return 0; // EDID is valid
}

int lt6911_edid_write(u8 *edid_data, u16 edid_size)
{
    u8 i;
    u8 chip_data[16] = {0};
    u8 wr_count = edid_size / LT6911UXC_WR_SIZE + 1;
    u8 version_str[32] = {0};

#ifndef RE_WRITE_VERSION
    // read version from chip
    if (lt6911_str_read(version_str) < 0) {
        return -EIO; // version read failed
    }
#endif
    // check chip platform
    if (chip_platform == LT6911_CHIP_LT6911UXC) {
        printk(KERN_INFO "Writing EDID...\n");

        // Start
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x59, 0x51) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x10) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5B, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5C, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5D, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x81) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        // Waiting for erasure
        msleep(500);
        if (i2c_read_byte(LT6911_SYS3_OFFSET, 0x08, chip_data));
        if (chip_data[0] != 0xEE) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xAE) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xEE) != 0) return -1;
        // Write
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        for (i = 0; i < wr_count; i++) {
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0xDF) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x20) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x00) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x21) != 0) return -1;
            if (i != wr_count-1) {
                if (i2c_write_bytes(LT6911_SYS_OFFSET, 0x59, edid_data+(LT6911UXC_WR_SIZE*i), LT6911UXC_WR_SIZE) != 0) return -1;
            } else {
                if (i2c_write_bytes(LT6911_SYS_OFFSET, 0x59, version_str, LT6911UXC_WR_SIZE) != 0) return -1;
            }
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5B, 0x01) != 0) return -1;
            if (i != wr_count-1) {
                if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5C, 0x80) != 0) return -1;
                if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5D, 0x00+(LT6911UXC_WR_SIZE*i)) != 0) return -1;
            } else {
                if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5C, 0x81) != 0) return -1;
                if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5D, 0x00) != 0) return -1;
            }
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0xC0) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x90) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
            if (i != wr_count-1) {
                if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
            } else {
                if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x88) != 0) return -1;
            }
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        }
        if (i2c_read_byte(LT6911_SYS3_OFFSET, 0x08, chip_data));
        if (chip_data[0] != 0xEE) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xAE) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xEE) != 0) return -1;

        printk(KERN_INFO "EDID write completed\n");
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        printk(KERN_INFO "Writing EDID...\n");

        // Start
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0xEE, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x59, 0x50) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x04) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5B, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5C, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5D, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x01) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        // Waiting for erasure
        msleep(600);
        for (i = 0; i < wr_count; i++) {
            if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xBF) != 0) return -1;
            msleep(1);
            if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xFF) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x04) != 0) return -1;
            msleep(1);
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0xDF) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x20) != 0) return -1;
            msleep(1);
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x21) != 0) return -1;
            if (i != wr_count - 1) {
                if (i2c_write_bytes(LT6911D_MANAGE_OFFSET, 0x59, edid_data+(LT6911D_WR_SIZE*i), LT6911D_WR_SIZE) != 0) return -1;
            } else {
                if (i2c_write_bytes(LT6911D_MANAGE_OFFSET, 0x59, version_str, LT6911D_WR_SIZE) != 0) return -1;
            }
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5B, 0x00) != 0) return -1;
            if (i != wr_count - 1) {
                if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5C, 0x80) != 0) return -1;
            } else {
                if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5C, 0x81) != 0) return -1;
            }
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5D, 0x00+(LT6911D_WR_SIZE*i)) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
            msleep(5);
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
            msleep(10);
        }
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x08) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;

        printk(KERN_INFO "EDID write completed\n");
    } else {
        printk(KERN_ERR "Not currently supporting EDID writing outside of LT6911UXC\n");
    }

    return 0;
}

int lt6911_edid_read(u8 *edid_data, u16 edid_size)
{
    u8 i;
    u8 wr_count = edid_size / LT6911UXC_WR_SIZE;

    // Read EDID data from LT6911UXC
    if (chip_platform == LT6911_CHIP_LT6911UXC) {
        printk(KERN_INFO "Reading EDID...\n");
        // Read
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        for (i = 0; i < wr_count; i++) {
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0x5F) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0xA0) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5B, 0x01) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5C, 0x80) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5D, 0x00+(LT6911UXC_WR_SIZE*i)) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x90) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
            if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x21) != 0) return -1;
            if (i2c_read_bytes(LT6911_SYS_OFFSET, 0x5F, edid_data+(LT6911UXC_WR_SIZE*i), LT6911UXC_WR_SIZE) != 0) return -1;
        }
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        printk(KERN_INFO "Reading EDID...\n");
        // Read
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0xEE, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x59, 0x50) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x21) != 0) return -1;
        for (i = 0; i < wr_count; i++) {
            if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xBF) != 0) return -1;
            msleep(1);
            if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xFF) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x04) != 0) return -1;
            msleep(1);
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0x5F) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x20) != 0) return -1;
            msleep(1);
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5B, 0x00) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5C, 0x80) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5D, 0x00+(LT6911D_WR_SIZE*i)) != 0) return -1;
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
            msleep(1);
            if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
            if (i2c_read_bytes(LT6911D_MANAGE_OFFSET, 0x5F, edid_data+(LT6911D_WR_SIZE*i), LT6911D_WR_SIZE) != 0) return -1;
            msleep(10);
        }
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x08) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;
    } else {
        printk(KERN_ERR "Not currently supporting EDID reading outside of LT6911UXC\n");
        return -1;
    }
    return 0;
}

int lt6911_str_write(u8 *str, u16 len)
{
    u8 i;
    u8 chip_data[16] = {0};
    u8 version_str[LT6911UXC_WR_SIZE] = {0};

    // read version from chip
    if (lt6911_str_read(version_str) < 0) {
        return -1; // version read failed
    }

    for (i = 0; i < LT6911UXC_WR_SIZE; i++) {
        // printk(KERN_INFO "version_str[%d] = 0x%02x\n", i, version_str[i]);
        if (version_str[i] != 0xFF && version_str[i] != 0x00) {
            return -1; // version string is not empty
        }
    }

    // max len = LT6911UXC_WR_SIZE;
    if (len > LT6911UXC_WR_SIZE) {
        printk(KERN_ERR "String length exceeds maximum of %d bytes\n", LT6911UXC_WR_SIZE);
        return -1;
    }

    // check chip platform
    if (chip_platform == LT6911_CHIP_LT6911UXC) {
        printk(KERN_INFO "Writing String...\n");

        // Start
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x59, 0x51) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x10) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5B, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5C, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5D, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x81) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        // Waiting for erasure
        msleep(500);
        if (i2c_read_byte(LT6911_SYS3_OFFSET, 0x08, chip_data));
        if (chip_data[0] != 0xEE) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xAE) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xEE) != 0) return -1;
        // Write
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;

        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x20) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_write_bytes(LT6911_SYS_OFFSET, 0x59, str, len) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5B, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5C, 0x81) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5D, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0xC0) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x90) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        // if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x88) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;

        if (i2c_read_byte(LT6911_SYS3_OFFSET, 0x08, chip_data));
        if (chip_data[0] != 0xEE) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xAE) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS3_OFFSET, 0x08, 0xEE) != 0) return -1;

        printk(KERN_INFO "EDID write completed\n");
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        printk(KERN_INFO "Writing EDID for LT6911D...\n");
        // LT6911D specific write sequence

        // Start
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0xEE, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x59, 0x50) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x04) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5B, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5C, 0x81) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5D, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x01) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        // Waiting for erasure
        msleep(600);
        if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xBF) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xFF) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x04) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x20) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_write_bytes(LT6911D_MANAGE_OFFSET, 0x59, str, len) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5B, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5C, 0x81) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5D, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
        msleep(5);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        msleep(10);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x08) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;

    } else {
        printk(KERN_ERR "Not currently supporting String writing outside of LT6911UXC/LT6911D\n");
    }

    return 0;
}

int lt6911_str_read(u8 *str)
{
    int i = 0;
    // Read String data from LT6911UXC
    if (chip_platform == LT6911_CHIP_LT6911UXC) {
        printk(KERN_INFO "Reading String...\n");
        // Read
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0xFF, 0x80) != 0) return -1;
        lt6911_enable();
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x84) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5E, 0x5F) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0xA0) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5B, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5C, 0x81) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5D, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x90) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x5A, 0x80) != 0) return -1;
        if (i2c_write_byte(LT6911_SYS_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_read_bytes(LT6911_SYS_OFFSET, 0x5F, str, LT6911UXC_WR_SIZE) != 0) return -1;
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        printk(KERN_INFO "Reading String...\n");
        // Read
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0xEE, 0x01) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0xDF) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x59, 0x50) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x21) != 0) return -1;
        if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xBF) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_DATA_OFFSET, 0x03, 0xFF) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x04) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5E, 0x5F) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x20) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5B, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5C, 0x81) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5D, 0x00+(LT6911D_WR_SIZE*i)) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x10) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_read_bytes(LT6911D_MANAGE_OFFSET, 0x5F, str, LT6911D_WR_SIZE) != 0) return -1;
        msleep(10);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x08) != 0) return -1;
        msleep(1);
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x5A, 0x00) != 0) return -1;
        if (i2c_write_byte(LT6911D_MANAGE_OFFSET, 0x58, 0x00) != 0) return -1;

    } else {
        printk(KERN_ERR "Not currently supporting String reading outside of LT6911UXC\n");
        return -1;
    }
    for (i = 0; i < LT6911UXC_WR_SIZE; i++) {
        if (str[i] == 0xFF) {
            str[i] = '\0'; // Replace null byte with string terminator
        }
    }
    return 0;
}

void hdmi_change_process(u8 hdmi_state)
{
    int hdmi_type;
	u16 height = 0;
	u16 width = 0;
    u16 fps = 0;
    u8 hdcp_mode;

    if (hdmi_state == 1) {
        // HDMI signal is stable
        printk(KERN_INFO "HDMI signal is stable\n");
        hdmi_type = lt6911_get_csi_res(&width, &height);
        printk(KERN_INFO "Detected HDMI resolution type: %d\n", hdmi_type);
        if (hdmi_type < 0) {
            printk(KERN_ERR "Failed to get HDMI resolution\n");
            lt6911_disable();
            return;
        }
        // Update proc file content
        switch (hdmi_type)
        {
        case NEW_RES:
            // snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "new res\n");
            // hdmi_status_buffer_length = strlen(hdmi_status_buffer);
            // break;
        case NORMAL_RES:
            // snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "normal res\n");
            // snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "new res\n");
            // hdmi_status_buffer_length = strlen(hdmi_status_buffer);
            // break;
        case UNSUPPORT_RES:
            // snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "unsupport res\n");
            // snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "new res\n");
            // hdmi_status_buffer_length = strlen(hdmi_status_buffer);
            // break;
        case UNKNOWN_RES:
            // snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "unknown res\n");
            snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "new res\n");
            hdmi_status_buffer_length = strlen(hdmi_status_buffer);
            break;
        case ERROR_RES:
            snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "error res\n");
            snprintf(hdmi_width_buffer, sizeof(hdmi_width_buffer), "%d\n", 0);
            snprintf(hdmi_height_buffer, sizeof(hdmi_height_buffer), "%d\n", 0);
            hdmi_status_buffer_length = strlen(hdmi_status_buffer);
            hdmi_width_buffer_length = strlen(hdmi_width_buffer);
            hdmi_height_buffer_length = strlen(hdmi_height_buffer);
            printk(KERN_ERR "Error resolution detected\n");
            lt6911_disable();
            return;
        default:
            break;
        }
        snprintf(hdmi_width_buffer, sizeof(hdmi_width_buffer), "%d\n", width);
        snprintf(hdmi_height_buffer, sizeof(hdmi_height_buffer), "%d\n", height);
        hdmi_width_buffer_length = strlen(hdmi_width_buffer);
        hdmi_height_buffer_length = strlen(hdmi_height_buffer);
        printk(KERN_INFO "HDMI resolution: %d x %d\n", width, height);

        // Get HDCP mode
        lt6911_get_hdcp_mode(&hdcp_mode);
        switch (hdcp_mode)
        {
        case 0x00:
            // printk(KERN_INFO "HDCP mode: No HDCP\n");
            snprintf(hdmi_hdcp_buffer, sizeof(hdmi_hdcp_buffer), "no hdcp\n");
            break;
        case 0x01:
            // printk(KERN_INFO "HDCP mode: HDCP 1.4\n");
            snprintf(hdmi_hdcp_buffer, sizeof(hdmi_hdcp_buffer), "hdcp 1.4\n");
            break;
        case 0x02:
            // printk(KERN_INFO "HDCP mode: HDCP 2.2\n");
            snprintf(hdmi_hdcp_buffer, sizeof(hdmi_hdcp_buffer), "hdcp 2.2\n");
            break;

        default:
            // printk(KERN_ERR "Unknown HDCP mode: %02x\n", hdcp_mode);
            snprintf(hdmi_hdcp_buffer, sizeof(hdmi_hdcp_buffer), "unknown hdcp\n");
            break;
        }
        hdmi_hdcp_buffer_length = strlen(hdmi_hdcp_buffer);
        printk(KERN_INFO "HDCP mode: %s\n", hdmi_hdcp_buffer);

        // Get CSI fps
        lt6911_get_csi_fps(&fps, width, height);
        snprintf(hdmi_fps_buffer, sizeof(hdmi_fps_buffer), "%d\n", fps);
        hdmi_fps_buffer_length = strlen(hdmi_fps_buffer);
        printk(KERN_INFO "HDMI FPS: %d\n", fps);

        atomic_inc(&lt6911_ctx.status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);

    } else if (hdmi_state == 0) {
        // HDMI signal has disappeared
        snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "disappear\n");
        snprintf(hdmi_width_buffer, sizeof(hdmi_width_buffer), "%d\n", 0);
        snprintf(hdmi_height_buffer, sizeof(hdmi_height_buffer), "%d\n", 0);
        snprintf(hdmi_fps_buffer, sizeof(hdmi_fps_buffer), "%d\n", 0);
        snprintf(hdmi_hdcp_buffer, sizeof(hdmi_hdcp_buffer), "no hdcp\n");
        hdmi_status_buffer_length = strlen(hdmi_status_buffer);
        hdmi_width_buffer_length = strlen(hdmi_width_buffer);
        hdmi_height_buffer_length = strlen(hdmi_height_buffer);
        hdmi_fps_buffer_length = strlen(hdmi_fps_buffer);
        hdmi_hdcp_buffer_length = strlen(hdmi_hdcp_buffer);
        printk(KERN_INFO "HDMI signal has disappeared\n");

        atomic_inc(&lt6911_ctx.status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);

        lt6911_disable();
        return;
    } else {
        // unknown HDMI state
        printk(KERN_ERR "Unknown HDMI state: %d\n", hdmi_state);
        snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "unknown\n");
        snprintf(hdmi_width_buffer, sizeof(hdmi_width_buffer), "%d\n", 0);
        snprintf(hdmi_height_buffer, sizeof(hdmi_height_buffer), "%d\n", 0);
        snprintf(hdmi_fps_buffer, sizeof(hdmi_fps_buffer), "%d\n", 0);
        snprintf(hdmi_hdcp_buffer, sizeof(hdmi_hdcp_buffer), "unknown hdcp\n");
        hdmi_status_buffer_length = strlen(hdmi_status_buffer);
        hdmi_width_buffer_length = strlen(hdmi_width_buffer);
        hdmi_height_buffer_length = strlen(hdmi_height_buffer);
        hdmi_fps_buffer_length = strlen(hdmi_fps_buffer);
        hdmi_hdcp_buffer_length = strlen(hdmi_hdcp_buffer);

        atomic_inc(&lt6911_ctx.status);
        wake_up_interruptible(&lt6911_ctx.wait_queue);

        lt6911_disable();
        return;
    }
}

void lt6911_force_resolution(u16 width, u16 height, int fps)
{
    snprintf(hdmi_status_buffer, sizeof(hdmi_status_buffer), "new res\n");
    snprintf(hdmi_width_buffer, sizeof(hdmi_width_buffer), "%d\n", width);
    snprintf(hdmi_height_buffer, sizeof(hdmi_height_buffer), "%d\n", height);
    hdmi_status_buffer_length = strlen(hdmi_status_buffer);
    hdmi_width_buffer_length = strlen(hdmi_width_buffer);
    hdmi_height_buffer_length = strlen(hdmi_height_buffer);
    if (fps > 0) {
        snprintf(hdmi_fps_buffer, sizeof(hdmi_fps_buffer), "%d\n", fps);
        hdmi_fps_buffer_length = strlen(hdmi_fps_buffer);
    } else {
        snprintf(hdmi_fps_buffer, sizeof(hdmi_fps_buffer), "%d\n", 30);
        hdmi_fps_buffer_length = strlen(hdmi_fps_buffer);
    }

    atomic_inc(&lt6911_ctx.status);
    wake_up_interruptible(&lt6911_ctx.wait_queue);
}

void audio_change_process(u8 audio_state)
{
    u16 sample_rate;
    int ret;
    // Handle audio change events
    switch (audio_state)
    {
    case 0:
        // Audio signal is disappearing
        snprintf(audio_sample_rate_buffer, sizeof(audio_sample_rate_buffer), "disappear\n");
        audio_sample_rate_buffer_length = strlen(audio_sample_rate_buffer);
        // printk(KERN_INFO "Audio signal has disappeared\n");
        break;
    case 1:
        // Audio signal is stable
        ret = lt6911_get_audio_sample_rate(&sample_rate);
        if (ret < 0) {
            printk(KERN_ERR "Failed to get audio sample rate\n");
            snprintf(audio_sample_rate_buffer, sizeof(audio_sample_rate_buffer), "unknown\n");
            audio_sample_rate_buffer_length = strlen(audio_sample_rate_buffer);
            return;
        }
        snprintf(audio_sample_rate_buffer, sizeof(audio_sample_rate_buffer), "%d\n", sample_rate);
        audio_sample_rate_buffer_length = strlen(audio_sample_rate_buffer);
        // printk(KERN_INFO "Audio signal is stable\n");
        break;
    default:
        break;
    }
}

static void get_hdmi_info_handler(struct work_struct *work)
{
    u8 signal_state;
    int ret;

    if (lt6911_enable() < 0) return;
    if (lt6911_disable_watchdog() < 0) {
        printk(KERN_ERR "Failed to disable watchdog\n");
        lt6911_disable();
        return;
    }

    // check HDMI state
    ret = lt6911_get_signal_state(&signal_state);
    printk(KERN_INFO "HDMI signal state: 0x%02x\n", signal_state);
    if (ret < 0) {
        printk(KERN_ERR "Failed to get HDMI state\n");
        lt6911_disable();
        return;
    }

    if (signal_state & 0x01) {
        if (force_height != -1 && force_width != -1) {
            lt6911_force_resolution(force_width, force_height, force_fps);
        } else {
            hdmi_change_process(1); // HDMI signal is stable
        }
    } else {
        hdmi_change_process(0); // HDMI signal is disappearing
    }

    // audio signal
    if (signal_state & 0x02) audio_change_process(1); // Audio signal is stable
    else                     audio_change_process(0); // Audio signal is disappearing

    if (lt6911_disable() < 0) return;

    return;
}

// Interupt IRQ
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    // read GPIO level
    int value = gpio_get_value(INT_PIN);

    if (chip_platform == LT6911_CHIP_LT6911UXC) {
        // LT6911UXC
        if (value == 0) {
            // fall edge detected: lt6911uxc
            schedule_work(&get_hdmi_info_work);
        }
    } else if (chip_platform == LT6911_CHIP_LT6911D) {
        // LT6911D
        if (value == 0) {
            // fall edge detected: lt6911d
            schedule_work(&get_hdmi_info_work);
        }
    } else if (chip_platform == LT6911_CHIP_LT6911C) {
        // LT6911C
        if (value == 1) {
            // rise edge detected: lt6911c
            schedule_work(&get_hdmi_info_work);
        }
    } else {
        // printk(KERN_ERR "Unknown chip platform\n");
        return IRQ_HANDLED;
    }

    return IRQ_HANDLED;
}

int i2c_init(void)
{
    struct i2c_adapter *adapter;

    adapter = i2c_get_adapter(I2C_BUS);
    if (!adapter) {
        printk(KERN_ERR "Failed to get I2C adapter\n");
        free_irq(irq_number, NULL);
        gpio_free(INT_PIN);
        return -1;
    }
    // printk(KERN_INFO "I2C adapter %d obtained\n", I2C_BUS);

    // struct i2c_client *client = to_i2c_client(dev);
    client = i2c_new_dummy(adapter, LT6911_ADDR);
    if (!client) {
        printk(KERN_ERR "Failed to create I2C device\n");
        free_irq(irq_number, NULL);
        gpio_free(INT_PIN);
        return -1;
    }

    // free the adapter after use
    i2c_put_adapter(adapter);

    return 0;
}

static int __init lt6911_manage_init(void)
{
    int ret;
    // struct i2c_adapter *adapter;

    printk(KERN_INFO "Force HDMI width: %d, height: %d, fps: %d\n", force_width, force_height, force_fps);

    // init GPIO
    ret = gpio_init();
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize GPIO\n");
        return -ENODEV;
    }

    // check 86102uxc existence
    board_version_check();

    // Init I2C
    ret = i2c_init();
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize I2C\n");
        return -ENODEV;
    }

    // check chip register
    ret = check_chip_register();
    if (ret < 0) {
        printk(KERN_ERR "This module only supports LT6911UXC/LT6911C chip\n");
        return -ENODEV;
    }

    // create proc info files
    if (proc_info_init() < 0) {
        printk(KERN_ERR "Failed to create proc info files\n");
        return -ENOMEM;
    }

    // init proc info buffers
    proc_buffer_init();

    // init 6911 hdmi info work
    INIT_WORK(&get_hdmi_info_work, get_hdmi_info_handler);

    // read version from chip
    if (lt6911_str_read(hdmi_version_buffer) < 0) {
        return -EIO; // version read failed
    }

    // read EDID from chip to buffer
    hdmi_edid_snapshot_buffer_length = EDID_BUFFER_SIZE;
    if (lt6911_edid_read(hdmi_edid_snapshot_buffer, hdmi_edid_snapshot_buffer_length) < 0) {
        return -EIO; // EDID read failed
    }

    // Restart Chip
    lt86102_pwr_ctrl(0); // Power off the LT86102UXC first
    lt6911_pwr_ctrl(0); // Power off the LT6911UXC
    msleep(100);
    lt6911_pwr_ctrl(1); // Power on the LT6911UXC first
    msleep(100);
    lt86102_pwr_ctrl(1); // Power on the LT86102UXC

    printk(KERN_INFO "lt6911_manage module loaded\n");
    return 0;
}

static void __exit lt6911_manage_exit(void)
{
    gpio_exit();
    i2c_unregister_device(client);
    cancel_work_sync(&get_hdmi_info_work);
    proc_info_exit();
    printk(KERN_INFO "GPIO-I2C interrupt module unloaded\n");
}

module_init(lt6911_manage_init);
module_exit(lt6911_manage_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.32");
MODULE_AUTHOR("Z2Z-BuGu");
MODULE_AUTHOR("916BGAI");
MODULE_DESCRIPTION("NanoKVM-Pro HDMI Module Management");

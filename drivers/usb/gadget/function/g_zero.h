/*
 * This header declares the utility functions used by "Gadget Zero", plus
 * interfaces to its two single-configuration function drivers.
 */

#ifndef __G_ZERO_H
#define __G_ZERO_H

#define GZERO_BULK_BUFLEN	4096
#define GZERO_QLEN		32
#define GZERO_ISOC_INTERVAL	4
#define GZERO_ISOC_MAXPACKET	1024
#define GZERO_SS_BULK_QLEN	1
#define GZERO_SS_ISO_QLEN	8

struct usb_zero_options {
	unsigned int pattern;
	unsigned int isoc_interval;
	unsigned int isoc_maxpacket;
	unsigned int isoc_mult;
	unsigned int isoc_maxburst;
	unsigned int bulk_buflen;
	unsigned int qlen;
	unsigned int ss_bulk_qlen;
	unsigned int ss_iso_qlen;
};

struct f_ss_opts {
	struct usb_function_instance func_inst;
	unsigned int pattern;
	unsigned int isoc_interval;
	unsigned int isoc_maxpacket;
	unsigned int isoc_mult;
	unsigned int isoc_maxburst;
	unsigned int bulk_buflen;
	unsigned int bulk_qlen;
	unsigned int iso_qlen;
	unsigned int int_interval;
	unsigned int int_maxpacket;
	unsigned int int_maxburst;
	unsigned int int_qlen;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

struct f_lb_opts {
	struct usb_function_instance func_inst;
	unsigned bulk_buflen;
	unsigned qlen;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

void lb_modexit(void);
int lb_modinit(void);

/* common utilities */
void disable_endpoints(struct usb_composite_dev *cdev,
		struct usb_ep *in, struct usb_ep *out,
		struct usb_ep *int_in, struct usb_ep *int_out,
		struct usb_ep *iso_in, struct usb_ep *iso_out);

#endif /* __G_ZERO_H */

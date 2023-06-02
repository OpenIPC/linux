#include <linux/err.h>
#include <linux/kfifo.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

extern int do_rsh_write(void *rpdev,
			const unsigned char *buf,
			int count);

struct rsh_struct
{
	struct tty_port		port;
	struct timer_list	timer;
	struct kfifo		fifo;
	void 			*rpdev;

	spinlock_t		lock;
};

static struct rsh_struct g_rsh[3];
static struct tty_driver *g_driver;

void rsh_cb(int index, void *data, int len)
{
	struct rsh_struct *rsh = &g_rsh[index];

	kfifo_in_locked(&rsh->fifo, (char *)data, len, &rsh->lock);
}

static void rsh_receive_chars(unsigned long data)
{
	struct rsh_struct *rsh = (struct rsh_struct *)data;
	struct tty_port *port = &rsh->port;
	char c;

	while (kfifo_out_locked(&rsh->fifo, &c, sizeof(c), &rsh->lock))
		tty_insert_flip_char(port, c, 0);

	tty_schedule_flip(port);

	spin_lock(&port->lock);
	if (port->tty)
		mod_timer(&rsh->timer, jiffies + 50);
	spin_unlock(&port->lock);
}

static int rsh_write(struct tty_struct *tty, const unsigned char *buf,
		     int count)
{
	struct rsh_struct *rsh = &g_rsh[tty->index];

	return do_rsh_write(rsh->rpdev, buf, count);
}

static int rsh_write_room(struct tty_struct *tty)
{
	return INT_MAX;
}

static int rsh_chars_in_buffer(struct tty_struct *tty)
{
	return 0;
}

static int rsh_open(struct tty_struct *tty, struct file *filp)
{
	struct rsh_struct *rsh = &g_rsh[tty->index];
	struct tty_port *port = &rsh->port;

	if (!port->tty) {
		tty->driver_data = rsh;
		tty->port = port;
		port->tty = tty;
		mod_timer(&rsh->timer, jiffies + 10);
	}

	return 0;
}

static void rsh_close(struct tty_struct *tty, struct file *filp)
{
	struct rsh_struct *rsh = &g_rsh[tty->index];
	struct tty_port *port = &rsh->port;

	if (tty->count == 1) {
		port->tty = NULL;
		del_timer(&rsh->timer);
	}
}

static const struct tty_operations rsh_ops =
{
	.open			= rsh_open,
	.close			= rsh_close,
	.write			= rsh_write,
	.write_room		= rsh_write_room,
	.chars_in_buffer	= rsh_chars_in_buffer,
};

extern int rsh_init_device(int index, void *rpdev)
{
	struct rsh_struct *rsh = &g_rsh[index];
	struct tty_port *port;
	int ret;
	const char *name[3] = {
		"CA9-A <-> CA9-B",
		"CA9-A <-> ARM11",
		"CA9-B <-> ARM11",
	};

	rsh = &g_rsh[index];
	rsh->rpdev = rpdev;
	port = &rsh->port;

	tty_port_init(port);

	setup_timer(&rsh->timer, rsh_receive_chars, (unsigned long)rsh);

	spin_lock_init(&rsh->lock);

        ret = kfifo_alloc(&rsh->fifo, 1024 * 64, GFP_KERNEL);
        if (ret)
                return ret;

        tty_port_register_device(port, g_driver, index, NULL);

	printk("RPMSG: Remote Shell /dev/ttyRSH%d (%s)\n", index, name[index]);

	return 0;
}

int rsh_init_driver(void)
{
	struct tty_driver *driver;
	int ret;

	driver = tty_alloc_driver(3, 0);
	if (!driver)
		return -ENOMEM;

	driver->driver_name	= "ttyRSH";
	driver->name		= "ttyRSH";
	driver->major		= 0;
	driver->minor_start	= 0;
	driver->type		= TTY_DRIVER_TYPE_SYSTEM;
	driver->subtype		= SYSTEM_TYPE_SYSCONS;
	driver->init_termios	= tty_std_termios;
	driver->flags		= TTY_DRIVER_DYNAMIC_DEV;

	tty_set_operations(driver, &rsh_ops);
	ret = tty_register_driver(driver);
	if (ret) {
		put_tty_driver(driver);
		return ret;
	}

	g_driver = driver;

	return 0;
}

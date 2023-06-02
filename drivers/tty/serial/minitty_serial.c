/*
* Smallest shortcut replacement for tty and serial core layers.
*
* Based mainly on tty_io.c, n_tty.c and serial_core.c from many smart people.
*
* Created by:  Nicolas Pitre, January 2017
* Copyright:   (C) 2017  Linaro Limited
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/ctype.h>
#include <linux/console.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/serial_core.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct minitty_data {
	struct uart_state state;
	struct ktermios termios;
	struct mutex mutex;
	unsigned char *rx_buf;
	int rx_head, rx_vetted, rx_tail;
	int rx_lines, column, canon_start_pos;
	bool rx_raw;
	bool rx_overflow;
	wait_queue_head_t write_wait;
	wait_queue_head_t read_wait;
	struct work_struct rx_work;
	struct cdev cdev;
	struct device *dev;
	int usecount;
};

#define RX_BUF_SIZE PAGE_SIZE
#define RX_BUF_WRAP(x) ((x) & (RX_BUF_SIZE - 1))

/*
* Functions called back by low level UART drivers when
* the TX buffer is getting near empty.
*/
void uart_write_wakeup(struct uart_port *port)
{
	struct uart_state *state = port->state;
	struct minitty_data *mtty = container_of(state, typeof(*mtty), state);

	wake_up_interruptible_poll(&mtty->write_wait, POLLOUT);
}
EXPORT_SYMBOL(uart_write_wakeup);

static void
uart_update_mctrl(struct uart_port *port, unsigned int set, unsigned int clear)
{
	unsigned long flags;
	unsigned int old;

	spin_lock_irqsave(&port->lock, flags);
	old = port->mctrl;
	port->mctrl = (old & ~clear) | set;
	if (old != port->mctrl)
		port->ops->set_mctrl(port, port->mctrl);
	spin_unlock_irqrestore(&port->lock, flags);
}

#define uart_set_mctrl(port, set)	uart_update_mctrl(port, set, 0)
#define uart_clear_mctrl(port, clear)	uart_update_mctrl(port, 0, clear)

static void uart_change_pm(struct uart_state *state,
			enum uart_pm_state pm_state)
{
	struct uart_port *port =state->uart_port; 

	if (state->pm_state != pm_state) {
		if (port && port->ops->pm)
			port->ops->pm(port, pm_state, state->pm_state);
		state->pm_state = pm_state;
	}
}

int uart_suspend_port(struct uart_driver *drv, struct uart_port *port)
{
	return -EPROTONOSUPPORT;
}
EXPORT_SYMBOL(uart_suspend_port);

int uart_resume_port(struct uart_driver *drv, struct uart_port *port)
{
	return -EPROTONOSUPPORT;
}
EXPORT_SYMBOL(uart_resume_port);

/**
*	uart_handle_dcd_change - handle a change of carrier detect state
*	@port: uart_port structure for the open port
*	@status: new carrier detect status, nonzero if active
*
*	Caller must hold port->lock
*/
void uart_handle_dcd_change(struct uart_port *port, unsigned int status)
{
	port->icount.dcd++;
}
EXPORT_SYMBOL_GPL(uart_handle_dcd_change);

/**
*	uart_handle_cts_change - handle a change of clear-to-send state
*	@port: uart_port structure for the open port
*	@status: new clear to send status, nonzero if active
*
*	Caller must hold port->lock
*/
void uart_handle_cts_change(struct uart_port *port, unsigned int status)
{
	port->icount.cts++;

	if (uart_softcts_mode(port)) {
		if (port->hw_stopped) {
			if (status) {
				port->hw_stopped = 0;
				port->ops->start_tx(port);
				uart_write_wakeup(port);
			}
		} else {
			if (!status) {
				port->hw_stopped = 1;
				port->ops->stop_tx(port);
			}
		}
	}
}
EXPORT_SYMBOL_GPL(uart_handle_cts_change);

static void uart_start_tx(struct minitty_data *mtty)
{
	struct uart_port *port = mtty->state.uart_port;
	spin_lock_irq(&port->lock);
	if (!port->hw_stopped)
		port->ops->start_tx(port);
	spin_unlock_irq(&port->lock);
}

static int uart_chars_in_buffer(struct minitty_data *mtty)
{
	struct uart_state *state = &mtty->state;
	struct uart_port *port = mtty->state.uart_port;
	int ret;

	spin_lock_irq(&port->lock);
	ret = uart_circ_chars_pending(&state->xmit);
	spin_unlock_irq(&port->lock);
	return ret;
}

static void uart_flush_tx_buffer(struct minitty_data *mtty)
{
	struct uart_state *state = &mtty->state;
	struct uart_port *port = mtty->state.uart_port;

	spin_lock_irq(&port->lock);
	uart_circ_clear(&state->xmit);
	if (port->ops->flush_buffer)
		port->ops->flush_buffer(port);
	spin_unlock_irq(&port->lock);
	uart_write_wakeup(port);
}

static int uart_get_lsr_info(struct minitty_data *mtty, unsigned int __user *p)
{
	struct uart_state *state = &mtty->state;
	struct uart_port *port = mtty->state.uart_port;
	unsigned int result;

	mutex_lock(&mtty->mutex);
	result = port->ops->tx_empty(port);

	/*
	* If we're about to load something into the transmit
	* register, we'll pretend the transmitter isn't empty to
	* avoid a race condition (depending on when the transmit
	* interrupt happens).
	*/
	if (port->x_char ||
		((uart_circ_chars_pending(&state->xmit) > 0) &&
		!uart_tx_stopped(port)))
		result &= ~TIOCSER_TEMT;
	mutex_unlock(&mtty->mutex);
	return put_user(result, p);
}

static int uart_tiocmget(struct minitty_data *mtty, int __user *p)
{
	struct uart_port *port = mtty->state.uart_port;
	int ret = -EIO;

	mutex_lock(&mtty->mutex);
	ret = port->mctrl;
	spin_lock_irq(&port->lock);
	ret |= port->ops->get_mctrl(port);
	spin_unlock_irq(&port->lock);
	mutex_unlock(&mtty->mutex);
	if (ret >= 0)
		ret = put_user(ret, p);
	return ret;
}

static int
uart_tiocmset(struct minitty_data *mtty, unsigned int cmd, unsigned __user *p)
{
	struct uart_port *port = mtty->state.uart_port;
	unsigned int set, clear, val;
	int ret;

	ret = get_user(val, p);
	if (ret)
		return ret;
	set = clear = 0;
	switch (cmd) {
	case TIOCMBIS:
		set = val;
		break;
	case TIOCMBIC:
		clear = val;
		break;
	case TIOCMSET:
		set = val;
		clear = ~val;
		break;
	}
	set &= TIOCM_DTR|TIOCM_RTS|TIOCM_OUT1|TIOCM_OUT2|TIOCM_LOOP;
	clear &= TIOCM_DTR|TIOCM_RTS|TIOCM_OUT1|TIOCM_OUT2|TIOCM_LOOP;

	mutex_lock(&mtty->mutex);
	uart_update_mctrl(port, set, clear);
	mutex_unlock(&mtty->mutex);
	return 0;
}

static int uart_break_ctl(struct minitty_data *mtty, int break_state)
{
	struct uart_port *port = mtty->state.uart_port;
	int ret = -EIO;

	mutex_lock(&mtty->mutex);
	port->ops->break_ctl(port, break_state);
	mutex_unlock(&mtty->mutex);
	return ret;
}

/*
* Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
* - mask passed in arg for lines of interest
*   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
* Caller should use TIOCGICOUNT to see which one it was
*/
static int uart_wait_modem_status(struct minitty_data *mtty, unsigned long arg)
{
	struct uart_port *uport = mtty->state.uart_port;
	struct tty_port *port = &mtty->state.port;
	DECLARE_WAITQUEUE(wait, current);
	struct uart_icount cprev, cnow;
	int ret;

	/*
	* note the counters on entry
	*/
	spin_lock_irq(&uport->lock);
	memcpy(&cprev, &uport->icount, sizeof(struct uart_icount));
	if (uport->ops->enable_ms)
		uport->ops->enable_ms(uport);
	spin_unlock_irq(&uport->lock);

	add_wait_queue(&port->delta_msr_wait, &wait);
	for (;;) {
		spin_lock_irq(&uport->lock);
		memcpy(&cnow, &uport->icount, sizeof(struct uart_icount));
		spin_unlock_irq(&uport->lock);

		set_current_state(TASK_INTERRUPTIBLE);

		if (((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
			((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
			((arg & TIOCM_CD)  && (cnow.dcd != cprev.dcd)) ||
			((arg & TIOCM_CTS) && (cnow.cts != cprev.cts))) {
			ret = 0;
			break;
		}

		schedule();

		/* see if a signal did it */
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}

		cprev = cnow;
	}
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&port->delta_msr_wait, &wait);

	return ret;
}

/*
* Get counter of input serial line interrupts (DCD,RI,DSR,CTS)
* NB: both 1->0 and 0->1 transitions are counted except for
*     RI where only 0->1 is counted.
*/
static int uart_tiocgicount(struct minitty_data *mtty, void __user *p)
{
	struct uart_port *port = mtty->state.uart_port;
	struct serial_icounter_struct icount;
	struct uart_icount cnow;

	spin_lock_irq(&port->lock);
	memcpy(&cnow, &port->icount, sizeof(struct uart_icount));
	spin_unlock_irq(&port->lock);

	memset(&icount, 0, sizeof(icount));
	icount.cts         = cnow.cts;
	icount.dsr         = cnow.dsr;
	icount.rng         = cnow.rng;
	icount.dcd         = cnow.dcd;
	icount.rx          = cnow.rx;
	icount.tx          = cnow.tx;
	icount.frame       = cnow.frame;
	icount.overrun     = cnow.overrun;
	icount.parity      = cnow.parity;
	icount.brk         = cnow.brk;
	icount.buf_overrun = cnow.buf_overrun;
	if (copy_to_user(p, &icount, sizeof(icount)))
		return -EFAULT;
	return 0;
}

static void uart_change_speed(struct minitty_data *mtty,
				struct ktermios *old_termios)
{
	struct uart_port *port = mtty->state.uart_port;
	struct ktermios *termios = &mtty->termios;
	int hw_stopped;

	port->ops->set_termios(port, termios, old_termios);

	/*
	* Set modem status enables based on termios cflag
	*/
	spin_lock_irq(&port->lock);
	if (termios->c_cflag & CRTSCTS)
		port->status |= UPSTAT_CTS_ENABLE;
	else
		port->status &= ~UPSTAT_CTS_ENABLE;

	if (termios->c_cflag & CLOCAL)
		port->status &= ~UPSTAT_DCD_ENABLE;
	else
		port->status |= UPSTAT_DCD_ENABLE;

	/* reset sw-assisted CTS flow control based on (possibly) new mode */
	hw_stopped = port->hw_stopped;
	port->hw_stopped = uart_softcts_mode(port) &&
				!(port->ops->get_mctrl(port) & TIOCM_CTS);
	if (port->hw_stopped) {
		if (!hw_stopped)
			port->ops->stop_tx(port);
	} else {
		if (hw_stopped)
			port->ops->start_tx(port);
	}
	spin_unlock_irq(&port->lock);
}

static void uart_set_termios(struct minitty_data *mtty,
				struct ktermios *old_termios)
{
	struct uart_port *port = mtty->state.uart_port;
	unsigned int cflag = mtty->termios.c_cflag;
	unsigned int iflag_mask = IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK;
	bool sw_changed = false;

	/*
	* Drivers doing software flow control also need to know
	* about changes to these input settings.
	*/
	if (port->flags & UPF_SOFT_FLOW) {
		iflag_mask |= IXANY|IXON|IXOFF;
		sw_changed =
		mtty->termios.c_cc[VSTART] != old_termios->c_cc[VSTART] ||
		mtty->termios.c_cc[VSTOP] != old_termios->c_cc[VSTOP];
	}

	/*
	* These are the bits that are used to setup various
	* flags in the low level driver. We can ignore the Bfoo
	* bits in c_cflag; c_[io]speed will always be set
	* appropriately by set_termios(). 
	*/
	if ((cflag ^ old_termios->c_cflag) == 0 &&
		mtty->termios.c_ospeed == old_termios->c_ospeed &&
		mtty->termios.c_ispeed == old_termios->c_ispeed &&
		((mtty->termios.c_iflag ^ old_termios->c_iflag) & iflag_mask) == 0 &&
		!sw_changed)
		return;

	uart_change_speed(mtty, old_termios);
	/* reload cflag from termios; port driver may have overriden flags */
	cflag = mtty->termios.c_cflag;

	/* Handle transition to B0 status */
	if ((old_termios->c_cflag & CBAUD) && !(cflag & CBAUD))
		uart_clear_mctrl(port, TIOCM_RTS | TIOCM_DTR);
	/* Handle transition away from B0 status */
	else if (!(old_termios->c_cflag & CBAUD) && (cflag & CBAUD)) {
		unsigned int mask = TIOCM_DTR;
		if (!(cflag & CRTSCTS))
			mask |= TIOCM_RTS;
		uart_set_mctrl(port, mask);
	}
}

static void uart_wait_until_sent(struct minitty_data *mtty)
{
	struct uart_port *port = mtty->state.uart_port;
	unsigned long char_time, expire, timeout;

	/*
	* Set the check interval to be 1/5 of the estimated time to
	* send a single character, and make it at least 1.
	*
	* Note: we have to use pretty tight timings here to satisfy
	* the NIST-PCTS.
	*/
	char_time = (port->timeout - HZ/50) / port->fifosize;
	char_time = char_time / 5;
	if (char_time == 0)
		char_time = 1;

	/*
	* If the transmitter hasn't cleared in twice the approximate
	* amount of time to send the entire FIFO, it probably won't
	* ever clear.  This assumes the UART isn't doing flow
	* control, which is currently the case.  Hence, if it ever
	* takes longer than port->timeout, this is probably due to a
	* UART bug of some kind.  So, we clamp the timeout parameter at
	* 2*port->timeout.
	*/
	timeout = 2 * port->timeout;

	expire = jiffies + timeout;
	while (!port->ops->tx_empty(port)) {
				msleep_interruptible(jiffies_to_msecs(char_time));
					if (signal_pending(current))
									break;
						if (time_after(jiffies, expire))
										break;
	}
}

static void mtty_wait_until_sent(struct minitty_data *mtty)
{
	long timeout = MAX_SCHEDULE_TIMEOUT;

	timeout = wait_event_interruptible_timeout(mtty->write_wait,
							!uart_chars_in_buffer(mtty), timeout);
	if (timeout > 0)
		uart_wait_until_sent(mtty);
}

static void mtty_set_termios(struct minitty_data *mtty,
				struct ktermios *old_termios)
{
	bool was_raw = mtty->rx_raw;

	mtty->rx_raw = !I_IGNCR(mtty) && !I_ICRNL(mtty) && !I_INLCR(mtty) &&
			!L_ICANON(mtty) && !L_ISIG(mtty) && !L_ECHO(mtty);
	if (!mtty->rx_raw && was_raw)
		mtty->rx_lines = mtty->column = mtty->canon_start_pos = 0;

	/* mark things we don't support. */
	mtty->termios.c_iflag |= IGNBRK | IGNPAR;
	mtty->termios.c_iflag &= ~(ISTRIP | IUCLC | IXON | IXOFF);
	mtty->termios.c_lflag &= ~IEXTEN;

	/* The termios change make the tty ready for I/O */
	wake_up_interruptible(&mtty->write_wait);
	wake_up_interruptible(&mtty->read_wait);
}

static int set_termios(struct minitty_data *mtty, unsigned int cmd,
			void __user *arg)
{
	struct ktermios new_termios, old_termios;
	int ret;

	mutex_lock(&mtty->mutex);
	new_termios = mtty->termios;
	mutex_unlock(&mtty->mutex);

	switch (cmd) {
	case TCSETAF:
	case TCSETAW:
	case TCSETA:
		ret = user_termio_to_kernel_termios(&new_termios,
							(struct termio __user *)arg);
		break;
#ifdef TCGETS2
	case TCSETSF2:
	case TCSETSW2:
	case TCSETS2:
		ret = user_termios_to_kernel_termios(&new_termios,
							(struct termios2 __user *)arg);
		break;
	default:
		ret = user_termios_to_kernel_termios_1(&new_termios,
							(struct termios __user *)arg);
		break;
#else
	default:
		ret = user_termios_to_kernel_termios(&new_termios,
							(struct termios __user *)arg);
		break;
#endif
	}
	if (ret)
		return -EFAULT;

	switch (cmd) {
	case TCSETSF:
#ifdef TCGETS2
	case TCSETSF2:
#endif
	case TCSETAF:
		uart_flush_tx_buffer(mtty);
	}

	switch (cmd) {
	case TCSETSF:
	case TCSETSW:
#ifdef TCGETS2
	case TCSETSF2:
	case TCSETSW2:
#endif
	case TCSETAF:
	case TCSETAW:
		mtty_wait_until_sent(mtty);
		if (signal_pending(current))
			return -ERESTARTSYS;
	}

	/*
	* If old style Bfoo values are used then load c_ispeed/c_ospeed
	* with the real speed so its unconditionally usable.
	*/
	new_termios.c_ispeed = tty_termios_input_baud_rate(&new_termios);
	new_termios.c_ospeed = tty_termios_baud_rate(&new_termios);

	mutex_lock(&mtty->mutex);
	old_termios = mtty->termios;
	mtty->termios = new_termios;
	mtty_set_termios(mtty, &old_termios);
	uart_set_termios(mtty, &old_termios);
	mutex_unlock(&mtty->mutex);
	return 0;
}

static int tiocsetd(int __user *p)
{
	int ldisc;

	if (get_user(ldisc, p))
		return -EFAULT;
	if (ldisc != N_TTY)
		return -EINVAL;
	return 0;
}

static int tiocgetd(int __user *p)
{
	return put_user(N_TTY, p);
}

static void copy_termios(struct minitty_data *mtty, struct ktermios *kterm)
{
	mutex_lock(&mtty->mutex);
	*kterm = mtty->termios;
	mutex_unlock(&mtty->mutex);
}

static long minitty_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct minitty_data *mtty = file->private_data;
	struct uart_port *port = mtty->state.uart_port;
	void __user *p = (void __user *)arg;
	struct ktermios kterm;
	int ret = -ENOIOCTLCMD;

	switch (cmd) {
	case TIOCSETD:
		return tiocsetd(p);
	case TIOCGETD:
		return tiocgetd(p);
	case TIOCSBRK:
		return uart_break_ctl(mtty, -1);
	case TIOCCBRK:
		return uart_break_ctl(mtty, 0);
	case TIOCMGET:
		return uart_tiocmget(mtty, p);
	case TIOCMSET:
	case TIOCMBIC:
	case TIOCMBIS:
		return uart_tiocmset(mtty, cmd, p);
	case TIOCGICOUNT:
		return uart_tiocgicount(mtty, p);
	case TIOCMIWAIT:
		return uart_wait_modem_status(mtty, arg);
	case TIOCSERGETLSR:
		return uart_get_lsr_info(mtty, p);

#ifndef TCGETS2
	case TCGETS:
		copy_termios(mtty, &kterm);
		if (kernel_termios_to_user_termios((struct termios __user *)arg, &kterm))
			return -EFAULT;
		return 0;
#else
	case TCGETS:
		copy_termios(mtty, &kterm);
		if (kernel_termios_to_user_termios_1((struct termios __user *)arg, &kterm))
			return -EFAULT;
		return 0;
	case TCGETS2:
		copy_termios(mtty, &kterm);
		if (kernel_termios_to_user_termios((struct termios2 __user *)arg, &kterm))
			return -EFAULT;
		return 0;
	case TCSETSF2:
	case TCSETSW2:
	case TCSETS2:
#endif
	case TCSETSF:
	case TCSETSW:
	case TCSETS:
	case TCSETAF:
	case TCSETAW:
	case TCSETA:
		return set_termios(mtty, cmd, p);
	case TCGETA:
		copy_termios(mtty, &kterm);
		if (kernel_termios_to_user_termio((struct termio __user *)arg, &kterm))
			return -EFAULT;
		return 0;

	default:
		mutex_lock(&mtty->mutex);
		if (port->ops->ioctl)
			ret = port->ops->ioctl(port, cmd, arg);
		mutex_unlock(&mtty->mutex);
		break;
	}

	if (ret == -ENOIOCTLCMD)
		ret = -EINVAL;
	return ret;
}

/*
* Functions called back by low level UART drivers normally provided by
* the regular TTY layer to deliver RX data that we have to emulate.
* We ssimply ignore characters with errors here.
*/

int tty_insert_flip_char(struct tty_port *port, unsigned char ch, char flag)
{
	struct uart_state *state = container_of(port, struct uart_state, port);
	struct minitty_data *mtty = container_of(state, typeof(*mtty), state);

	if (flag == TTY_NORMAL) {
		int tail = smp_load_acquire(&mtty->rx_tail);
		int head = mtty->rx_head;
		int next = RX_BUF_WRAP(head + 1);
		/*
		* Advance head only if buffer is not full.
		* Keep on overwriting last char otherwise.
		*/
		mtty->rx_buf[head] = ch;
		if (next != tail) {
			smp_store_release(&mtty->rx_head, next);
			return 1;
		} else {
			smp_store_release(&mtty->rx_overflow, true);
		}
	}		
	return 0;
}
EXPORT_SYMBOL(tty_insert_flip_char);

void uart_insert_char(struct uart_port *port, unsigned int status,
			unsigned int overrun, unsigned int ch, unsigned int flag)
{
	struct uart_state *state = port->state;
	struct minitty_data *mtty = container_of(state, typeof(*mtty), state);

	if (flag == TTY_NORMAL) {
		int tail = smp_load_acquire(&mtty->rx_tail);
		int head = mtty->rx_head;
		int next = RX_BUF_WRAP(head + 1);
		/*
		* Advance head only if buffer is not full.
		* Keep on overwriting last char otherwise.
		*/
		mtty->rx_buf[head] = ch;
		if (next != tail) {
			smp_store_release(&mtty->rx_head, next);
		} else {
			smp_store_release(&mtty->rx_overflow, true);
			port->icount.buf_overrun++;
		}
	}
}
EXPORT_SYMBOL_GPL(uart_insert_char);

int tty_insert_flip_string(struct tty_port *port, const unsigned char *chars,
			size_t size)
{
	struct uart_state *state = container_of(port, struct uart_state, port);
	struct minitty_data *mtty = container_of(state, typeof(*mtty), state);
	int head, tail, len, ret = 0;

	tail = smp_load_acquire(&mtty->rx_tail);
	head = mtty->rx_head;
	do {
		len = CIRC_SPACE(head, tail, RX_BUF_SIZE);
		if (len > size)
			len = size;
		memcpy(mtty->rx_buf+head, chars, len);
		head = RX_BUF_WRAP(head + len);
		chars += len;
		size -= len;
		ret += len;
	} while (size && len && head == 0);
	smp_store_release(&mtty->rx_head, head);
	return ret;
}
EXPORT_SYMBOL(tty_insert_flip_string);

int tty_buffer_request_room(struct tty_port *port, size_t size)
{
	struct uart_state *state = container_of(port, struct uart_state, port);
	struct minitty_data *mtty = container_of(state, typeof(*mtty), state);
	int tail = smp_load_acquire(&mtty->rx_tail);
	int head = mtty->rx_head;
	int space = CIRC_SPACE(head, tail, RX_BUF_SIZE);
	return size < space ? size : space;
}
EXPORT_SYMBOL_GPL(tty_buffer_request_room);

void tty_schedule_flip(struct tty_port *port)
{
	struct uart_state *state = container_of(port, struct uart_state, port);
	struct minitty_data *mtty = container_of(state, typeof(*mtty), state);

	queue_work(system_unbound_wq, &mtty->rx_work);
}
EXPORT_SYMBOL(tty_schedule_flip);

void tty_flip_buffer_push(struct tty_port *port)
{
	tty_schedule_flip(port);
}
EXPORT_SYMBOL(tty_flip_buffer_push);

/*
* Line Discipline Stuff
*/

static bool is_utf8_continuation(struct minitty_data *mtty, unsigned char c)
{
	return (I_IUTF8(mtty) && (c & 0xc0) == 0x80);
}

static bool is_line_termination(struct minitty_data *mtty, unsigned char c)
{
	return (c == '\n' || c == EOF_CHAR(mtty) || c == EOL_CHAR(mtty));
}

/*
* Queue the provided character string in its entirety or nothing.
* Return true if queued, false otherwise.
*/
static bool queue_tx_chars(struct minitty_data *mtty, unsigned char *s, int len)
{
	struct circ_buf *circ = &mtty->state.xmit;
	int head, tail, space;

	tail = smp_load_acquire(&circ->tail);
	head = circ->head;
	space = CIRC_SPACE(head, tail, UART_XMIT_SIZE);
	if (space < len)
		return false;
	while (len--) {
		circ->buf[head] = *s++;
		head = (head + 1) & (UART_XMIT_SIZE - 1);
	}
	smp_store_release(&circ->head, head);
	return true;
}

/*
* Queue characters in their cooked sequence.
* Return true if queued, or false otherwise.
*/
static bool tx_cooked_char(struct minitty_data *mtty, unsigned char c)
{
	int spaces, next_col = mtty->column;

	switch (c) {
	case '\n':
		if (O_ONLRET(mtty))
			next_col = 0;
		if (O_ONLCR(mtty)) {
			if (!queue_tx_chars(mtty, "\r\n", 2))
				return false;
			mtty->column = mtty->canon_start_pos = 0;
			return true;
		}
		break;
	case '\r':
		if (O_ONOCR(mtty) && mtty->column == 0)
			return true;
		if (O_OCRNL(mtty)) {
			c = '\n';
			if (O_ONLRET(mtty))
				next_col = 0;
		} else
			next_col = 0;
		break;
	case '\t':
		spaces = 8 - (mtty->column & 7);
		if (O_TABDLY(mtty) == XTABS) {
			if (!queue_tx_chars(mtty, "        ", spaces))
				return false;
			mtty->column += spaces;
			return true;
		}
		next_col += spaces;
		break;
	case '\b':
		if (next_col > 0)
			next_col--;
		break;
	default:
		if (iscntrl(c))
			break;
		if (is_utf8_continuation(mtty, c))
			break;
		next_col++;
		break;
	}
	if (!queue_tx_chars(mtty, &c, 1))
		return false;
	mtty->column = next_col;
	if (next_col == 0)
		mtty->canon_start_pos = 0;
	return true;
}

/*
* Queue echoed characters, converting CTRL sequences into "^X" if need be.
* Return true if queued, or false otherwise.
*/
static bool echo_rx_char(struct minitty_data *mtty, unsigned char c)
{
	if (L_ECHOCTL(mtty) && iscntrl(c) && c != '\t' && c != '\n') {
		unsigned char buf[2];
		buf[0] = '^';
		buf[1] = c ^ 0100;
		return queue_tx_chars(mtty, buf, 2);
	}
	if (O_OPOST(mtty))
		return tx_cooked_char(mtty, c);
	else
		return queue_tx_chars(mtty, &c, 1);
}

/*
* Remove character from RX buffer at given position by shifting
* all preceding characters ahead.
*/
static void eat_rx_char(struct minitty_data *mtty, int pos)
{
	unsigned char *buf = mtty->rx_buf;
	int tail = mtty->rx_tail;
	int bottom = (tail <= pos) ? tail : 0;

	memmove(&buf[bottom+1], &buf[bottom], pos - bottom);
	if (tail > pos) {
		buf[0] = buf[RX_BUF_SIZE-1];
		memmove(&buf[tail+1], &buf[tail], RX_BUF_SIZE - 1 - tail);
	}
	smp_store_release(&mtty->rx_tail, RX_BUF_WRAP(tail + 1));
}

/*
* Create needed erase sequence according to the erase character c at
* position pos in the RX buffer. The erase sequence is sent for each
* erased characters and only if that succeeds then the character is
* actually removed from the buffer. The erase character itself is removed
* last so if the whole erase sequence cannot be completed then this can
* be resumed later.
*/
static bool erase_rx_char(struct minitty_data *mtty, unsigned char c, int pos)
{
	int prev_pos = RX_BUF_WRAP(pos - 1);
	bool seen_alnum = false;

	while (pos != mtty->rx_tail) {
		unsigned char prev_c = mtty->rx_buf[prev_pos];

		if (is_line_termination(mtty, prev_c)) {
			/* End of previous line: we don't erase further. */
			break;
		}

		if (is_utf8_continuation(mtty, prev_c)) {
			/* UTF8 continuation char: we just drop it */
			eat_rx_char(mtty, prev_pos);
			continue;
		}

		if (c == WERASE_CHAR(mtty) && seen_alnum && !isalnum(prev_c)) {
			/* Beginning of previous word: we don't erase further */
			break;
		}

		if (prev_c == '\t') {
			/* depends on characters before the tab */
			int spaces = 0;
			int i = prev_pos;
			while (i != mtty->rx_tail) {
				unsigned char before;
				i = RX_BUF_WRAP(i - 1);
				before = mtty->rx_buf[i];
				if (before == '\t')
					break;
				if (is_line_termination(mtty, before))
					break;
				if (L_ECHOCTL(mtty) && iscntrl(before))
					spaces += 2;
				else if (is_utf8_continuation(mtty, before))
					continue;
				else if (!iscntrl(before))
					spaces++;
			}
			if (i == mtty->rx_tail)
				spaces += mtty->canon_start_pos;
			spaces = 8 - (spaces & 7);
			if (!queue_tx_chars(mtty, "\b\b\b\b\b\b\b\b", spaces))
				return false;
			mtty->column -= spaces;
		} else if (L_ECHOCTL(mtty) && iscntrl(prev_c)) {
			/* control chars were printed as "^X" */
			if (!queue_tx_chars(mtty, "\b\b  \b\b", 6))
				return false;
			mtty->column -= 2;
		} else if (!iscntrl(prev_c)) {
			if (!queue_tx_chars(mtty, "\b \b", 3))
				return false;
			mtty->column -= 1;
		}

		/* erase sequence sent, now remove the char from the buffer */
		eat_rx_char(mtty, prev_pos);

		if (c == ERASE_CHAR(mtty))
			break;
	}

	/* Finally remove the erase character itself. */
	eat_rx_char(mtty, pos);
	return true;
}

/*
* Process RX bytes: canonical mode, echo, signals, etc.
* This might not process all RX characters if e.g. there is not enough
* room in the TX buffer to contain corresponding echo sequences.
*/
static void minitty_process_rx(struct minitty_data *mtty)
{
	bool xmit = false;
	int i, head;
	
	head = smp_load_acquire(&mtty->rx_head);

	if (mtty->rx_raw) {
		smp_store_release(&mtty->rx_vetted, head);
		return;
	}

	/*
	* RX overflow mitigation: evaluate the last received character
	* stored at the very head of the buffer in case it might be a
	* signal or newline character that could kick the reader into
	* action. We potentially overwrite the last vetted character but
	* we're past any concern for lost characters at this point.
	*/
	if (unlikely(mtty->rx_overflow)) {
		WRITE_ONCE(mtty->rx_overflow, false);
		if (RX_BUF_WRAP(head + 1) == mtty->rx_tail) {
			i = RX_BUF_WRAP(head - 1);
			mtty->rx_buf[i] = mtty->rx_buf[head];
			if (mtty->rx_vetted == head)
				mtty->rx_vetted = i;
		}
	}

	for (i = mtty->rx_vetted; i != head; i = RX_BUF_WRAP(i + 1)) {
		unsigned char c = mtty->rx_buf[i];

		if (c == '\r') {
			if (I_IGNCR(mtty)) {
				eat_rx_char(mtty, i);
				continue;
			}
			if (I_ICRNL(mtty))
				mtty->rx_buf[i] = c = '\n';
		} else if (c == '\n' && I_INLCR(mtty))
			mtty->rx_buf[i] = c = '\r';

		if (L_ICANON(mtty)) {
			if ((L_ECHOE(mtty) && c == ERASE_CHAR(mtty)) ||
				(L_ECHOE(mtty) && c == WERASE_CHAR(mtty)) ||
				(L_ECHOK(mtty) && c == KILL_CHAR(mtty))) {
				xmit = true;
				if (!erase_rx_char(mtty, c, i))
						break;
				continue;
			}
			if (is_line_termination(mtty, c)) {
				mtty->rx_lines++;
				if (c != '\n')
					continue;
			}
		}

		if (L_ECHO(mtty) || (c == '\n' && L_ECHONL(mtty))) {
			xmit = true;
			if (!echo_rx_char(mtty, c))
				break;
		}
	}

	smp_store_release(&mtty->rx_vetted, i);

	if (xmit)
		uart_start_tx(mtty);
}

static bool rx_data_available(struct minitty_data *mtty, bool poll)
{
	bool data_avail = (mtty->rx_tail != mtty->rx_vetted);
	if (data_avail && !L_ICANON(mtty)) {
		int amt = poll && !TIME_CHAR(mtty) && MIN_CHAR(mtty) ?
				MIN_CHAR(mtty) : 1;
		data_avail = RX_BUF_WRAP(mtty->rx_vetted - mtty->rx_tail) >= amt;
	} else if (data_avail && !mtty->rx_lines) {
		/* wait for a full line */
		data_avail = false;
	} else if (!data_avail && mtty->rx_lines) {
		/*
		* This may happen if the RX buffer was flushed by a signal
		* or during RX overflow. Let's just reset it to zero.
		*/
		mtty->rx_lines = 0;
	}
	return data_avail;
}

static void uart_rx_work(struct work_struct *work)
{
	struct minitty_data *mtty = container_of(work, typeof(*mtty), rx_work);

	mutex_lock(&mtty->mutex);
	minitty_process_rx(mtty);
	if (rx_data_available(mtty, true))
		wake_up_interruptible_poll(&mtty->read_wait, POLLIN);
	mutex_unlock(&mtty->mutex);
}

static ssize_t minitty_raw_read(struct minitty_data *mtty, char __user *buf,
				size_t count)
{
	int head, tail, len, ret = 0;

	head = smp_load_acquire(&mtty->rx_vetted); 
	tail = mtty->rx_tail;
	do {
		len = CIRC_CNT(head, tail, RX_BUF_SIZE);
		if (len > count)
			len = count;
		if (copy_to_user(buf, mtty->rx_buf+tail, len) != 0)
			return -EFAULT;
		tail = RX_BUF_WRAP(tail + len);
		buf += len;
		count -= len;
		ret += len;
	} while (count && len && tail == 0);
	smp_store_release(&mtty->rx_tail, tail);
	return ret;
}

static ssize_t minitty_cooked_read(struct minitty_data *mtty, char __user *buf,
				size_t count)
{
	int head, tail, i, ret;
	bool eol = false;

	head = smp_load_acquire(&mtty->rx_vetted);
	tail = mtty->rx_tail;

	/* First, locate the end-of-line marker if any. */
	for (i = tail; i != head && count; i = RX_BUF_WRAP(i + 1), count--) {
		unsigned char c = mtty->rx_buf[i];
		if (is_line_termination(mtty, c)) {
			eol = true;
			break;
		}
	}

	count = CIRC_CNT(i, tail, RX_BUF_SIZE);

	if (eol) {
		/* Include the line delimiter except for EOF */
		if (mtty->rx_buf[i] != EOF_CHAR(mtty))
			count++;
		i = RX_BUF_WRAP(i + 1);
	}

	ret = minitty_raw_read(mtty, buf, count);
	if (ret >= 0 && eol) {
		/* we consumed a whole line */
		mtty->rx_lines--;
		/* adjust tail in case EOF was skipped */
		smp_store_release(&mtty->rx_tail, i);
	}
	return ret;
}

static ssize_t minitty_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	struct minitty_data *mtty = file->private_data;
	char __user *buf0 = buf;
	DEFINE_WAIT_FUNC(wait, woken_wake_function);
	int minimum, time;
	long timeout;
	int ret = 0;

	mutex_lock(&mtty->mutex);

	minimum = time = 0;
	timeout = MAX_SCHEDULE_TIMEOUT;
	if (!L_ICANON(mtty)) {
		minimum = MIN_CHAR(mtty);
		if (minimum) {
			time = (HZ / 10) * TIME_CHAR(mtty);
		} else {
			timeout = (HZ / 10) * TIME_CHAR(mtty);
			minimum = 1;
		}
	}

	add_wait_queue(&mtty->read_wait, &wait);

	while (count) {
		minitty_process_rx(mtty);

		if (!rx_data_available(mtty, false)) {
			if (!timeout)
				break;
			if (file->f_flags & O_NONBLOCK) {
				ret = -EAGAIN;
				break;
			}
			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				break;
			}
			mutex_unlock(&mtty->mutex);
			timeout = wait_woken(&wait, TASK_INTERRUPTIBLE,
						timeout);
			mutex_lock(&mtty->mutex);
			continue;
		}

		if (L_ICANON(mtty)) {
			ret = minitty_cooked_read(mtty, buf, count);
			if (ret > 0)
				buf += ret;
			break;
		}

		ret = minitty_raw_read(mtty, buf, count);
		if (ret < 0)
			break;
		buf += ret;
		count -= ret;
		if (buf - buf0 >= minimum)
			break;
		if (time)
			timeout = time;
	}

	remove_wait_queue(&mtty->read_wait, &wait);
	mutex_unlock(&mtty->mutex);
	if (buf - buf0)
		ret = buf - buf0;
	return ret;
}

static ssize_t minitty_raw_write(struct minitty_data *mtty, const char __user *buf,
				size_t count)
{
	struct circ_buf *circ = &mtty->state.xmit;
	int head, tail, len, ret = 0;

	tail = smp_load_acquire(&circ->tail);
	head = circ->head;
	do {
		len = CIRC_SPACE_TO_END(head, tail, UART_XMIT_SIZE);
		if (len > count)
			len = count;
		if (copy_from_user(circ->buf + head, buf, len) != 0)
			return -EFAULT;
		head = (head + len) & (UART_XMIT_SIZE - 1);
		buf += len;
		count -= len;
		ret += len;
	} while (count && len && head == 0);
	smp_store_release(&circ->head, head);

	uart_start_tx(mtty);
	return ret;
}

static ssize_t minitty_cooked_write(struct minitty_data *mtty, const char __user *buf,
					size_t count)
{
	const char __user *buf0 = buf;

	while (count--) {
		unsigned char c;
		if (get_user(c, buf) != 0)
			return -EFAULT;
		if (!tx_cooked_char(mtty, c))
			break;
		buf++;
	}
	mtty->canon_start_pos = mtty->column;

	uart_start_tx(mtty);
	return buf - buf0;
}

static ssize_t minitty_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct minitty_data *mtty = file->private_data;
	const char __user *buf0 = buf;
	DEFINE_WAIT_FUNC(wait, woken_wake_function);
	int ret;

	mutex_lock(&mtty->mutex);
	add_wait_queue(&mtty->write_wait, &wait);

	while (1) {
		/* give priority to RX echo and signals */
		minitty_process_rx(mtty);

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}

		if (O_OPOST(mtty))
			ret = minitty_cooked_write(mtty, buf, count);
		else
			ret = minitty_raw_write(mtty, buf, count);
		if (ret < 0)
			break;
		buf += ret;
		count -= ret;
		if (!count)
			break;
		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}
		mutex_unlock(&mtty->mutex);
		wait_woken(&wait, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
		mutex_lock(&mtty->mutex);
	}

	remove_wait_queue(&mtty->write_wait, &wait);
	mutex_unlock(&mtty->mutex);
	return (buf - buf0) ? buf - buf0 : ret;
}

static unsigned int minitty_poll(struct file *file, poll_table *wait)
{
	struct minitty_data *mtty = file->private_data;
	struct uart_port *port = mtty->state.uart_port;
	unsigned int mask = 0;

	mutex_lock(&mtty->mutex);

	poll_wait(file, &mtty->read_wait, wait);
	poll_wait(file, &mtty->write_wait, wait);

	if (rx_data_available(mtty, true)) {
		mask |= POLLIN | POLLRDNORM;
	} else {
		minitty_process_rx(mtty);
		if (rx_data_available(mtty, true))
			mask |= POLLIN | POLLRDNORM;
	}

	if (!port->hw_stopped) {
		struct circ_buf *circ = &mtty->state.xmit;
		int tail = smp_load_acquire(&circ->tail);
		int head = circ->head;
		int count = CIRC_CNT(head, tail, UART_XMIT_SIZE);
		if (count < WAKEUP_CHARS)
			mask |= POLLOUT | POLLWRNORM;
	}

	mutex_unlock(&mtty->mutex);

	return mask;
}

static int uart_port_startup(struct minitty_data *mtty)
{
	struct uart_state *state = &mtty->state;
	struct uart_port *port = state->uart_port;
	unsigned long page;
	int ret;

	/* Make sure the device is in D0 state. */
	uart_change_pm(state, UART_PM_STATE_ON);

	/* Initialise and allocate the transmit buffer. */
	page = get_zeroed_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;
	state->xmit.buf = (unsigned char *) page;
	uart_circ_clear(&state->xmit);

	/* Initialise and allocate the receive buffer. */
	page = get_zeroed_page(GFP_KERNEL);
	if (!page) {
		ret = -ENOMEM;
		goto err_free_tx;
	}
	mtty->rx_buf = (unsigned char *) page;
	mtty->rx_head = mtty->rx_tail = mtty->rx_vetted = mtty->rx_lines = 0;
	mtty->rx_overflow = false;

	ret = port->ops->startup(port);
	if (ret)
		goto err_free_rx;

	if (uart_console(port) && port->cons->cflag) {
		mtty->termios.c_cflag = port->cons->cflag;
		port->cons->cflag = 0;
	}

	/* Initialise the hardware port settings. */
	uart_change_speed(mtty, NULL);

	/*
	* Setup the RTS and DTR signals once the
	* port is open and ready to respond.
	*/
	uart_set_mctrl(port, TIOCM_RTS | TIOCM_DTR);

	return 0;

err_free_rx:
	free_page((unsigned long)mtty->rx_buf);
	mtty->rx_buf = NULL;
err_free_tx:
	free_page((unsigned long)state->xmit.buf);
	state->xmit.buf = NULL;
	return ret;
}

/*
* This routine will shutdown a serial port; interrupts are disabled, and
* DTR is dropped if the hangup on close termio flag is on.
*/
static void uart_port_shutdown(struct minitty_data *mtty)
{
	struct uart_state *state = &mtty->state;
	struct uart_port *port = state->uart_port;

	spin_lock_irq(&port->lock);
	port->ops->stop_rx(port);
	spin_unlock_irq(&port->lock);

	if (uart_console(port))
		port->cons->cflag = mtty->termios.c_cflag;

	/* Turn off DTR and RTS early. */
	if (C_HUPCL(mtty))
		uart_clear_mctrl(port, TIOCM_DTR | TIOCM_RTS);

	/* Free the IRQ and disable the port. */
	port->ops->shutdown(port);
	synchronize_irq(port->irq);

	/* Free the transmit buffer page. */
	free_page((unsigned long)state->xmit.buf);
	state->xmit.buf = NULL;

	/* Free the receive buffer page. */
	free_page((unsigned long)mtty->rx_buf);
	mtty->rx_buf = NULL;
}

static int minitty_open(struct inode *inode, struct file *file)
{
	struct minitty_data *mtty = NULL;
	dev_t devt = inode->i_rdev;
	int ret = 0;

	if (devt == MKDEV(TTYAUX_MAJOR, 1)) {
		struct console *co;
		struct uart_driver *drv;
		console_lock();
		for_each_console(co) {
			if (co->device  != uart_console_device)
				continue;
			drv = co->data;
			mtty = container_of(drv->state, typeof(*mtty), state);
			mtty +=	co->index;
			break;
		}
		console_unlock();
		if (!mtty)
			return -ENODEV;
	} else {
		mtty = container_of(inode->i_cdev, typeof(*mtty), cdev);
	}

	nonseekable_open(inode, file);

	file->private_data = mtty;

	mutex_lock(&mtty->mutex);
	if (!mtty->usecount++) {
		ret = uart_port_startup(mtty);
		if (ret)
			mtty->usecount--;
	}
	mutex_unlock(&mtty->mutex);
	return ret;
}

static int minitty_release(struct inode *inode, struct file *file)
{
	struct minitty_data *mtty = file->private_data;
	struct uart_state *state = &mtty->state;
	struct uart_port *port = state->uart_port;

	mutex_lock(&mtty->mutex);
	mtty->usecount--;
	if (!mtty->usecount) {
		uart_flush_tx_buffer(mtty);
		uart_port_shutdown(mtty);
		if (!uart_console(port))
			uart_change_pm(state, UART_PM_STATE_OFF);
	}
	mutex_unlock(&mtty->mutex);
	return 0;
}

static const struct file_operations minitty_fops = {
	.llseek		= no_llseek,
	.read		= minitty_read,
	.write		= minitty_write,
	.poll		= minitty_poll,
	.unlocked_ioctl	= minitty_ioctl,
	.open		= minitty_open,
	.release	= minitty_release,
};

struct class *minitty_class;

static int
uart_configure_port(struct uart_driver *drv, struct uart_state *state,
			struct uart_port *port)
{
	unsigned int flags;

	/*
	* If there isn't a port here, don't do anything further.
	*/
	if (!port->iobase && !port->mapbase && !port->membase)
		return -ENXIO;

	/*
	* Now do the auto configuration stuff.  Note that config_port
	* is expected to claim the resources and map the port for us.
	*/
	flags = 0;
	if (port->flags & UPF_BOOT_AUTOCONF) {
		if (!(port->flags & UPF_FIXED_TYPE)) {
			port->type = PORT_UNKNOWN;
			flags |= UART_CONFIG_TYPE;
		}
		port->ops->config_port(port, flags);
	}

	if (port->type != PORT_UNKNOWN) {
		unsigned long flags;

		pr_info("%s%d %s\n", drv->dev_name, port->line,
			port->ops->type ? port->ops->type(port) : "");

		/* Power up port for set_mctrl() */
		uart_change_pm(state, UART_PM_STATE_ON);

		/*
		* Ensure that the modem control lines are de-activated.
		* keep the DTR setting that is set in uart_set_options()
		* We probably don't need a spinlock around this, but
		*/
		spin_lock_irqsave(&port->lock, flags);
		port->ops->set_mctrl(port, port->mctrl & TIOCM_DTR);
		spin_unlock_irqrestore(&port->lock, flags);

		/*
		* If this driver supports console, and it hasn't been
		* successfully registered yet, try to re-register it.
		* It may be that the port was not available.
		*/
		if (port->cons && !(port->cons->flags & CON_ENABLED))
			register_console(port->cons);

		/*
		* Power down all ports by default, except the
		* console if we have one.
		*/
		if (!uart_console(port))
			uart_change_pm(state, UART_PM_STATE_OFF);

		return 0;
	}

	return -EINVAL;
}

/**
*	uart_add_one_port - attach a driver-defined port structure
*	@drv: pointer to the uart low level driver structure for this port
*	@port: uart port structure to use for this port.
*/
int uart_add_one_port(struct uart_driver *drv, struct uart_port *port)
{
	unsigned int index = port->line;
	dev_t devt = MKDEV(drv->major, drv->minor) + index;
	struct minitty_data *mtty;
	struct uart_state *state;
	int ret;

	mtty = container_of(drv->state, typeof(*mtty), state) + index;
	state = &mtty->state;

	state->uart_port = port;
	state->pm_state = UART_PM_STATE_UNDEFINED;
	port->state = state;
	port->cons = drv->cons;
	port->minor = drv->minor + index;
	uart_port_lock_init(port);

	/* our default termios */
	mtty->termios.c_iflag = ICRNL;
	mtty->termios.c_oflag = OPOST | ONLCR;
	mtty->termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	mtty->termios.c_lflag = ICANON | ECHO | ECHOE | ECHOK | ECHOKE | ECHOCTL;
	mtty->termios.c_ispeed = 9600;
	mtty->termios.c_ospeed = 9600;
	memcpy(mtty->termios.c_cc, INIT_C_CC, sizeof(cc_t)*NCCS);

	mutex_init(&mtty->mutex);
	init_waitqueue_head(&mtty->write_wait);
	init_waitqueue_head(&mtty->read_wait);
	INIT_WORK(&mtty->rx_work, uart_rx_work);

	if (port->cons && port->dev)
		of_console_check(port->dev->of_node, port->cons->name, index);

	ret = uart_configure_port(drv, state, port);
	/*
	* We don't support setserial so no point registering a nonexistent
	* device . Silently ignore this port if not present.
	*/
	if (ret) {
		ret = 0;
		goto out;
	}

	state->port.console = uart_console(port);

	cdev_init(&mtty->cdev, &minitty_fops);
	mtty->cdev.owner = drv->owner;
	ret = cdev_add(&mtty->cdev, devt, 1);
	if (ret)
		goto out;
	mtty->dev = device_create(minitty_class, port->dev, devt, mtty,
				"%s%d", drv->dev_name, index);
	if (IS_ERR(mtty->dev)) {
		ret = PTR_ERR(mtty->dev);
		goto err_cdev_del;
	}

	return 0;

err_cdev_del:
	cdev_del(&mtty->cdev);
out:
	return ret;
}
EXPORT_SYMBOL(uart_add_one_port);

/**
*	uart_remove_one_port - detach a driver defined port structure
*	@drv: pointer to the uart low level driver structure for this port
*	@port: uart port structure for this port
*
*	This unhooks the specified port structure from the core driver.
*	No further calls will be made to the low-level code for this port.
*/
int uart_remove_one_port(struct uart_driver *drv, struct uart_port *port)
{
	unsigned int index = port->line;
	dev_t devt = MKDEV(drv->major, drv->minor) + index;
	struct minitty_data *mtty;
	struct uart_state *state;

	mtty = container_of(drv->state, typeof(*mtty), state) + index;
	state = &mtty->state;
	BUG_ON(state != port->state);

	device_destroy(minitty_class, devt);
	cdev_del(&mtty->cdev);

	if (uart_console(port))
		unregister_console(port->cons);

	if (port->type != PORT_UNKNOWN && port->ops->release_port)
		port->ops->release_port(port);
	port->type = PORT_UNKNOWN;
	state->uart_port = NULL;

	return 0;
}
EXPORT_SYMBOL(uart_remove_one_port);

/**
*	uart_register_driver - register a driver with the uart core layer
*	@drv: low level driver structure
*
*	Register a uart driver. The per-port structures should be
*	registered using uart_add_one_port after this call has succeeded.
*/
int uart_register_driver(struct uart_driver *drv)
{
	struct minitty_data *mtty;
	int ret;

	BUG_ON(drv->state);

	mtty = kzalloc(sizeof(*mtty) * drv->nr, GFP_KERNEL);
	if (!mtty)
		return -ENOMEM;

	if (!drv->major) {
		dev_t devt;
		ret = alloc_chrdev_region(&devt, drv->minor, drv->nr, drv->driver_name);
		drv->major = MAJOR(devt);
		drv->minor = MINOR(devt);
	} else {
		dev_t devt = MKDEV(drv->major, drv->minor);
		ret = register_chrdev_region(devt, drv->nr, drv->driver_name);
	}
	if (ret < 0)
		goto err;

	drv->state = &mtty->state;
	return 0;

err:
	kfree(mtty);
	return ret;
}
EXPORT_SYMBOL(uart_register_driver);

/**
*	uart_unregister_driver - remove a driver from the uart core layer
*	@drv: low level driver structure
*
*	Remove all references to a driver from the core driver.  The low
*	level driver must have removed all its ports via the
*	uart_remove_one_port() if it registered them with uart_add_one_port().
*/
void uart_unregister_driver(struct uart_driver *drv)
{
	dev_t devt = MKDEV(drv->major, drv->minor);
	struct minitty_data *mtty;

	unregister_chrdev_region(devt, drv->nr);
	mtty = container_of(drv->state, typeof(*mtty), state);
	drv->state = NULL;
	kfree(mtty);
}
EXPORT_SYMBOL(uart_unregister_driver);

struct tty_struct *tty_port_tty_get(struct tty_port *port)
{
	return NULL;
}
EXPORT_SYMBOL(tty_port_tty_get);

void do_SAK(struct tty_struct *tty)
{
}
EXPORT_SYMBOL(do_SAK);

struct tty_driver *uart_console_device(struct console *co, int *index)
{
	return NULL;
}

static struct cdev console_cdev;

static char *minitty_devnode(struct device *dev, umode_t *mode)
{
	if (!mode)
		return NULL;
	if (dev->devt == MKDEV(TTYAUX_MAJOR, 0) ||
			dev->devt == MKDEV(TTYAUX_MAJOR, 2))
		*mode = 0666;
	return NULL;
}

static int __init minitty_class_init(void)
{
	minitty_class = class_create(THIS_MODULE, "tty");
	if (IS_ERR(minitty_class))
		return PTR_ERR(minitty_class);
	minitty_class->devnode = minitty_devnode;
	return 0;
}
postcore_initcall(minitty_class_init);

int __init minitty_init(void)
{
	dev_t devt = MKDEV(TTYAUX_MAJOR, 1);
	cdev_init(&console_cdev, &minitty_fops);
	if (cdev_add(&console_cdev, devt, 1) ||
		register_chrdev_region(devt, 1, "/dev/console") < 0)
		panic("Couldn't register /dev/console driver\n");
	device_create(minitty_class, NULL, devt, NULL, "console");
	return 0;
}
device_initcall(minitty_init);
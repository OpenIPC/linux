#include <linux/types.h>

#include "8250.h"

struct ax8250_port_data {
	/* Port properties */
	int			line;

	/* DMA operations */
	struct uart_8250_dma	dma;

	/* Hardware configuration */
	u8			dlf_size;
};

void ax8250_setup_port(struct uart_port *p);
extern void serial8250_update_uartclk(struct uart_port *port, unsigned int uartclk);


static struct uart_driver sg_ms_uart;

/* Serial Console Structure Definition */
static struct console sg_ms_serial_console =
{
    .name  = "ttyS",
    .write = ms_serial_console_write,
    .setup = ms_serial_console_setup,
    .flags = CON_PRINTBUFFER,
    .index = 0,
    .data = &sg_ms_uart,
};


static int __init ms_early_console_init(void)
{
    /* Register UART console */
    register_console(&sg_ms_serial_console);

    return 0;
}

console_initcall(ms_early_console_init);

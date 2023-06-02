/*
 * fh_serial.h
 *
 *  Created on: Jul 29, 2014
 *      Author: duobao
 */
#ifndef FH_SERIAL_H_
#define FH_SERIAL_H_
/****************************************************************************
 * #include section
 *	add #include here if any
 ***************************************************************************/

/****************************************************************************
 * #define section
 *	add constant #define here if any
 ***************************************************************************/

#define lift_shift_bit_num(bit_num)			(1<<bit_num)
#define ISR_NUMBER0							(30)
#define ISR_NUMBER1							(31)
#define ISR_NUMBER2							(35)
#define UART0_PORT							0
#define UART1_PORT							1
#define UART2_PORT							2
#define	UART_MAX_NUM						3
#define UART_DATA_ARRIVED					1
#define UART_LCR_DLAB_POS        			(lift_shift_bit_num(7))

#define UART0_FIFO_SIZE							32
#define UART1_FIFO_SIZE							16
#define UART2_FIFO_SIZE							16

/****************************************************************************
* ADT section
*	add Abstract Data Type definition here
***************************************************************************/
//error status
enum {
	UART_CONFIG_OK = 0,
	UART_CONFIG_FIFO_OK = 0,
	UART_CONFIG_LINE_OK = 0,
	UART_CONFIG_DIVISOR_OK = 0,
	UART_WRITE_DATA_OK = 0,
	UART_READ_DATA_OK = 0,
	UART_CLEAR_ERROR_OK = 0,
	UART_RESET_RX_POOL_OK = 0,
	UART_CLEAR_RX_DATA_READY_OK = 0,
	UART_INIT_OK = 0,
	UART_CONFIG_PARA_ERROR = lift_shift_bit_num(0),
	UART_CONFIG_FIFO_ERROR = lift_shift_bit_num(1),
	UART_IS_BUSY = lift_shift_bit_num(2),
	UART_DW_FIFO_OVERFLOW = lift_shift_bit_num(3),  //dw rxfifo overflow ,maybe rxisr is closed or main clock is too low
	UART_SW_FIFO_OVERFLOW = lift_shift_bit_num(4),	//soft rxfifo overflow , maybe main clk is too low
	UART_PARITY_ERROR = lift_shift_bit_num(5),
	UART_FRAME_ERROR = lift_shift_bit_num(6),
	UART_BREAK_ERROR = lift_shift_bit_num(7),
	UART_FIFO_EMPTY = lift_shift_bit_num(8),
};

//interrupt enable
typedef enum enum_uart_irq {
	UART_INT_PTIME_POS = (lift_shift_bit_num(7)),
	UART_INT_EDSSI_POS = (lift_shift_bit_num(3)),
	UART_INT_ELSI_POS  = (lift_shift_bit_num(2)),
	UART_INT_ETBEI_POS = (lift_shift_bit_num(1)),
	UART_INT_ERBFI_POS = (lift_shift_bit_num(0)),
	UART_INT_ALL          = 0x0f,
}uart_irq_e;

//interrupt id
enum {
	UART_INT_ID_MODEM = 		0,
	UART_INT_ID_NO_INT = 		1,
	UART_INT_ID_THR_EMPTY =  	2,
	UART_INT_ID_RECEIVE_DATA =	4,
	UART_INT_ID_RECEIVE_LINE =	6,
	UART_INT_ID_BUSY = 			7,
	UART_INT_ID_TIME_OUT =	    12,
	UART_FIFO_IS_ENABLE =	    0xc0,
};

typedef enum enum_uart_line {
    Uart_line_5n1 = 0x00,   // 5 data bits, no parity, 1 stop bit
    Uart_line_5n1_5 = 0x04, // 5 data bits, no parity, 1.5 stop bits
    Uart_line_5e1 = 0x18,   // 5 data bits, even parity, 1 stop bit
    Uart_line_5e1_5 = 0x1c, // 5 data bits, even parity, 1.5 stop bits
    Uart_line_5o1 = 0x08,   // 5 data bits, odd parity, 1 stop bit
    Uart_line_5o1_5 = 0x0c, // 5 data bits, odd parity, 1.5 stop bits
    Uart_line_6n1 = 0x01,   // 6 data bits, no parity, 1 stop bit
    Uart_line_6n2 = 0x05,   // 6 data bits, no parity, 2 stop bits
    Uart_line_6e1 = 0x19,   // 6 data bits, even parity, 1 stop bit
    Uart_line_6e2 = 0x1d,   // 6 data bits, even parity, 2 stop bits
    Uart_line_6o1 = 0x09,   // 6 data bits, odd parity, 1 stop bit
    Uart_line_6o2 = 0x0d,   // 6 data bits, odd parity, 2 stop bits
    Uart_line_7n1 = 0x02,   // 7 data bits, no parity, 1 stop bit
    Uart_line_7n2 = 0x06,   // 7 data bits, no parity, 2 stop bits
    Uart_line_7e1 = 0x1a,   // 7 data bits, even parity, 1 stop bit
    Uart_line_7e2 = 0x1e,   // 7 data bits, even parity, 2 stop bits
    Uart_line_7o1 = 0x0a,   // 7 data bits, odd parity, 1 stop bit
    Uart_line_7o2 = 0x0e,   // 7 data bits, odd parity, 2 stop bits
    Uart_line_8n1 = 0x03,   // 8 data bits, no parity, 1 stop bit
    Uart_line_8n2 = 0x07,   // 8 data bits, no parity, 2 stop bits
    Uart_line_8e1 = 0x1b,   // 8 data bits, even parity, 1 stop bit
    Uart_line_8e2 = 0x1f,   // 8 data bits, even parity, 2 stop bits
    Uart_line_8o1 = 0x0b,   // 8 data bits, odd parity, 1 stop bit
    Uart_line_8o2 = 0x0f    // 8 data bits, odd parity, 2 stop bits
}uart_line_e;

//rx & tx fifo config
typedef enum enum_uart_fifo {
	UART_INT_RXFIFO_DEPTH_1 = 0x01,        			//fifo enable, rx 1 byte, set rx int
	UART_INT_RXFIFO_DEPTH_QUARTER = 0x41,			//fifo enable, rx 1/4 fifo, set rx int
	UART_INT_RXFIFO_DEPTH_HALF =0x81,				//fifo enable, rx 1/2 fifo, set rx int
	UART_INT_RXFIFO_2LESS_THAN_FULL =0xc1,			//fifo enable, rx 2 less than full,  set rx int
}uart_fifo_e;

//line status
enum {
	UART_LINE_STATUS_RFE	 = (lift_shift_bit_num(7)),
	UART_LINE_STATUS_TEMT 	 = (lift_shift_bit_num(6)),
	UART_LINE_STATUS_THRE 	 = (lift_shift_bit_num(5)),
	UART_LINE_STATUS_BI	 	 = (lift_shift_bit_num(4)),
	UART_LINE_STATUS_FE 	 = (lift_shift_bit_num(3)),
	UART_LINE_STATUS_PE   	 = (lift_shift_bit_num(2)),
	UART_LINE_STATUS_OE 	 = (lift_shift_bit_num(1)),
	UART_LINE_STATUS_DR 	 = (lift_shift_bit_num(0)),
};

//uart status
enum {
	UART_STATUS_RFF	 = (lift_shift_bit_num(4)),
	UART_STATUS_RFNE = (lift_shift_bit_num(3)),
	UART_STATUS_TFE	 = (lift_shift_bit_num(2)),
	UART_STATUS_TFNF = (lift_shift_bit_num(1)),
	UART_STATUS_BUSY = (lift_shift_bit_num(0)),

};

#define 	UART_CLOCK_FREQ   	(30000000)   //15MHZ
typedef enum enum_uart_baudrate{
	BAUDRATE_9600 	= (((UART_CLOCK_FREQ/9600)+8)/16),
	BAUDRATE_19200 	= (((UART_CLOCK_FREQ/19200)+8)/16),
	BAUDRATE_38400  = (((UART_CLOCK_FREQ/38400)+8)/16),
	BAUDRATE_57600  = (((UART_CLOCK_FREQ/57600)+8)/16),
	BAUDRATE_115200 = (((UART_CLOCK_FREQ/115200)+8)/16),
	BAUDRATE_194000 = (((UART_CLOCK_FREQ/194000)+8)/16),
}uart_baudrate_e;

typedef struct _fh_uart_info {
	const char * name;
	unsigned int irq_num;
	unsigned char __iomem	*base_add;
	uart_baudrate_e	baudrate;
	uart_line_e		line_ctrl;
	uart_fifo_e		fifo_ctrl;
	struct platform_device *dev;

}fh_uart_info;

struct fh_uart_port {
	fh_uart_info	fh_info;
	struct uart_port port;
};

#endif /* FH_SERIAL_H_ */

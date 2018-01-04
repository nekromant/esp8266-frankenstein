//Stupid bit of code that does the bare minimum to make os_printf work.

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "uart_hw.h"
#include "uart.h"

static void ICACHE_FLASH_ATTR stdoutUartTxd(char c) {
	//Wait until there is room in the FIFO
	// while (((READ_PERI_REG(UART_STATUS(0))>>UART_TXFIFO_CNT_S)&UART_TXFIFO_CNT)>=126) ;
	//Send the character
	// WRITE_PERI_REG(UART_FIFO(0), c);
}

static void ICACHE_FLASH_ATTR stdoutPutchar(char c) {
	//convert \n -> \r\n
	if (c=='\n') stdoutUartTxd('\r');
	stdoutUartTxd(c);
}

void stdoutInit() {
	/*
	//Enable TxD pin
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);

	//Set baud rate and other serial parameters to 115200,n,8,1
	uart_div_modify(0, UART_CLK_FREQ/BIT_RATE_115200);
	WRITE_PERI_REG(UART_CONF0(0), (STICK_PARITY_DIS)|(ONE_STOP_BIT << UART_STOP_BIT_NUM_S)|(EIGHT_BITS << UART_BIT_NUM_S));

	//Reset tx & rx fifo
	SET_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
	CLEAR_PERI_REG_MASK(UART_CONF0(0), UART_RXFIFO_RST|UART_TXFIFO_RST);
	//Clear pending interrupts
	WRITE_PERI_REG(UART_INT_CLR(0), 0xffff);

	//Install our own putchar handler
	*/
	os_install_putc1((void *)stdoutPutchar);

}



/*
typedef struct uart_param {
	int uart_baud_rate;
	int	uart_parity_mode;
	int	uart_stop_bit;
	int	uart_xfer_bit;				
} uart_param_t;

typedef void (*uart_rx_handler_t)(char);


void ICACHE_FLASH_ATTR uart0_init(uart_param_t* param, uart_rx_handler_t rx_handler)
{
    portENTER_CRITICAL();

    _rx_handler = rx_handler;

    PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    WRITE_PERI_REG(UART_CLKDIV(UART0), UART_CLK_FREQ / (param->uart_baud_rate));
    WRITE_PERI_REG(UART_CONF0(UART0), param->uart_parity_mode | (param->uart_stop_bit << UART_STOP_BIT_NUM_S) | (param->uart_xfer_bit << UART_BIT_NUM_S));

    // Clear RX and TX FIFO
    SET_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);
	
    // Clear all interrupt
    WRITE_PERI_REG(UART_INT_CLR(UART0), 0x1ff);

    _xt_isr_attach(ETS_UART_INUM, _uart0_rx_isr);

    // RX interrupt conditions: FIFO full (1 char)
    SET_PERI_REG_MASK(UART_CONF1(UART0), ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));

    // Enable RX Interrupt
    SET_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA | UART_FRM_ERR_INT_ENA);

    _xt_isr_unmask(1 << ETS_UART_INUM);  // ETS_UART_INTR_ENABLE();
    portEXIT_CRITICAL();
	
	
    _xt_isr_attach(ETS_UART_INUM, _uart0_rx_isr);
	_xt_isr_unmask(1 << ETS_UART_INUM);
}

LOCAL void _uart0_rx_isr(void)
{
    char ch;

    // Frame error, ignore and continue
    if (UART_FRM_ERR_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_FRM_ERR_INT_ST))
    {
        // printf("FRM_ERR\r\n");
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
    }
    // RX FIFO FULL
    else if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST))
    {
        //os_printf("RX_FULL\r\n");
        while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S))
        {
            ch = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
            if (_rx_handler)
                _rx_handler(ch);
        }
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
    }
}

*/

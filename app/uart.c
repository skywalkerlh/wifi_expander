#include "bg_types.h"
//#include "native_gecko.h"
//#include "app.h"
#include "uartdrv.h"
//#include "efr32_api.h"
#include "uart.h"
#include "buf_factory.h"
#include "rtt.h"
#include "uart_ptl.h"
#include "os.h"
#include "bg_types.h"
#include "rtos_gecko.h"
#include "uartdrv.h"
//#include "efr32_api.h"
#include "uart.h"
#include "buf_factory.h"
#include "uart_communicate.h"
#include "rtt.h"
static void uart_rx_callback(UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount);
static void uart_tx_callback(UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount);

void APP_USART_IRQ_NAME(void);
void APP_USART_TX_IRQ_NAME(void);

// temporary buffer for receiving data from UART
#define  BUFF_SIZE  1024
static uint8_t rxbuf[BUFF_SIZE];

unsigned int volatile p_in, p_out;//接收循环buffer下标

static  UARTDRV_HandleData_t handleData;


#define APP_USART_IRQ_NAME    USART0_RX_IRQHandler
#define APP_USART_IRQn        USART0_RX_IRQn


static void uart_rx_callback(UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount);
static void uart_tx_callback(UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount);

void APP_USART_IRQ_NAME(void);

// Define receive/transmit operation queues
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, rxBufferQueue);
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, txBufferQueue);

struct buffer_factory *g_uart_send_buf = NULL;
struct buffer_factory *g_uart_recv_buf = NULL;
extern uart_recv_buf_t UartRecvBuf;

extern OS_SEM app_uart_send_complete_sem;

static  UARTDRV_HandleData_t handleData;
static  UARTDRV_Handle_t uart_handle = &handleData;

// Configuration for USART0, location 0
#define APP_UART                                   \
{                                                 \
	USART0,                                       \
	115200,                                       \
	_USART_ROUTELOC0_TXLOC_LOC0,                  \
	_USART_ROUTELOC0_RXLOC_LOC0,                  \
	usartStopbits1,                               \
	usartNoParity,                                \
	usartOVS16,                                   \
	false,                                        \
	uartdrvFlowControlNone,                       \
	gpioPortA,                                    \
	4,                                            \
	gpioPortA,                                    \
	5,                                            \
	(UARTDRV_Buffer_FifoQueue_t *)&rxBufferQueue, \
	(UARTDRV_Buffer_FifoQueue_t *)&txBufferQueue, \
	_USART_ROUTELOC1_CTSLOC_LOC0,                 \
	_USART_ROUTELOC1_RTSLOC_LOC0                  \
}


#if BSP_UARTNCP_USART_PORT == HAL_SERIAL_PORT_USART0
// USART0
#define APP_USART_UART        USART0
#define APP_USART_CLK         cmuClock_USART0
#define APP_USART_IRQ_NAME    USART0_RX_IRQHandler
#define APP_USART_IRQn        USART0_RX_IRQn
#define APP_USART_TX_IRQ_NAME   USART0_TX_IRQHandler
#define APP_USART_TX_IRQn       USART0_TX_IRQn
#define APP_USART_USART       1
#endif

int32_t app_uart_init(void)
{
	struct Buffer *p_buf;
	// Initialize driver handle
	UARTDRV_InitUart_t initData = APP_UART;
	UARTDRV_InitUart(uart_handle, &initData);

	CORE_SetNvicRamTableHandler(APP_USART_IRQn, (void *)APP_USART_IRQ_NAME);
//	CORE_SetNvicRamTableHandler(APP_USART_TX_IRQn, (void *)APP_USART_TX_IRQ_NAME);
	NVIC_ClearPendingIRQ(APP_USART_IRQn);           // Clear pending RX interrupt flag in NVIC
//	NVIC_ClearPendingIRQ(APP_USART_TX_IRQn);        // Clear pending TX interrupt flag in NVIC
	NVIC_EnableIRQ(APP_USART_IRQn);
//	NVIC_EnableIRQ(APP_USART_TX_IRQn);

	//Setup RX timeout
	uart_handle->peripheral.uart->TIMECMP1 = USART_TIMECMP1_RESTARTEN
										 | USART_TIMECMP1_TSTOP_RXACT
										 | USART_TIMECMP1_TSTART_RXEOF
										 | (0x30 << _USART_TIMECMP1_TCMPVAL_SHIFT);

//	//IRQ
//	USART_IntClear(uart_handle->peripheral.uart, _USART_IF_MASK);       // Clear any USART interrupt flags
//	USART_IntEnable(uart_handle->peripheral.uart, USART_IF_TXIDLE | USART_IF_TCMP1| USART_IF_RXDATAV); // USART_IF_TCMP1    USART_IF_RXDATAV
	//IRQ
	USART_IntClear(uart_handle->peripheral.uart, _USART_IF_MASK);// Clear any USART interrupt flags
	USART_IntEnable(uart_handle->peripheral.uart, USART_IF_TCMP1);

	/* RX the next command header*/
	UARTDRV_Receive(uart_handle, rxbuf, BUFF_SIZE, NULL);// BUFF_SIZE

	//缓冲初始化
	if(create_buf_factory(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, sizeof(uart_send_format_t), &g_uart_send_buf) != 0)
	{
		DBG("file:%s line:%d\n",__FILE__, __LINE__);
		return -1;
	}

//	if(create_buf_factory(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, sizeof(uart_cmd_format_t), &g_uart_recv_buf) != 0)
//	{
//		DBG("file:%s line:%d\n",__FILE__, __LINE__);
//		return -1;
//	}
//
//	//开始接收数据
//	p_buf = (void*)buf_factory_produce(g_uart_recv_buf);
//	if(p_buf == NULL)
//	{
//		DBG("file:%s line:%d\n",__FILE__, __LINE__);
//		return -1;
//	}
////	DBG("file:%s line:%d\n",__FILE__, __LINE__);
////	DBG("p_buf->memory1 = %4x\n", p_buf->memory);
//	UARTDRV_Receive(uart_handle, p_buf->memory, sizeof(uart_cmd_format_t), NULL);// BUFF_SIZE
////	UARTDRV_Receive(uart_handle, rxbuf, BUFF_SIZE, NULL);// BUFF_SIZE

	return 0;
}

Ecode_t ret = 0;
static uint8_t* buffer = NULL;
static uint32_t received = 0;
static uint32_t remaining = 0;
void APP_USART_IRQ_NAME()
{

	struct Buffer *p_buf;
	RTOS_ERR  err;
	if (uart_handle->peripheral.uart->IF & USART_IF_TCMP1)
	{
		//stop the timer
		uart_handle->peripheral.uart->TIMECMP1 &= ~_USART_TIMECMP1_TSTART_MASK;
		uart_handle->peripheral.uart->TIMECMP1 |= USART_TIMECMP1_TSTART_RXEOF;
		USART_IntClear(uart_handle->peripheral.uart, USART_IF_TCMP1);


		UARTDRV_GetReceiveStatus(uart_handle, &buffer, &received, &remaining);
//		DBG(L_RED"buffer: %04X received:  %04X remaining:   %04X\r\n"D_NONE,buffer, received, remaining);

		if (buffer && received >= 1)
		{
//			DBG(YELLOW);
//			for(int i = 0; i < received; i++)
//			{
//				DBG(" %02X",buffer[i]);
//			}
//			DBG("\r\n"D_NONE);
			uart_recv(buffer, received);
			//abort receive operation
			UARTDRV_Abort(uart_handle, uartdrvAbortReceive);
			//enqueue next receive buffer
//			p_buf = (void*)buf_factory_produce(g_uart_recv_buf);
////			DBG("file:%s line:%d\n",__FILE__, __LINE__);
////			DBG("p_buf->memory2 = %4x\n", p_buf->memory);
//			UARTDRV_Receive(uart_handle, p_buf->memory, sizeof(uart_cmd_format_t), NULL);
			ret = UARTDRV_Receive(uart_handle, rxbuf, BUFF_SIZE, NULL); // BUFF_SIZE
//			DBG("file:%s line:%d\n",__FILE__, __LINE__);
//			DBG("buffer1 = %4x\n", buffer);
			OSSemPost(&(UartRecvBuf.sem),
					OS_OPT_POST_1,
					&err);
//			OSQPost(&app_uart_recv_msg,
//					buffer,
//					received,
//					OS_OPT_POST_FIFO,
//					&err);

		}
	}
}

void APP_USART_TX_IRQ_NAME()
{
	// 	DBG(L_RED"TX: before.uart->IF: %04X\r\n"D_NONE,handle->peripheral.uart->IF);
	if (uart_handle->peripheral.uart->IF & USART_IF_TXIDLE)
	{
		//    gecko_external_signal(NCP_USART_UPDATE_SIGNAL);
		USART_IntClear(uart_handle->peripheral.uart, USART_IF_TXIDLE);
	}
	//   DBG(L_RED"TX: after.uart->IF: %04X\r\n"D_NONE,handle->peripheral.uart->IF);
}



static void uart_tx_callback(UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount)
{
	RTOS_ERR  err;
//	buf_factory_recycle(0, data, g_uart_send_buf);

//	DBG("file:%s line:%d\n",__FILE__, __LINE__);
	OSSemPost(&app_uart_send_complete_sem,
			OS_OPT_POST_1,
			&err);
}

uint32_t app_usart_transmit(uint8_t* data, uint32_t len)
{
//	DBG("file:%s line:%d\n",__FILE__, __LINE__);
	return UARTDRV_Transmit(uart_handle, data, len, uart_tx_callback);

//	return UARTDRV_TransmitB(uart_handle, data, len, uart_tx_callback);
}





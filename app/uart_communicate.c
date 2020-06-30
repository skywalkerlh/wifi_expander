/*
 * uart_communicate.c
 *
 *  Created on: 2019年10月12日
 *      Author: Administrator
 */
#include <string.h>
#include "os.h"
#include "rtos_utils.h"
#include "uart.h"
#include "uart_ptl.h"
#include "uartdrv.h"
#include "list.h"
#include "uart_communicate.h"
#include "buf_factory.h"
#include "rtt.h"

#define  App_Uart_Send_Task_PRIO      12u//App_Uart_Send_Task_PRIO必须低于App_Uart_Poll_Task_PRIO
//#define  App_Uart_Recv_Task_PRIO      13u
#define  App_Uart_Poll_Task_PRIO      10u
#define  App_Uart_Parse_Task_PRIO      11u

#define  App_Uart_Send_Task_STK_SIZE  256u
OS_TCB   App_Uart_Send_Task_TCB;
static  CPU_STK  App_Uart_Send_Task_Stk[App_Uart_Send_Task_STK_SIZE];

//#define  App_Uart_Recv_Task_STK_SIZE  256u
//static  OS_TCB   App_Uart_Recv_Task_TCB;
//static  CPU_STK  App_Uart_Recv_Task_Stk[App_Uart_Recv_Task_STK_SIZE];

#define  App_Uart_Poll_Task_STK_SIZE  256u
OS_TCB   App_Uart_Poll_Task_TCB;
static  CPU_STK  App_Uart_Poll_Task_Stk[App_Uart_Poll_Task_STK_SIZE];

#define  App_Uart_Parse_Task_STK_SIZE  512u
OS_TCB   App_Uart_Parse_Task_TCB;
static  CPU_STK  App_Uart_Parse_Task_Stk[App_Uart_Parse_Task_STK_SIZE];

OS_MUTEX uart_poll_mutex;
OS_Q app_uart_send_msg;
OS_SEM app_uart_send_complete_sem;
//OS_Q app_uart_recv_msg;

struct list_head g_uart_send_list;
extern struct buffer_factory *g_uart_send_buf;
//extern struct buffer_factory *g_uart_recv_buf;

void App_Uart_Send_Task(void *p_arg)
{
	CPU_TS ts;
	RTOS_ERR  err;
	uart_send_format_t *Data_OutQ;
//	void *p;
	uint16_t Data_len;

	while(1)
	{
		Data_OutQ = OSQPend(&app_uart_send_msg,
					0,
					OS_OPT_PEND_BLOCKING,
					&Data_len,
					&ts,
					&err);
		app_usart_transmit((uint8_t*)(&Data_OutQ->cmd), Data_OutQ->cmd_len);

		OSSemPend(&app_uart_send_complete_sem,
				0,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);

//		if(Data_OutQ->poll.free_flag)
		if((Data_OutQ->poll.free_flag) && (Data_OutQ->poll.type == 0))
		{
//			DBG("p_buf2 = %4x\r\n", Data_OutQ->buf);
			memset(&(Data_OutQ->poll), 0, sizeof(uart_poll_t));
			buf_factory_recycle(0, Data_OutQ->buf, g_uart_send_buf);
		}
	}
}



uart_recv_buf_t UartRecvBuf = {
		.p_in = 0,
		.p_out = 0
};

void uart_recv(uint8_t* data, uint16_t len)
{
//	CPU_TS ts;
//	RTOS_ERR  err;
	uint32_t i = 0;

//	OSMutexPend(&UartRecvData.mutex,
//				0,
//				OS_OPT_PEND_BLOCKING,
//				&ts,
//				&err);

//				DBG(YELLOW);
//				for(int i = 0; i < len; i++)
//				{
//					DBG(" %02X",data[i]);
//				}
//				DBG("\r\n"D_NONE);

	for(i = 0; i < len; i++)
	{
		UartRecvBuf.data[UartRecvBuf.p_in] = data[i];
		UartRecvBuf.p_in++;
		if(UartRecvBuf.p_in >= UART_RECV_BUFF_SIZE)
		{
			UartRecvBuf.p_in = 0;
		}
	}
//	OSMutexPost(&UartRecvData.mutex,
//				OS_OPT_POST_NONE,
//				&err);
}



//void App_Uart_Recv_Task(void *p_arg)
//{
//	CPU_TS ts;
//	RTOS_ERR  err;
////	uart_recv_format_t *Pdata;
//	void *memory;
//	uint16_t Data_len;
//
//	while(1)
//	{
//		memory = OSQPend(&app_uart_recv_msg,
//					0,
//					OS_OPT_PEND_BLOCKING,
//					&Data_len,
//					&ts,
//					&err);
//
//		uart_recv((uint8_t*)memory, Data_len);
////		DBG("file:%s line:%d\n",__FILE__, __LINE__);
////		DBG("buffer2 = %4x\n", memory);
//
//		buf_factory_recycle(0, memory, g_uart_recv_buf);
//	}
//}


void App_Uart_Parse_Task(void *p_arg)
{
	CPU_TS ts;
	RTOS_ERR  err;

	while(1)
	{
//	    OSTimeDlyHMSM(0, 0, 0, 5,
//	                  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
//	                  &err);
		OSSemPend(&(UartRecvBuf.sem),
				5,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);


		if(!(uart_get_data()))
		{
			continue;
		}
		uart_protocol_parse();
	}
}


void App_Uart_Poll_Task(void *p_arg)
{
	CPU_TS ts;
	RTOS_ERR  err;
	struct list_head *p;
	static uart_send_format_t *parray;

	while(1)
	{
		OSTimeDlyHMSM(0, 0, 0, 5,
	                  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
	                  &err);

		OSMutexPend(&uart_poll_mutex,
					0,
					OS_OPT_PEND_BLOCKING,
					&ts,
					&err);

		p = g_uart_send_list.next;

		while(p != &g_uart_send_list)
		{
			parray = (uart_send_format_t *)(p->owner);
			if(parray->poll.type == 0)//如果是数据回应
			{
				parray->poll.free_flag = 1;
				OSQPost(&app_uart_send_msg,
						parray,
						parray->cmd_len,
						OS_OPT_POST_FIFO,
						&err);

				p = p->next;
				list_del(p->prev);
//				continue;
				break;// OSQPost连续发送，OSQpend会丢失，所以每次只处理list中一个发送动作（PS:ztmcd）
			}

			else if(parray->poll.type == 1)//如果是主动发送
			{
				if(parray->poll.times == 0)//首次发送
				{
					parray->poll.times++;
					OSQPost(&app_uart_send_msg,
							parray,
//							sizeof(uart_send_format_t),
							parray->cmd_len,
							OS_OPT_POST_FIFO,
							&err);
					break;// OSQPost连续发送，OSQpend会丢失，所以每次只处理list中一个发送动作（PS:ztmcd）
				}
				else
				{
					if(parray->poll.free_flag)//如果已经收到回应
					{
						p = p->next;
						list_del(p->prev);
						memset(&(parray->poll), 0, sizeof(uart_poll_t));
						buf_factory_recycle(0, parray->buf, g_uart_send_buf);
						continue;
					}

					parray->poll.delay += 5;
					if(parray->poll.delay >= 500)
					{
						parray->poll.delay = 0;
						parray->poll.times++;
						if(parray->poll.times>3)
						{
							p = p->next;
							list_del(p->prev);
							memset(&(parray->poll), 0, sizeof(uart_poll_t));
							buf_factory_recycle(0, parray->buf, g_uart_send_buf);
							continue;
						}
						OSQPost(&app_uart_send_msg,
								parray,
//								sizeof(uart_send_format_t),
								parray->cmd_len,
								OS_OPT_POST_FIFO,
								&err);
						break;// OSQPost连续发送，OSQpend会丢失，所以每次只处理list中一个发送动作（PS:ztmcd）
					}
				}
			}
			p = p->next;
		}

		OSMutexPost(&uart_poll_mutex,
					OS_OPT_POST_NONE,
					&err);
	}
}



void uart_context_init(void)
{
	RTOS_ERR  err;

	if(app_uart_init() != 0)
	{
		DBG("app uart init error!\n");
	}

	//链表初始化
	init_list_head(&g_uart_send_list);

	OSMutexCreate(&uart_poll_mutex,
				"uart_poll_mutex",
				&err);

//	OSMutexCreate(&UartRecvData.mutex,
//				"uart_recv_mutex",
//				&err);

	OSSemCreate(&(UartRecvBuf.sem),
				"UartRecvData_sem",
				0,
				&err);

	OSSemCreate(&app_uart_send_complete_sem,
				"app_uart_send_complete_sem",
				0,
				&err);

	OSQCreate(&app_uart_send_msg,
			"app_uart_send_msg",
			EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS+1,
			&err);

//	OSQCreate(&app_uart_recv_msg,
//			"app_uart_recv_msg",
//			EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS+1,
//			&err);


	OSTaskCreate(&App_Uart_Send_Task_TCB,
				"App_Uart_Send_Task",
				App_Uart_Send_Task,
				0,
				App_Uart_Send_Task_PRIO,
				&App_Uart_Send_Task_Stk[0],
				(App_Uart_Send_Task_STK_SIZE / 10u),
				App_Uart_Send_Task_STK_SIZE,
				0,
				0,
				0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				&err);
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

//	OSTaskCreate(&App_Uart_Recv_Task_TCB,
//				"App_Uart_Recv_Task",
//				App_Uart_Recv_Task,
//				0,
//				App_Uart_Recv_Task_PRIO,
//				&App_Uart_Recv_Task_Stk[0],
//				(App_Uart_Recv_Task_STK_SIZE / 10u),
//				App_Uart_Recv_Task_STK_SIZE,
//				0,
//				0,
//				0,
//				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
//				&err);
//	/*   Check error code.                                  */
//	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	OSTaskCreate(&App_Uart_Poll_Task_TCB,
				"App_Uart_Poll_Task",
				App_Uart_Poll_Task,
				0,
				App_Uart_Poll_Task_PRIO,
				&App_Uart_Poll_Task_Stk[0],
				(App_Uart_Poll_Task_STK_SIZE / 10u),
				App_Uart_Poll_Task_STK_SIZE,
				0,
				0,
				0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				&err);
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	OSTaskCreate(&App_Uart_Parse_Task_TCB,
				"App_Uart_Parse_Task",
				App_Uart_Parse_Task,
				0,
				App_Uart_Parse_Task_PRIO,
				&App_Uart_Parse_Task_Stk[0],
				(App_Uart_Parse_Task_STK_SIZE / 10u),
				App_Uart_Parse_Task_STK_SIZE,
				0,
				0,
				0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				&err);
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

}

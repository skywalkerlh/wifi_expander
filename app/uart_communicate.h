/*
 * uart_communicate.h
 *
 *  Created on: 2019年10月12日
 *      Author: Administrator
 */

#ifndef UART_COMMUNICATE_H_
#define UART_COMMUNICATE_H_

#include "list.h"


#define UART_PROCOTOL_MAX_LEN    400

typedef struct
{
	uint8_t head;
	uint8_t length_msb;
	uint8_t length_lsb;
	uint8_t sta_param;
	uint8_t sn;
	uint8_t pdt_type;
	uint8_t cmd_id;
}m_cmd_header_t;


typedef struct
{
	uint32_t   delay;
	uint32_t   times;
	uint32_t   type;    //0:消息应答      //1:主动发送
	uint32_t   free_flag;
}uart_poll_t;

typedef struct
{
	m_cmd_header_t  header;
	uint8_t data[UART_PROCOTOL_MAX_LEN];
} uart_cmd_format_t;


typedef struct
{
	struct list_head list;
	struct Buffer *buf;
	uart_poll_t poll;
	uart_cmd_format_t cmd;
	uint32_t cmd_len;
}uart_send_format_t;

typedef struct
{
	void * memory;
	uint32_t data_len;
}uart_recv_format_t;


#define UART_RECV_BUFF_SIZE  1024
typedef struct
{
	uint8_t data[UART_RECV_BUFF_SIZE];
	volatile uint32_t  p_in, p_out;//接收循环buffer下标
	OS_SEM  sem;
}uart_recv_buf_t;



void uart_context_init(void);
void uart_recv(uint8_t* data, uint16_t len);

#endif /* UART_COMMUNICATE_H_ */

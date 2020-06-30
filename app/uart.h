/*
 * uart.h
 *
 *  Created on: 2019Äê10ÔÂ12ÈÕ
 *      Author: Administrator
 */

#ifndef UART_H_
#define UART_H_





int32_t app_uart_init(void);
uint32_t app_usart_transmit(uint8_t* data, uint32_t len);

#endif /* UART_H_ */

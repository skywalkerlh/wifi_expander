/*
 * ble.h
 *
 *  Created on: 2019年10月12日
 *      Author: Administrator
 */

#ifndef BLE_H_
#define BLE_H_

#include "os.h"
#include "mble_api.h"


#define BLE_CONNECTION_OPEND_EVT      0x0001
#define BLE_CONNECTION_CLOSED_EVT     0x0002

#define NOTIFY_ARRAY_SIZE  1024
typedef struct
{
	uint8_t data[NOTIFY_ARRAY_SIZE];
	uint32_t pb_in;
	uint32_t pb_out;
	OS_MUTEX mutex;
}m_notify_array_t;

#define BLE_RECV_BUFF_SIZE  1024
typedef struct
{
	uint8_t data[BLE_RECV_BUFF_SIZE];
	volatile uint32_t  p_in, p_out;//接收循环buffer下标
	OS_SEM  sem;
}ble_recv_array_t;

void ble_context_init(void);
int32_t advertising_init(uint8_t conf_stat_flag, uint8_t *resp, mble_addr_t dev_mac);
void advertising_start(void);
uint32_t ptr_minus(uint32_t ptr, uint32_t off, uint32_t buf_size);
uint32_t ptr_plus(uint32_t ptr, uint32_t off, uint32_t buf_size);


#endif /* BLE_H_ */

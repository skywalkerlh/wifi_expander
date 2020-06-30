/*
 * ble.c
 *
 *  Created on: 2019年10月12日
 *      Author: Administrator
 */
#include <stdio.h>

#include "rtos_bluetooth.h"
#include "rtos_gecko.h"
//GATT DB
#include "gatt_db.h"
#include "mble_api.h"
#include "rtos_utils.h"
#include "bsp.h"
#include "buf_factory.h"
#include "uart_ptl.h"
#include "rtt.h"
#include "uart.h"
#include "ble.h"

////不能修改------------------------------------------------------------------------------------------
	#define  APP_CFG_TASK_BLUETOOTH_LL_PRIO     3u
	#define  APP_CFG_TASK_BLUETOOTH_STACK_PRIO  4u
	#define  APP_CFG_TASK_APPLICATION_PRIO      5u


	// Event Handler Task
	OS_TCB   ApplicationTaskTCB;
	#define  APPLICATION_STACK_SIZE  (1024 / sizeof(CPU_STK))
	CPU_STK  ApplicationTaskStk[APPLICATION_STACK_SIZE];
//-------------------------------------------------------------------------------------------------


#define  Ble_Notify_Task_PRIO      9u
//ble notify task
#define  Ble_Notify_Task_STK_SIZE  (1024 / sizeof(CPU_STK))
OS_TCB   Ble_Notify_Task_TCB;
static  CPU_STK  Ble_Notify_Task_Stk[Ble_Notify_Task_STK_SIZE];


#define  App_Ble_Parse_Task_PRIO      8u
#define  App_Ble_Parse_Task_STK_SIZE  (1024 / sizeof(CPU_STK))
OS_TCB   App_Ble_Parse_Task_TCB;
static  CPU_STK  App_Ble_Parse_Task_Stk[App_Ble_Parse_Task_STK_SIZE];


#define  Ble_Event_Task_PRIO      7u
#define  Ble_Event_Task_STK_SIZE  (1024 / sizeof(CPU_STK))
OS_TCB   Ble_Event_Task_TCB;
static  CPU_STK  Ble_Event_Task_Stk[Ble_Event_Task_STK_SIZE];

OS_SEM ble_notify_ready_sem;
OS_Q ble_event_msg;


extern OS_MUTEX uart_poll_mutex;
extern struct list_head g_uart_send_list;
extern struct buffer_factory *g_uart_send_buf;
uint16 ble_mtu = 20;
uint8 ble_conn_handle = DISCONNECTION;
uint8_t ble_conn_state = 0;// 0: disconnect, 1: connected
m_notify_array_t notify_array = {
		.pb_in = 0,
		.pb_out = 0
};

ble_recv_array_t ble_recv_array = {
		.p_in = 0,
		.p_out = 0
};

uart_cmd_format_t ble_recv_buf;

#ifdef OTA
static uint8_t boot_to_dfu = 0;
#endif

/*
 * Bluetooth stack configuration
 */

#define MAX_CONNECTIONS 1
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];
/* Gecko configuration parameters (see gecko_configuration.h) */
static const gecko_configuration_t bluetooth_config =
{
  .config_flags = GECKO_CONFIG_FLAG_RTOS,
#if defined(FEATURE_LFXO)
  .sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
#else
  .sleep.flags = 0,
#endif // LFXO
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .gattdb = &bg_gattdb_data,
  .scheduler_callback = BluetoothLLCallback,
  .stack_schedule_callback = BluetoothUpdate,
#if (HAL_PA_ENABLE)
  .pa.config_enable = 1, // Set this to be a valid PA config
#if defined(FEATURE_PA_INPUT_FROM_VBAT)
  .pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#else
  .pa.input = GECKO_RADIO_PA_INPUT_DCDC,
#endif // defined(FEATURE_PA_INPUT_FROM_VBAT)
#endif // (HAL_PA_ENABLE)
  .rf.flags = GECKO_RF_CONFIG_ANTENNA,                 /* Enable antenna configuration. */
  .rf.antenna = GECKO_RF_ANTENNA,                      /* Select antenna path! */
#ifdef OTA
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
#endif
};



void BluetoothEventHandler(struct gecko_cmd_packet* evt)
{
//	CPU_TS ts;
	RTOS_ERR  err;
//	uart_cmd_format_t*  Respdata;
	uint16_t result;

	switch (BGLIB_MSG_ID(evt->header)) {

	case gecko_evt_system_boot_id:
	{
//		uint8_t simu_mac[6] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab};
//        char *resp = "Wyze Extender xxxxxx";
//        advertising_init(1, (uint8_t *)resp, simu_mac);
//        advertising_start();
	}break;

	case gecko_evt_le_connection_opened_id:
	{
		DBG("le_connection_opened! \r\n ");
		ble_conn_state = 1;
//		ble_state_notify(ble_conn_state);
		OSQPost(&ble_event_msg,
				NULL,
				BLE_CONNECTION_OPEND_EVT,
				OS_OPT_POST_FIFO,
				&err);

		ble_conn_handle = evt->data.evt_le_connection_opened.connection;

//		struct gecko_msg_le_connection_set_timing_parameters_rsp_t *ret;
		mble_gap_conn_param_t conn_params={
				.min_conn_interval = 16,
				.max_conn_interval = 32,
				.slave_latency = 0,
				.conn_sup_timeout = 200,
				.min_ce_length = 0,
				.max_ce_length = 0
		};
//		ret = gecko_cmd_le_connection_set_parameters(ble_conn_handle, 16, 32, 0, 200);
		if(mble_gap_update_conn_params(ble_conn_handle, conn_params) != MI_SUCCESS)
//		if(ret->result != bg_err_success)
		{
			DBG("le_connection_set_parameters failed. \r\n");
		}
	}break;

    case gecko_evt_le_connection_closed_id:
    {
		//Start advertisement at boot, and after disconnection
		DBG("le_connection_closed, reason: 0x%2.2x\r\n", evt->data.evt_le_connection_closed.reason);
		ble_conn_state = 0;
//		ble_state_notify(ble_conn_state);
		OSQPost(&ble_event_msg,
				NULL,
				BLE_CONNECTION_CLOSED_EVT,
				OS_OPT_POST_FIFO,
				&err);
		ble_conn_handle = DISCONNECTION;

//		gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
		result = gecko_cmd_le_gap_start_advertising(0, le_gap_user_data, le_gap_connectable_scannable)->result;
		if (result != bg_err_success)
		{
			DBG("advertising open failed!\n");
			gecko_external_signal(START_ADV_RETRY_BIT_MASK);
		}
		else
		{
			DBG("advertising open successed!\n");
		}

    }break;

    case gecko_evt_le_connection_parameters_id:
    {
    	DBG("le_connection_parameters! connection:%d, interval:%d, latency:%d, security_mode:%d, timeout:%d, txsize:%d\r\n",
    			evt->data.evt_le_connection_parameters.connection,
				evt->data.evt_le_connection_parameters.interval,
				evt->data.evt_le_connection_parameters.latency,
				evt->data.evt_le_connection_parameters.security_mode,
				evt->data.evt_le_connection_parameters.timeout,
				evt->data.evt_le_connection_parameters.txsize);
    }break;

	case gecko_evt_gatt_mtu_exchanged_id:
	{
		ble_mtu = evt->data.evt_gatt_mtu_exchanged.mtu - 3;
	}break;

	case gecko_evt_gatt_server_user_write_request_id:
	{
		int len = evt->data.evt_gatt_server_attribute_value.value.len;
		if(evt->data.evt_gatt_server_attribute_value.attribute == gattdb_Recv)
		{
			for(int i = 0; i < len; i++)
			{
				ble_recv_array.data[ble_recv_array.p_in] = evt->data.evt_gatt_server_attribute_value.value.data[i];
				ble_recv_array.p_in++;
				if(ble_recv_array.p_in >= BLE_RECV_BUFF_SIZE)
				{
					ble_recv_array.p_in = 0;
				}
			}

			OSSemPost(&(ble_recv_array.sem),
					OS_OPT_POST_1,
					&err);
		}
	}break;

    case gecko_evt_system_external_signal_id:
    {
		if (evt->data.evt_system_external_signal.extsignals & START_ADV_RETRY_BIT_MASK)
		{
			DBG("advertising retry...\n");
			result = gecko_cmd_le_gap_start_advertising(0, le_gap_user_data, le_gap_connectable_scannable)->result;
			if (result != bg_err_success)
			{
				gecko_external_signal(START_ADV_RETRY_BIT_MASK);
			}
			else
			{
				DBG("advertising open successed!\n");
			}
		}
    }break;

  }
}

/*********************************************************************************************************
 *                                             BluetoothApplicationTask()
 *
 * Description : Bluetooth Application task.
 *
 * Argument(s) : p_arg       the argument passed by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
void  BluetoothApplicationTask(void *p_arg)
{
  RTOS_ERR      os_err;
  (void)p_arg;

  while (DEF_TRUE) {
    OSFlagPend(&bluetooth_event_flags, (OS_FLAGS)BLUETOOTH_EVENT_FLAG_EVT_WAITING,
               0,
               OS_OPT_PEND_BLOCKING + OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME,
               NULL,
               &os_err);
    BluetoothEventHandler((struct gecko_cmd_packet*)bluetooth_evt);

    OSFlagPost(&bluetooth_event_flags, (OS_FLAGS)BLUETOOTH_EVENT_FLAG_EVT_HANDLED, OS_OPT_POST_FLAG_SET, &os_err);
  }
}

int32_t advertising_init(uint8_t conf_stat_flag, uint8_t *resp, mble_addr_t dev_mac)
{
	// adv data
	uint8_t adv_data[31]={0};
	uint8_t adv_len=0;
	// add flags
	adv_data[adv_len++] = 0x02;
	adv_data[adv_len++] = 0x01;
	adv_data[adv_len++] = 0x06;
	// service UUID
	adv_data[adv_len++] = 3;
	adv_data[adv_len++] = 2;
	adv_data[adv_len++] = 0x7B;
	adv_data[adv_len++] = 0xFD;
	// manufacture data
	adv_data[adv_len++] = 13;
	adv_data[adv_len++] = 0xFF;
	adv_data[adv_len++] = 0x70; // CID
	adv_data[adv_len++] = 0x08;
	adv_data[adv_len++] = 0x03; // MID
	adv_data[adv_len++] = 0x01; // PID
	adv_data[adv_len++] = dev_mac[0];
	adv_data[adv_len++] = dev_mac[1];
	adv_data[adv_len++] = dev_mac[2];
	adv_data[adv_len++] = dev_mac[3];
	adv_data[adv_len++] = dev_mac[4];
	adv_data[adv_len++] = dev_mac[5];
	adv_data[adv_len++] = conf_stat_flag;   //(conf_stat_flag & 0x01) << 7;
	DBG("conf_stat_flag be sent = %x!\n",conf_stat_flag);
	adv_data[adv_len++] = 0x00; // 版本
//	mible_addr_t dev_mac;
//	mible_gap_address_get(dev_mac);
//	adv_data[adv_len++] = dev_mac[5];
//	adv_data[adv_len++] = dev_mac[4];
//	adv_data[adv_len++] = dev_mac[3];
//	adv_data[adv_len++] = dev_mac[2];
//	adv_data[adv_len++] = dev_mac[1];
//	adv_data[adv_len++] = dev_mac[0];

	// scan resp data
	uint8_t scan_resp_data[31] = {0};
	uint8_t scan_resp_len = 0;

	if(strlen(resp) > 28)
	{
		DBG("scan resp len error! len=%d\n",strlen(resp));
		return -1;
	}
	//  //TX power
	scan_resp_data[scan_resp_len++] = 0x02;
	scan_resp_data[scan_resp_len++] = 0x0A;
	scan_resp_data[scan_resp_len++] = 0;
	//local name
	scan_resp_data[scan_resp_len] = strlen(resp)+2;
	scan_resp_data[scan_resp_len+1] = 0x09;
	strcpy(&scan_resp_data[scan_resp_len+2], resp);
	scan_resp_len += strlen(resp)+3;

	if(mble_gap_adv_data_set(adv_data,adv_len,scan_resp_data,scan_resp_len) != MI_SUCCESS)
	{
		DBG("mible_gap_adv_data_set error!\n");
		return -1;
	}

	return 0;
}

void advertising_start(void)
{
	mble_status_t reason;
	mble_gap_adv_param_t adv_param =(mble_gap_adv_param_t){
	.adv_type = MBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
	.adv_interval_min = 0x0020,
	.adv_interval_max = 0x0020,
	.ch_mask = {0},
	};

	reason = mible_gap_adv_start(&adv_param);
	if(MI_SUCCESS != reason){
		DBG("mible_gap_adv_start failed. reason: %d\r\n", reason);
	}

}


uint32_t ptr_minus(uint32_t ptr, uint32_t off, uint32_t buf_size)
{
	if(ptr >= off)
	{
		return (ptr - off);
	}
	else
	{
		return (ptr + buf_size - off);
	}
}

uint32_t ptr_plus(uint32_t ptr, uint32_t off, uint32_t buf_size)
{
	if((ptr + off) >= buf_size)
	{
		return (ptr + off) - buf_size;
	}
	else
	{
		return (ptr + off);
	}
}


static uint8_t NotifyBuf[256] = {0};
static uint8_t NotifyLen = 0;

void Ble_Notify_Task(void *p_arg)
{
	CPU_TS ts;
	RTOS_ERR  err;
	struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t*  notify_rsp = NULL;
	uint32_t len_remain = 0;
	uint32_t p_out_tmp = 0;

	while(1)
	{
		OSSemPend(&ble_notify_ready_sem,
				5,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);

		OSMutexPend(&notify_array.mutex,
					0,
					OS_OPT_PEND_BLOCKING,
					&ts,
					&err);
		if(ble_conn_state == 0)
		{
			notify_array.pb_out = notify_array.pb_in;
		}
		else
		{
			while(notify_array.pb_out != notify_array.pb_in)
			{
				len_remain = ptr_minus(notify_array.pb_in, notify_array.pb_out, NOTIFY_ARRAY_SIZE);
				NotifyLen = (len_remain > ble_mtu) ? ble_mtu : len_remain;
				p_out_tmp = notify_array.pb_out;
				for(uint8_t i = 0; i < NotifyLen; i++)
				{
					NotifyBuf[i] = notify_array.data[p_out_tmp];
					p_out_tmp = ptr_plus(p_out_tmp, 1, NOTIFY_ARRAY_SIZE);
				}

				notify_rsp =  gecko_cmd_gatt_server_send_characteristic_notification(ble_conn_handle, gattdb_Send, NotifyLen, NotifyBuf);

				if(notify_rsp->result == bg_err_success)
				{
					notify_array.pb_out = p_out_tmp;
				}
				break;
			}
		}
		OSMutexPost(&notify_array.mutex,
					OS_OPT_POST_NONE,
					&err);
	}
}

static int32_t app_ble_get(uint8_t * p_byte)
{
	if(ble_recv_array.p_out == ble_recv_array.p_in)
	{
		return -1;
	}

	*p_byte = ble_recv_array.data[ble_recv_array.p_out];
	ble_recv_array.p_out++;
	if(ble_recv_array.p_out >= BLE_RECV_BUFF_SIZE)
	{
		ble_recv_array.p_out = 0;
	}

	return 0;
}

static uint32_t ble_get_data(void)
{
	uint8_t ch;
	static uint32_t proctol_start = false;
	static uint32_t recv_len = 0;
	static uint8_t* recv_data_p = (uint8_t*)(&ble_recv_buf);
	static uint32_t data_len = 0;

	while(!app_ble_get(&ch))
	{
		if((ch == WIFIAMP_APP_HEADER)&&(proctol_start == false))
		{
			proctol_start = true;
		}

		if(proctol_start == true)
		{
			recv_data_p[recv_len++] = ch;

			if(recv_len == 3)
			{
				data_len = (recv_data_p[1]<<8) + recv_data_p[2] + 1;
				if(data_len == 0)
				{
					recv_len = 0;
					data_len = 0;
					proctol_start = false;
					return true;
				}
				else if(data_len > UART_PROCOTOL_MAX_LEN)
				{
					recv_len = 0;
					data_len = 0;
					proctol_start = false;
					return false;
				}
			}

			else if(recv_len > 3)
			{
				data_len--;

				if(data_len == 0)
				{
					recv_len = 0;
					proctol_start = false;
					return true;
				}
			}
		}
		else
		{
			continue;
		}
	}
	return false;
}

void App_Ble_Parse_Task(void *p_arg)
{
	CPU_TS ts;
	RTOS_ERR  err;
	static uart_send_format_t*  Sendpdata;
	struct Buffer *p_buf;
	uint32_t len;

	while(1)
	{
		OSSemPend(&(ble_recv_array.sem),
				5,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);

		if(!(ble_get_data()))
		{
			continue;
		}

		p_buf = (void*)buf_factory_produce(g_uart_send_buf);
		if(p_buf == NULL)
		{
//				DBG("p_buf1 = NULL\r\n");
			continue;
		}

		Sendpdata = (uart_send_format_t*)(p_buf->memory);
		Sendpdata->buf = p_buf;
//			DBG("p_buf1 = %4x\r\n", p_buf);

		len = (ble_recv_buf.header.length_msb << 8) + ble_recv_buf.header.length_lsb + 4;
		memcpy(&(Sendpdata->cmd), &ble_recv_buf, len);

		Sendpdata->poll.type = 0;
		Sendpdata->list.owner = Sendpdata;
		Sendpdata->cmd_len = len;

		OSMutexPend(&uart_poll_mutex,
					0,
					OS_OPT_PEND_BLOCKING,
					&ts,
					&err);
		list_add_tail(&(Sendpdata->list), &g_uart_send_list);
		OSMutexPost(&uart_poll_mutex,
					OS_OPT_POST_NONE,
					&err);
	}
}

void Ble_Event_Task(void *p_arg)
{
	CPU_TS ts;
	RTOS_ERR  err;
//	void *p;
	uint16_t event;

	while(1)
	{
		OSQPend(&ble_event_msg,
					0,
					OS_OPT_PEND_BLOCKING,
					&event,
					&ts,
					&err);

		switch (event)
		{
			case BLE_CONNECTION_OPEND_EVT:
				ble_state_notify(1);
				break;

			case BLE_CONNECTION_CLOSED_EVT:
				ble_state_notify(0);
				break;
		}
	}

}

/***************************************************************************//**
 * Setup the bluetooth init function.
 *
 * @return error code for the gecko_init function
 *
 * All bluetooth specific initialization code should be here like gecko_init(),
 * gecko_init_whitelisting(), gecko_init_multiprotocol() and so on.
 ******************************************************************************/
static errorcode_t initialize_bluetooth()
{
  errorcode_t err = gecko_init(&bluetooth_config);
  APP_RTOS_ASSERT_DBG((err == bg_err_success), 1);
  return err;
}

void ble_context_init(void)
{
	RTOS_ERR  err;

	OSSemCreate(&ble_notify_ready_sem,
				"ble_notify_ready_sem",
				0,
				&err);

	OSMutexCreate(&(notify_array.mutex),
				"notify_array_mutex",
				&err);

	OSSemCreate(&(ble_recv_array.sem),
				"BleRecvArray_sem",
				0,
				&err);

	OSQCreate(&ble_event_msg,
			"ble_event_msg",
			8,
			&err);

	bluetooth_start(APP_CFG_TASK_BLUETOOTH_LL_PRIO,
				  APP_CFG_TASK_BLUETOOTH_STACK_PRIO,
				  initialize_bluetooth);

	// Create task for Event handler
	OSTaskCreate((OS_TCB     *)&ApplicationTaskTCB,
			   (CPU_CHAR   *)"Bluetooth Application Task",
			   (OS_TASK_PTR ) BluetoothApplicationTask,
			   (void       *) 0u,
			   (OS_PRIO     ) APP_CFG_TASK_APPLICATION_PRIO,
			   (CPU_STK    *)&ApplicationTaskStk[0u],
			   (CPU_STK_SIZE)(APPLICATION_STACK_SIZE / 10u),
			   (CPU_STK_SIZE) APPLICATION_STACK_SIZE,
			   (OS_MSG_QTY  ) 0u,
			   (OS_TICK     ) 0u,
			   (void       *) 0u,
			   (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
			   (RTOS_ERR   *)&err);
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	OSTaskCreate((OS_TCB     *)&Ble_Notify_Task_TCB,
			   (CPU_CHAR   *)"Ble Notify Task",
			   (OS_TASK_PTR ) Ble_Notify_Task,
			   (void       *) 0u,
			   (OS_PRIO     ) Ble_Notify_Task_PRIO,
			   (CPU_STK    *)&Ble_Notify_Task_Stk[0u],
			   (CPU_STK_SIZE)(Ble_Notify_Task_STK_SIZE / 10u),
			   (CPU_STK_SIZE) Ble_Notify_Task_STK_SIZE,
			   (OS_MSG_QTY  ) 0u,
			   (OS_TICK     ) 0u,
			   (void       *) 0u,
			   (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
			   (RTOS_ERR   *)&err);
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	OSTaskCreate(&App_Ble_Parse_Task_TCB,
				"App_Uart_Parse_Task",
				App_Ble_Parse_Task,
				0,
				App_Ble_Parse_Task_PRIO,
				&App_Ble_Parse_Task_Stk[0],
				(App_Ble_Parse_Task_STK_SIZE / 10u),
				App_Ble_Parse_Task_STK_SIZE,
				0,
				0,
				0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				&err);
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

	OSTaskCreate(&Ble_Event_Task_TCB,
				"Ble_Event_Task",
				Ble_Event_Task,
				0,
				Ble_Event_Task_PRIO,
				&Ble_Event_Task_Stk[0],
				(Ble_Event_Task_STK_SIZE / 10u),
				Ble_Event_Task_STK_SIZE,
				0,
				0,
				0,
				(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				&err);
	/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}


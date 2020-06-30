/*
 * uart_ptl.c
 *
 *  Created on: 2019年10月12日
 *      Author: Administrator
 */


#include "bg_types.h"
#include "rtos_gecko.h"
#include "ble.h"
#include "uartdrv.h"
//#include "mible_api.h"
//#include "efr32_api.h"
#include "uartdrv.h"
#include "uart.h"
#include "btl_interface.h"
#include "btl_interface_storage.h"
#include "buf_factory.h"
#include "uart_communicate.h"
#include "uart_ptl.h"
#include "rtt.h"


#define setresplen( len )														\
{			                                                                    \
	Respdata->header.length_msb = len>>8;                                       \
	Respdata->header.length_lsb = len;                                          \
}


uint8_t check_sum_cal(void *p, uint16_t len);

IAP_STR iap_str;
unsigned char seq_id = 0;
//extern uint32_t pb_in, pb_out;
//extern uint8_t send_to_app_array[BUFF_SIZE];

//uint8_t *updata_buf;
extern struct buffer_factory *g_uart_send_buf;
extern uint8_t ble_conn_state;
extern m_notify_array_t notify_array;
extern OS_MUTEX uart_poll_mutex;
extern OS_SEM ble_notify_ready_sem;
extern struct list_head g_uart_send_list;
extern uart_recv_buf_t UartRecvBuf;
uart_cmd_format_t uart_recv_buf;
uart_cmd_format_t *Data_p;
static uart_cmd_format_t*  Respdata = NULL;
static uart_send_format_t*  sendpdata = NULL;
static uint16_t  Respdatalen = 0;
static uint8_t uart_send_sn = 0;
extern OS_Q app_uart_send_msg;

static PARAM_STR param={0,0,{NULL}};

static unsigned short const wCRC16Table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};



void CRC16(uint8_t* pDataIn, uint32_t iLenIn, uint16_t* pCRCOut)
{
     unsigned short wResult = 0;
     unsigned short wTableNo = 0;
     int i = 0;
    for( i = 0; i < iLenIn; i++)
    {
        wTableNo = ((wResult & 0xff) ^ (pDataIn[i] & 0xff));
        wResult = ((wResult >> 8) & 0xff) ^ wCRC16Table[wTableNo];
    }

    *pCRCOut = wResult;
}


int32_t app_uart_get(uint8_t * p_byte)
{
//	CPU_TS ts;
//	RTOS_ERR  err;
//	OSMutexPend(&UartRecvData.mutex,
//				0,
//				OS_OPT_PEND_BLOCKING,
//				&ts,
//				&err);
	if(UartRecvBuf.p_out == UartRecvBuf.p_in)
	{
//		OSMutexPost(&UartRecvData.mutex,
//					OS_OPT_POST_NONE,
//					&err);
		return -1;
	}

	*p_byte = UartRecvBuf.data[UartRecvBuf.p_out];
	UartRecvBuf.p_out++;
	if(UartRecvBuf.p_out >= UART_RECV_BUFF_SIZE)
	{
		UartRecvBuf.p_out = 0;
	}

//	OSMutexPost(&UartRecvData.mutex,
//				OS_OPT_POST_NONE,
//				&err);
	return 0;
}

uint32_t uart_get_data(void)
{
	uint8_t ch;
	static uint32_t proctol_start = false;
	static uint32_t recv_len = 0;
	static uint8_t* recv_data_p = (uint8_t*)(&uart_recv_buf);
	static uint32_t data_len = 0;

	while(!app_uart_get(&ch))
	{
//		DBG(" %02X",ch);
//		DBG("file:%s line:%d\n",__FILE__, __LINE__);
		//			DBG(YELLOW);
		//			for(int i = 0; i < received; i++)
		//			{
		//				DBG(" %02X",buffer[i]);
		//			}
		//			DBG("\r\n"D_NONE);
		if((ch == WIFIAMP_DEV_HEADER)&&(proctol_start == false))
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



static int32_t uart_protocol_dispose(uart_send_info_t* info)
{
	CPU_TS ts;
	RTOS_ERR  err;
	uint32_t protocol_len;
	static uart_cmd_format_t*  disRespdata;
	static uart_send_format_t*  disSendpdata;
	struct Buffer *p_buf;
//	struct message msg;

	p_buf = (void*)buf_factory_produce(g_uart_send_buf);
	if(p_buf == NULL)
	{
//		DBG("file:%s line:%d\n",__FILE__, __LINE__);
		return -1;
	}

	disSendpdata = (uart_send_format_t*)(p_buf->memory);
	disSendpdata->buf = p_buf;
	disRespdata = &(disSendpdata->cmd);

	disRespdata->header.head = WIFIAMP_MDL_HEADER;
	protocol_len = info->len + 4;
//	setresplen(protocol_len);
	disRespdata->header.length_msb = protocol_len >> 8;
	disRespdata->header.length_lsb = protocol_len;
	disRespdata->header.sta_param = 0;
	disRespdata->header.sn = uart_send_sn;
	uart_send_sn++;
	disRespdata->header.pdt_type = info->pdt_type;
	disRespdata->header.cmd_id = info->cmd_id;

	if(info->len)
	{
		memcpy(disRespdata->data, info->data, info->len);
	}
	disRespdata->data[info->len] = check_sum_cal(&(disRespdata->header.sta_param), protocol_len);

	memset(&(disSendpdata->poll), 0, sizeof(uart_poll_t));
	disSendpdata->poll.type = info->poll_type;
	disSendpdata->list.owner = disSendpdata;
	disSendpdata->cmd_len = protocol_len + 4;

//	DBG(YELLOW);
//	for(int i = 0; i < disSendpdata->cmd_len; i++)
//	{
//		DBG(" %02X",*((char*)&(disSendpdata->cmd)+i));
//	}
//	DBG("\r\n"D_NONE);

	OSMutexPend(&uart_poll_mutex,
				0,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);
	list_add_tail(&(disSendpdata->list), &g_uart_send_list);

//	disSendpdata->poll.free_flag = 1;
//	OSQPost(&app_uart_send_msg,
//			disSendpdata,
//			disSendpdata->cmd_len,
//			OS_OPT_POST_FIFO,
//			&err);

	OSMutexPost(&uart_poll_mutex,
				OS_OPT_POST_NONE,
				&err);
//	DBG("file:%s line:%d\n",__FILE__, __LINE__);
	return 0;
}


//主动上传命令
void ble_state_notify(uint8 state)
{
	uart_send_info_t  info;
	info.len = 0;
	info.data[info.len] = state;
	info.len += 1;
	info.cmd_id = NTF_CONN_STATE;
	info.pdt_type = PDTTYPE_BLE_MODULE;
	info.poll_type = 0;
	uart_protocol_dispose(&info);
}



void iap_req_request(uint32_t offset)
{
	uart_send_info_t  info;
	info.len = 0;
	info.data[info.len] = offset>>24;
	info.len++;
	info.data[info.len] = offset>>16;
	info.len++;
	info.data[info.len] = offset>>8;
	info.len++;
	info.data[info.len] = offset;
	info.len++;
	info.cmd_id = REQ_UPG_DATA;
	info.pdt_type = PDTTYPE_BLE_FLASH;
	info.poll_type = 1;
	uart_protocol_dispose(&info);
//	DBG("file:%s line:%d\n",__FILE__, __LINE__);

}


void iap_done(uint8_t done)
{
	uart_send_info_t  info;
	info.len = 0;
	info.data[info.len] = done;
	info.len++;
	info.cmd_id = NTF_UPG_DONE;
	info.pdt_type = PDTTYPE_BLE_FLASH;
	info.poll_type = 0;
	uart_protocol_dispose(&info);
}


//接收解析命令
void module_hand_shake_rsp(uart_cmd_format_t* Data_p)
{
	CPU_TS ts;
	RTOS_ERR  err;
	uint8_t fw_ver[3] = FW_VER;
	uint8_t hw_ver[3] = HW_VER;
	uint8_t mac[6];
	struct Buffer *p_buf;
	DBG(YELLOW"command: MODULE_HAND_SHAKE\r\n"D_NONE);

	p_buf = (void*)buf_factory_produce(g_uart_send_buf);
	if(p_buf == NULL)
	{
		return;
	}
	sendpdata = (uart_send_format_t*)(p_buf->memory);
	sendpdata->buf = p_buf;
	Respdata = &(sendpdata->cmd);

	Respdata->header.head = WIFIAMP_MDL_HEADER;
	Respdata->header.sta_param = 0;
	Respdata->header.sn = Data_p->header.sn;
	Respdata->header.pdt_type = Data_p->header.pdt_type;
	Respdata->header.cmd_id = ASK_MODULE_HAND_SHAKE;

	// 填充协议返回数据 ------------------------------------------------------------------------------------
	memcpy(&(Respdata->data[Respdatalen]), hw_ver, 3);
	Respdatalen += 3;
	memcpy(&(Respdata->data[Respdatalen]), fw_ver, 3);
	Respdatalen += 3;
	mible_gap_address_get(mac);
	Respdata->data[Respdatalen++] = mac[5];
	Respdata->data[Respdatalen++] = mac[4];
	Respdata->data[Respdatalen++] = mac[3];
	Respdata->data[Respdatalen++] = mac[2];
	Respdata->data[Respdatalen++] = mac[1];
	Respdata->data[Respdatalen++] = mac[0];

	Respdata->data[Respdatalen] = ble_conn_state;
	Respdatalen += 1;
	//------------------------------------------------------------------------------------------------------
	Respdatalen += 4;
	setresplen(Respdatalen);
	Respdata->data[Respdatalen - 4] = check_sum_cal(&(Respdata->header.sta_param), Respdatalen);

	memset(&(sendpdata->poll), 0, sizeof(uart_poll_t));
	sendpdata->poll.type = 0;
	sendpdata->list.owner = sendpdata;
	sendpdata->cmd_len = (Respdata->header.length_msb<<8) + Respdata->header.length_lsb + 4;

	OSMutexPend(&uart_poll_mutex,
				0,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);
	list_add_tail(&(sendpdata->list), &g_uart_send_list);
	OSMutexPost(&uart_poll_mutex,
				OS_OPT_POST_NONE,
				&err);
}
uint8_t const module_hand_shake_rsp_argvlist[]={1,0};



void adv_ctrl_rsp(uart_cmd_format_t* Data_p)
{
	CPU_TS ts;
	RTOS_ERR  err;
	struct Buffer *p_buf;
	DBG(YELLOW"command: CMD_ADV_CTRL\r\n"D_NONE);
	// 执行动作-------------------------------------------------------------------------
	if(param.argv[0][0] == 1)
	{
		advertising_start();
	}
	//----------------------------------------------------------------------------------
	p_buf = (void*)buf_factory_produce(g_uart_send_buf);
	if(p_buf == NULL)
	{
		return;
	}
	sendpdata = (uart_send_format_t*)(p_buf->memory);
	sendpdata->buf = p_buf;
	Respdata = &(sendpdata->cmd);

	Respdata->header.head = WIFIAMP_MDL_HEADER;
	Respdata->header.sta_param = 0;
	Respdata->header.sn = Data_p->header.sn;
	Respdata->header.pdt_type = Data_p->header.pdt_type;
	Respdata->header.cmd_id = ASK_ADV_CTRL;

	// 填充协议返回数据 -------------------------------------------------------------------------------------
	Respdata->data[Respdatalen] = param.argv[0][0];
	Respdatalen += 1;
	//------------------------------------------------------------------------------------------------------
	Respdatalen += 4;
	setresplen(Respdatalen);
	Respdata->data[Respdatalen - 4] = check_sum_cal(&(Respdata->header.sta_param), Respdatalen);

	memset(&(sendpdata->poll), 0, sizeof(uart_poll_t));
	sendpdata->poll.type = 0;
	sendpdata->list.owner = sendpdata;
	sendpdata->cmd_len = (Respdata->header.length_msb<<8) + Respdata->header.length_lsb + 4;

//	DBG(YELLOW);
//	for(int i = 0; i < sendpdata->cmd_len; i++)
//	{
//		DBG(" %02X",*((char*)&(sendpdata->cmd)+i));
//	}
//	DBG("\r\n"D_NONE);

	OSMutexPend(&uart_poll_mutex,
					0,
					OS_OPT_PEND_BLOCKING,
					&ts,
					&err);
	list_add_tail(&(sendpdata->list), &g_uart_send_list);
	OSMutexPost(&uart_poll_mutex,
				OS_OPT_POST_NONE,
				&err);
}
uint8_t const adv_ctrl_rsp_argvlist[]={1,0};


void set_adv_data_rsp(uart_cmd_format_t* Data_p)
{
	CPU_TS ts;
	RTOS_ERR  err;
	struct Buffer *p_buf;

	DBG(YELLOW"command: CMD_SET_ADV_DATA\r\n"D_NONE);

	// 执行动作-----------------------------------------------------------------
	if(advertising_init(param.argv[0][0], param.argv[2], param.argv[1]) != 0)
	{
		return;
	}
	//-------------------------------------------------------------------------

	p_buf = (void*)buf_factory_produce(g_uart_send_buf);
	if(p_buf == NULL)
	{
		return;
	}
	sendpdata = (uart_send_format_t*)(p_buf->memory);
	sendpdata->buf = p_buf;
	Respdata = &(sendpdata->cmd);

	Respdata->header.head = WIFIAMP_MDL_HEADER;
	Respdata->header.sta_param = 0;
	Respdata->header.sn = Data_p->header.sn;
	Respdata->header.pdt_type = Data_p->header.pdt_type;
	Respdata->header.cmd_id = ASK_SET_ADV_DATA;

	// 填充协议返回数据 -------------------------------------------------------------------------------------
	Respdata->data[Respdatalen] = param.argv[0][0];
	Respdatalen += 1;
	//------------------------------------------------------------------------------------------------------

	Respdatalen += 4;
	setresplen(Respdatalen);
	Respdata->data[Respdatalen - 4] = check_sum_cal(&(Respdata->header.sta_param), Respdatalen);

	memset(&(sendpdata->poll), 0, sizeof(uart_poll_t));
	sendpdata->poll.type = 0;
	sendpdata->list.owner = sendpdata;
	sendpdata->cmd_len = (Respdata->header.length_msb<<8) + Respdata->header.length_lsb + 4;

	OSMutexPend(&uart_poll_mutex,
				0,
				OS_OPT_PEND_BLOCKING,
				&ts,
				&err);
	list_add_tail(&(sendpdata->list), &g_uart_send_list);
	OSMutexPost(&uart_poll_mutex,
					OS_OPT_POST_NONE,
					&err);
}
uint8_t const set_adv_data_rsp_argvlist[]={1,6,1,0};



void upg_request_rsp(uart_cmd_format_t* Data_p)
{
	CPU_TS ts;
	RTOS_ERR  err;
	struct Buffer *p_buf;

	// 执行动作-----------------------------------------------------------------

	iap_str.fw_len = (param.argv[1][0]<<24)|
					 (param.argv[1][1]<<16)|
					 (param.argv[1][2]<<8)|
					 param.argv[1][3];

	DBG("iap firmware len:%d\n",iap_str.fw_len);
	if(iap_str.fw_len > 241664)
	{
		iap_str.fw_len = 0;
		DBG("firmware len error!\n");
		return;
	}

	if(bootloader_init() == BOOTLOADER_OK)
	{
		OSTimeDlyHMSM(0, 0, 0, 10,
					  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
					  &err);
		DBG("bootloader init succeed!\n");
	}
	else
	{
		DBG("bootloader init failed!\n");
		return;
	}

	if(bootloader_eraseStorageSlot(0) == BOOTLOADER_OK)
	{
		OSTimeDlyHMSM(0, 0, 0, 100,
					  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
					  &err);
		DBG("bootloader erase StorageSlot succeed!\n");
	}
	else
	{
		DBG("bootloader erase StorageSlot failed!\n");
		return;
	}

	iap_str.in_progress = 1;
	iap_str.finished = 0;
	iap_str.offset = 0;
	iap_str.page_len = 128;
	iap_str.finished = 0;
	iap_str.pack_num = 0;
	//-------------------------------------------------------------------------

	p_buf = (void*)buf_factory_produce(g_uart_send_buf);
	if(p_buf == NULL)
	{
//		DBG("file:%s line:%d\n",__FILE__, __LINE__);
		return;
	}
	sendpdata = (uart_send_format_t*)(p_buf->memory);
	sendpdata->buf = p_buf;
	Respdata = &(sendpdata->cmd);

	Respdata->header.head = WIFIAMP_MDL_HEADER;
	Respdata->header.sta_param = 0;
	Respdata->header.sn = Data_p->header.sn;
	Respdata->header.pdt_type = Data_p->header.pdt_type;
	Respdata->header.cmd_id = ASK_UPG_REQUEST;

	// 填充协议返回数据 -------------------------------------------------------------------------------------
	Respdata->data[Respdatalen] = iap_str.page_len>>24;
	Respdatalen += 1;
	Respdata->data[Respdatalen] = iap_str.page_len>>16;
	Respdatalen += 1;
	Respdata->data[Respdatalen] = iap_str.page_len>>8;
	Respdatalen += 1;
	Respdata->data[Respdatalen] = iap_str.page_len;
	Respdatalen += 1;

	//------------------------------------------------------------------------------------------------------

	Respdatalen += 4;
	setresplen(Respdatalen);
	Respdata->data[Respdatalen - 4] = check_sum_cal(&(Respdata->header.sta_param), Respdatalen);

	memset(&(sendpdata->poll), 0, sizeof(uart_poll_t));
	sendpdata->poll.type = 0;
	sendpdata->list.owner = sendpdata;
	sendpdata->cmd_len = (Respdata->header.length_msb<<8) + Respdata->header.length_lsb + 4;

	OSMutexPend(&uart_poll_mutex,
					0,
					OS_OPT_PEND_BLOCKING,
					&ts,
					&err);
	list_add_tail(&(sendpdata->list), &g_uart_send_list);
	OSMutexPost(&uart_poll_mutex,
						OS_OPT_POST_NONE,
						&err);
//	DBG("file:%s line:%d\n",__FILE__, __LINE__);
    //接着发送一条请求升级数据
//	OSTimeDlyHMSM(0, 0, 1, 0,
//                  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
//                  &err);

	iap_req_request(0);

}
uint8_t const upg_request_rsp_argvlist[]={3,1,0};



void upg_data_rsp(uart_cmd_format_t* Data_p)
{
	RTOS_ERR  err;
	uint16_t result;
	uint16_t data_length;
	uint16_t crc_val;
	uint32_t cur_offset;

	// 执行动作-----------------------------------------------------------------
	data_length = (Data_p->header.length_msb<<8) + Data_p->header.length_lsb;
	data_length -= 6;
	crc_val = (Data_p->data[data_length]<<8) + Data_p->data[data_length+1];
	data_length -= 4;
	CRC16(Data_p->data + 4, data_length, &result);

	cur_offset = (param.argv[0][0]<<24)|
			     (param.argv[0][1]<<16)|
			     (param.argv[0][2]<<8)|
			      param.argv[0][3];


	if((crc_val == result)&&(cur_offset == iap_str.offset)&&(data_length <= iap_str.page_len))
	{
		if(bootloader_writeStorage(0,
									iap_str.offset,
									Data_p->data + 4,
									data_length) == BOOTLOADER_OK)
		{
//			iap_str.offset += iap_str.page_len;
			iap_str.offset += data_length;
			DBG("iap_pack_num = %d\r\n", iap_str.pack_num);
			iap_str.pack_num++;
		}

		else
		{
			DBG("bootloader_write error\r\n");
//			return; // 注意正式版本不能直接返回
		}
	}
	else
	{
		DBG("upgrade data check error! \r\n");
		DBG("crc_val = %x, result = %x \r\n", crc_val, result);
		DBG("cur_offset = %d, req_offset = %d \r\n", cur_offset, iap_str.offset);
		DBG("iap data length = %d\r\n", data_length);
//		return; // 注意正式版本不能直接返回
	}

	if(iap_str.offset < iap_str.fw_len)
	{
		DBG("iap_str.offset = %d\r\n", iap_str.offset);
		iap_req_request(iap_str.offset);
	}
	else
	{
		DBG("iap_str.offset = %d, offset = fw_len\r\n", iap_str.offset);
		if(bootloader_verifyImage(0, NULL) == BOOTLOADER_OK)
		{
			DBG("bootloader verifyImage success\r\n");
			bootloader_setImageToBootload(0);

			iap_str.finished = 0xAA;
			mble_record_write(1, &iap_str.finished, 1);

			OSTimeDlyHMSM(0, 0, 1, 0,
						  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
						  &err);

			bootloader_rebootAndInstall();
		}
		else
		{
			DBG("bootloader verifyImage error\r\n");
			iap_done(1);
		}
	}
}
uint8_t const upg_data_rsp_argvlist[]={4,1,0};



static uart_cmd_list_t const uart_cmd_list[]=
{
	{CMD_MODULE_HAND_SHAKE,       ASK_MODULE_HAND_SHAKE,              		module_hand_shake_rsp,       		module_hand_shake_rsp_argvlist},
	{CMD_ADV_CTRL,                ASK_ADV_CTRL,                             adv_ctrl_rsp,       		        adv_ctrl_rsp_argvlist},
	{CMD_SET_ADV_DATA,            ASK_SET_ADV_DATA,                         set_adv_data_rsp,       		    set_adv_data_rsp_argvlist},
	{CMD_UPG_REQUEST,             ASK_UPG_REQUEST,                          upg_request_rsp,                    upg_request_rsp_argvlist},
	{RSP_UPG_DATA,                0,                                        upg_data_rsp,                       upg_data_rsp_argvlist},

	{0,0,NULL}
};




uint8_t check_sum_cal(void *p, uint16_t len)
{
	uint8_t check_sum = 0;
	uint32_t tmp = 0;
	uint16_t i;
	uint8_t* data = p;
	for(i=0; i<len; i++)
	{
		tmp += data[i];
	}
	check_sum = (uint8_t)tmp;
	return check_sum;
}

uint32_t procotol_check(uart_cmd_format_t* procotol_data)
{
	uint8_t check_value;
	uint16_t data_len;

	data_len = (procotol_data->header.length_msb<<8) + procotol_data->header.length_lsb;

	if(data_len < 4)return false;

	check_value = check_sum_cal(&(procotol_data->header.sta_param), data_len);

	if(procotol_data->data[data_len-4] != check_value)
	{
		DBG("procotol check error! cur_value = %d, check_value = %x\n", procotol_data->data[data_len-4], check_value);
		return false;
	}

	return true;
}

void ArgsParam(uint8_t *param_line, uint8_t const *argvlist, PARAM_STR* pParam)
{
	pParam->argc=0;

	if (argvlist==NULL) return;

	if(*argvlist)
		pParam->argv[pParam->argc++] = param_line;
	else
		return;


	while(*(argvlist+1))
	{
		param_line += (*argvlist);
		pParam->argv[pParam->argc] = param_line;
		argvlist++;
		if(++pParam->argc >= ARGV_MAX_NUM)
			break;
	}
}



void uart_protocol_parse(void)
{
	CPU_TS ts;
	RTOS_ERR  err;
	const uart_cmd_list_t *list;
	struct list_head *p;
	uart_send_format_t *parray;
	uint32_t i,cmd_len;

	Data_p = (uart_cmd_format_t*)&uart_recv_buf;
	//计算校验
	if(procotol_check(Data_p) == false)
	{
		return;
	}

	if((Data_p->header.pdt_type == PDTTYPE_BLE_MODULE)||(Data_p->header.pdt_type == PDTTYPE_BLE_FLASH))
	{
		for(list=uart_cmd_list; list->callback!=NULL; list++)
		{
			if(Data_p->header.cmd_id == list->cmdword)
			{
				Respdatalen = 0;
				ArgsParam(Data_p->data,list->argvlist,&param);

				list->callback(Data_p);

				if(list->ackword)
				{
					;
				}

				else
				{
					OSMutexPend(&uart_poll_mutex,
									0,
									OS_OPT_PEND_BLOCKING,
									&ts,
									&err);
					p = g_uart_send_list.next;
					while(p != &g_uart_send_list)
					{
						parray = (uart_send_format_t *)(p->owner);

						if((Data_p->header.sn == parray->cmd.header.sn)&&
							(Data_p->header.cmd_id == parray->cmd.header.cmd_id))
						{
							parray->poll.free_flag = 1;
							break;
						}
						p = p->next;
					}
					OSMutexPost(&uart_poll_mutex,
								OS_OPT_POST_NONE,
								&err);
				}

				return;
			}
		}
	}

	if(ble_conn_state == 1)
	{
		cmd_len = (Data_p->header.length_msb<<8) + Data_p->header.length_lsb + 4;
		OSMutexPend(&notify_array.mutex,
					0,
					OS_OPT_PEND_BLOCKING,
					&ts,
					&err);

//		DBG(YELLOW);
//		for(int i = 0; i < cmd_len; i++)
//		{
//			DBG(" %02X",*((char*)Data_p+i));
//		}
//		DBG("\r\n"D_NONE);

		for(i = 0; i < cmd_len; i++)
		{
			notify_array.data[notify_array.pb_in] = *((char*)Data_p+i);//Data_p->data[i];
			notify_array.pb_in = ptr_plus(notify_array.pb_in, 1, NOTIFY_ARRAY_SIZE);
		}
		OSMutexPost(&notify_array.mutex,
					OS_OPT_POST_NONE,
					&err);

		OSSemPost(&ble_notify_ready_sem,
				OS_OPT_POST_1,
				&err);
	}
}





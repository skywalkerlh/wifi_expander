/*
 * uart_ptl.h
 *
 *  Created on: 2019��10��12��
 *      Author: Administrator
 */

#ifndef UART_PTL_H_
#define UART_PTL_H_

#include "uart_communicate.h"

//frame head
#define WIFIAMP_DEV_HEADER	                0xD0
#define WIFIAMP_APP_HEADER	                0xC0
#define WIFIAMP_MDL_HEADER	                0xC0
// cmd
// ����ģ��
#define PDTTYPE_BLE_MODULE             0xFF
#define PDTTYPE_BLE_FLASH              0xF0

// NTF_�����ϴ���Ҫ��Ӧ
// REQ_�����ϴ���Ҫ��Ӧ
// CMD_���յ�����
// ASK_����CMD��Ӧ��
// RSP_���յ���Ӧ������ҪӦ��


#define NTF_CONN_STATE                      0x10

#define CMD_MODULE_HAND_SHAKE               0x13
#define ASK_MODULE_HAND_SHAKE               0x13

#define CMD_ADV_CTRL                        0x11
#define ASK_ADV_CTRL                        0x11

#define CMD_SET_ADV_DATA                    0x12
#define ASK_SET_ADV_DATA                    0x12

#define CMD_UPG_REQUEST                     0xD1
#define ASK_UPG_REQUEST                     0xD1

#define REQ_UPG_DATA                        0xD2
#define RSP_UPG_DATA                        0xD2

#define NTF_UPG_DONE                        0xD3



#define MAX_CMD_SIZE     400


#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)


#define FW_VER               {0x1,0x1,0x3}
#define HW_VER               {0x1,0x0,0x0}


//BLE��������
typedef struct stru_ble_data
{
	unsigned char state;		//״̬ID���̶�Ϊ0x00
	unsigned char seq_id;		//˳��ID
	unsigned char product_type;	//��Ʒ����
	unsigned char cmd_id;		//����ID
	unsigned char* buff;		//���ݻ�����
	unsigned int len;		    //���ݳ���
}STRU_BLE_DATA;


typedef struct
{
	uint8_t 	 cmd_id;       //�����֣����嶨�����չЭ��2.3
	uint32_t  len;          //�������ݳ��ȣ����û��������ֵ0
	uint8_t   pdt_type;     //��Ʒ���ͣ�Ѫѹ���Դ˽���Э��ֲ����
	uint8_t   data[128];    //��������
	uint8_t   poll_type;    // 1����Ҫ�ȴ���Ӧ  0�����õȴ���Ӧ
}uart_send_info_t;

typedef struct
{
	uint8_t  cmdword;
	uint8_t  ackword;
	void    (*callback)(uart_cmd_format_t* Data_p);
	uint8_t   const *argvlist;
}uart_cmd_list_t;

#define ARGV_MAX_NUM	24
typedef struct
{
	uint32_t 	len;
	uint8_t    argc;
	uint8_t    *argv[ARGV_MAX_NUM];
} PARAM_STR;


typedef struct IAP
{
	uint32_t in_progress;
	uint32_t offset;
	uint32_t fw_len;
	uint32_t page_len;
	uint8_t finished;
	uint8_t  version[3];
	uint32_t pack_num;
}IAP_STR;

void iap_done(uint8_t done);
void ble_state_notify(uint8_t state);
void uart_protocol_parse(void);
uint32_t uart_get_data(void);

#endif /* UART_PTL_H_ */



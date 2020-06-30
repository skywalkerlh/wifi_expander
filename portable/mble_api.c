/*
 * mble_api.c
 *
 *  Created on: 2019Äê11ÔÂ21ÈÕ
 *      Author: Administrator
 */

#include "bg_errorcodes.h"
#include "bg_types.h"
#include "rtos_gecko.h"
#include "gatt_db.h"

#include "mbedtls/aes.h"

#include "efr32_api.h"
#include "em_cmu.h"
#include "em_msc.h"
#include "em_gpio.h"
#include "efr32_api.h"
#include "stdbool.h"

#include "bg_gattdb_def.h"



#define ADV_HANDLE                      0
extern uint8 ble_conn_handle;


/*
 * @brief 	Restore data to flash
 * @param 	[in] record_id: identify an area in flash
 * 			[out] p_data: pointer to data
 *			[in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAM   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
mble_status_t mble_record_read(uint16_t record_id, uint8_t* p_data, uint8_t len)
{
    // TODO: support length longer than 56.
    if (len > MAX_SINGLE_PS_LENGTH){
        return MI_ERR_INVALID_LENGTH;
    } else if (record_id > 127) {
        return MI_ERR_INVALID_PARAM;
    } else if (p_data == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    struct gecko_msg_flash_ps_load_rsp_t *p_rsp;
    p_rsp = gecko_cmd_flash_ps_load(record_id + 0x4000);

    if (p_rsp->result == bg_err_success) {
        memcpy(p_data, p_rsp->value.data, len);
        return MI_SUCCESS;
    } else {
        return MI_ERR_INVALID_PARAM;
    }
}

/*
 * @brief 	Store data to flash
 * @param 	[in] record_id: identify an area in flash
 * 			[in] p_data: pointer to data
 * 			[in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 * allowed size.
 *          MI_ERR_INVALID_PARAM   p_data is not aligned to a 4 byte boundary.
 * @note  	Should use asynchronous mode to implement this function.
 *          The data to be written to flash has to be kept in memory until the
 * */
mble_status_t mble_record_write(uint16_t record_id, const uint8_t* p_data, uint8_t len)
{
    // TODO: support length longer than 56.
    if (len > MAX_SINGLE_PS_LENGTH){
        return MI_ERR_INVALID_LENGTH;
    } else if (p_data == NULL || record_id > 127) {
        return MI_ERR_INVALID_PARAM;
    }

    struct gecko_msg_flash_ps_save_rsp_t *p_rsp;
    p_rsp = gecko_cmd_flash_ps_save(record_id + 0x4000, len, p_data);


//    mble_arch_evt_param_t arch_evt_param;
//    arch_evt_param.record.id = record_id;
//    arch_evt_param.record.status = p_rsp->result == bg_err_success ? MI_SUCCESS : MI_ERR_RESOURCES;
//    mible_arch_event_callback(MBLE_ARCH_EVT_RECORD_WRITE, &arch_evt_param);
    return p_rsp->result == bg_err_success ? MI_SUCCESS : MI_ERR_RESOURCES;
}

/*
 * @brief	Update the connection parameters.
 * @param  	[in] conn_handle: the connection handle.
 *			[in] conn_params: the connection parameters.
 * @return  MI_SUCCESS             The Connection Update procedure has been
 *started successfully.
 *          MI_ERR_INVALID_STATE   Initiated this procedure in disconnected
 *state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 *retry.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note  	This function can be used by both central role and peripheral
 *role.
 * */
mble_status_t mble_gap_update_conn_params(uint8_t conn_handle,
        mble_gap_conn_param_t conn_params)
{
    struct gecko_msg_le_connection_set_timing_parameters_rsp_t *ret;
    if (conn_handle == DISCONNECTION) {
        return MI_ERR_INVALID_STATE;
    }

//    ret = gecko_cmd_le_connection_set_parameters(conn_handle,
//            conn_params.min_conn_interval, conn_params.max_conn_interval,
//            conn_params.slave_latency, conn_params.conn_sup_timeout);

    ret = gecko_cmd_le_connection_set_timing_parameters(conn_handle,
    		            								conn_params.min_conn_interval,
														conn_params.max_conn_interval,
														conn_params.slave_latency,
														conn_params.conn_sup_timeout,
														conn_params.min_ce_length,
														conn_params.max_ce_length);
    if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else if (ret->result == bg_err_invalid_conn_handle) {
        return MBLE_ERR_INVALID_CONN_HANDLE;
    } else if (ret->result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    } else if (ret->result == bg_err_wrong_state) {
        return MI_ERR_INVALID_STATE;
    }
    return MI_SUCCESS;
}

/**
 * @brief   Config advertising data
 * @param   [in] p_data : Raw data to be placed in advertising packet. If NULL, no changes are made to the current advertising packet.
 * @param   [in] dlen   : Data length for p_data. Max size: 31 octets. Should be 0 if p_data is NULL, can be 0 if p_data is not NULL.
 * @param   [in] p_sr_data : Raw data to be placed in scan response packet. If NULL, no changes are made to the current scan response packet data.
 * @param   [in] srdlen : Data length for p_sr_data. Max size: BLE_GAP_ADV_MAX_SIZE octets. Should be 0 if p_sr_data is NULL, can be 0 if p_data is not NULL.
 * @return  MI_SUCCESS             Successfully set advertising data.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * */
mble_status_t mble_gap_adv_data_set(uint8_t const * p_data, uint8_t dlen,
        uint8_t const *p_sr_data, uint8_t srdlen)
{
    struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t *ret;

    if (p_data != NULL && dlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set advertisement data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0, dlen, p_data);
        if (ret->result == bg_err_invalid_param) {
            return MI_ERR_INVALID_PARAM;
        }
//        memcpy(last_adv_data.data, p_data, dlen);
//        last_adv_data.len = dlen;
    }

    if (p_sr_data != NULL && srdlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set scan response data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1, srdlen, p_sr_data);
        if (ret->result == bg_err_invalid_param) {
            return MI_ERR_INVALID_PARAM;
        }
//        memcpy(last_scan_rsp.data, p_sr_data, srdlen);
//        last_scan_rsp.len = srdlen;
    }

    return MI_SUCCESS;
}

/*
 * @brief	Start advertising
 * @param 	[in] p_adv_param : pointer to advertising parameters, see
 * mible_gap_adv_param_t for details
 * @return  MI_SUCCESS             Successfully initiated advertising procedure.
 *          MI_ERR_INVALID_STATE   Initiated connectable advertising procedure
 * when connected.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 * retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again.
 * @note	Other default advertising parameters: local public address , no
 * filter policy
 * */
mble_status_t mible_gap_adv_start(mble_gap_adv_param_t *p_param)
{
    uint16_t result;
    uint8 channel_map = 0, connect = 0;

    if (p_param->ch_mask.ch_37_off != 1) {
        channel_map |= 0x01;
    }
    if (p_param->ch_mask.ch_38_off != 1) {
        channel_map |= 0x02;
    }
    if (p_param->ch_mask.ch_39_off != 1) {
        channel_map |= 0x04;
    }

    if ((ble_conn_handle != DISCONNECTION)
            && (p_param->adv_type == MBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED)) {
        return MI_ERR_INVALID_STATE;
    }

    result = gecko_cmd_le_gap_set_advertise_timing(ADV_HANDLE, p_param->adv_interval_min,
            p_param->adv_interval_max, 0, 0)->result;
//    MI_ERR_CHECK(result);
    if (result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    }

    result = gecko_cmd_le_gap_set_advertise_channel_map(ADV_HANDLE, channel_map)->result;
//    MI_ERR_CHECK(result);
    if (result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    }

    if (p_param->adv_type == MBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_connectable_scannable;
    } else if (p_param->adv_type == MBLE_ADV_TYPE_SCANNABLE_UNDIRECTED) {
        connect = le_gap_scannable_non_connectable;
    } else if (p_param->adv_type == MBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_non_connectable;
    } else {
        return MI_ERR_INVALID_PARAM;
    }

//    if (last_adv_data.len != 0) {
//        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0,
//                last_adv_data.len, last_adv_data.data)->result;
//        if (result != bg_err_success)
//            return MI_ERR_BUSY;
//    }
//    if (last_scan_rsp.len != 0) {
//        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1,
//                last_scan_rsp.len, last_scan_rsp.data)->result;
//        if (result != bg_err_success)
//            return MI_ERR_BUSY;
//    }

    result = gecko_cmd_le_gap_start_advertising(ADV_HANDLE, le_gap_user_data, connect)->result;
//    MI_ERR_CHECK(result);
    if (result == bg_err_success) {
//        advertising = 1;
        return MI_SUCCESS;
    } else {
//        connect_param_for_retry = connect;
        gecko_external_signal(START_ADV_RETRY_BIT_MASK);
        return MI_ERR_BUSY;
    }
}

/*
 * @brief 	Get BLE mac address.
 * @param 	[out] mac: pointer to data
 * @return  MI_SUCCESS			The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note: 	You should copy gap mac to mac[6]
 * */

mble_status_t mible_gap_address_get(mble_addr_t mac)
{
    struct gecko_msg_system_get_bt_address_rsp_t *ret = gecko_cmd_system_get_bt_address();
    memcpy(mac, ret->address.addr, 6);
    return MI_SUCCESS;
}













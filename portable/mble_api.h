/*
 * mble_api.h
 *
 *  Created on: 2019Äê11ÔÂ21ÈÕ
 *      Author: Administrator
 */

#ifndef PORTABLE_MBLE_API_H_
#define PORTABLE_MBLE_API_H_

#include "mble_type.h"
#include "efr32_api.h"


mble_status_t mble_record_read(uint16_t record_id, uint8_t* p_data, uint8_t len);
mble_status_t mble_record_write(uint16_t record_id, const uint8_t* p_data, uint8_t len);
mble_status_t mble_gap_update_conn_params(uint16_t conn_handle, mble_gap_conn_param_t conn_params);
mble_status_t mble_gap_adv_data_set(uint8_t const * p_data, uint8_t dlen,
        uint8_t const *p_sr_data, uint8_t srdlen);
mble_status_t mible_gap_adv_start(mble_gap_adv_param_t *p_param);


#endif /* PORTABLE_MBLE_API_H_ */

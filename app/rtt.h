/*
 * rtt.h
 *
 *  Created on: 2019Äê10ÔÂ12ÈÕ
 *      Author: Administrator
 */

#ifndef RTT_H_
#define RTT_H_


#include "SEGGER_RTT.h"


#define _DEBUG

#define D_NONE               "\x1B[0m"
#define BLACK                "\x1B[0;30m"
#define L_BLACK              "\x1B[1;30m"
#define RED                  "\x1B[0;31m"
#define L_RED                "\x1B[1;31m"
#define GREEN                "\x1B[0;32m"
#define L_GREEN              "\x1B[1;32m"
#define BROWN                "\x1B[0;33m"
#define YELLOW               "\x1B[1;33m"
#define BLUE                 "\x1B[0;34m"
#define L_BLUE               "\x1B[1;34m"
#define PURPLE               "\x1B[0;35m"
#define L_PURPLE             "\x1B[1;35m"
#define CYAN                 "\x1B[0;36m"
#define L_CYAN               "\x1B[1;36m"
#define GRAY                 "\x1B[0;37m"
#define WHITE                "\x1B[1;37m"

#define BOLD                 "\x1B[1m"
#define UNDERLINE            "\x1B[4m"
#define BLINK                "\x1B[5m"
#define REVERSE              "\x1B[7m"
#define HIDE                 "\x1B[8m"
#define CLEAR                "\x1B[2J"
#define CLRLINE              "\r\x1B[K" //or "\e[1K\r"


#ifdef _DEBUG
#define DBG(format, args...)   SEGGER_RTT_printf(0, format, ##args)
#else
#define DBG( format, args... )
#endif


void rtt_context_init(void);


#endif /* RTT_H_ */

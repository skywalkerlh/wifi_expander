/***************************************************************************//**
 * @file
 * @brief Implements the Thermometer (GATT Server) Role of the Health
 * Thermometer Profile, which enables a Collector device to connect and
 * interact with a Thermometer.  The device acts as a connection
 * Peripheral. The sample application is based on Micrium OS RTOS.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                            INCLUDE FILES
 *********************************************************************************************************
 *********************************************************************************************************
 */

#include "bsp/siliconlabs/generic/include/bsp_os.h"

#include <cpu/include/cpu.h>
#include <common/include/common.h>
#include <kernel/include/os.h>

#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>
#include <common/include/rtos_prio.h>
#include  <common/include/platform_mgr.h>

#include "sleep.h"
#include <stdio.h>

#include "rtos_bluetooth.h"
//Bluetooth API definition
#include "rtos_gecko.h"
//GATT DB
#include "gatt_db.h"

/* Board Headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"
#include "hal-config-app-common.h"

#include "em_cmu.h"
#include "bsp.h"
#include "rtt.h"
#include "uart_ptl.h"
#include "mble_api.h"

#include "ble.h"
#include "uart.h"
#include "btl_interface.h"
#include "btl_interface_storage.h"

//#include "mbedtls/aes.h"

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             LOCAL DEFINES
 *********************************************************************************************************
 *********************************************************************************************************
 */

#define OTA

#define  EX_MAIN_START_TASK_PRIO            21u
#define  EX_MAIN_START_TASK_STK_SIZE        512u

#define  APP_CFG_TASK_START_PRIO            2u

#define  App_Main_Task_PRIO                 6u
#define  App_Main_Task_STK_SIZE             (1024 / sizeof(CPU_STK))

// MTM: Added configs for all Micrium OS tasks

// Tick Task Configuration
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
#define  TICK_TASK_PRIO             0u
#define  TICK_TASK_STK_SIZE         256u
#define  TICK_TASK_CFG              .TickTaskCfg = \
{                                                  \
    .StkBasePtr = &TickTaskStk[0],                 \
    .StkSize    = (TICK_TASK_STK_SIZE),            \
    .Prio       = TICK_TASK_PRIO,                  \
    .RateHz     = 1000u                            \
},
#else
#define  TICK_TASK_CFG
#endif

// Idle Task Configuration
#if (OS_CFG_TASK_IDLE_EN == DEF_ENABLED)
#define  IDLE_TASK_STK_SIZE         256u
#define  IDLE_TASK_CFG              .IdleTask = \
{                                               \
    .StkBasePtr  = &IdleTaskStk[0],             \
    .StkSize     = IDLE_TASK_STK_SIZE           \
},
#else
#define  IDLE_TASK_CFG
#endif

// Timer Task Configuration
#if (OS_CFG_TMR_EN == DEF_ENABLED)
#define  TIMER_TASK_PRIO            4u
#define  TIMER_TASK_STK_SIZE        256u
#define  TIMER_TASK_CFG             .TmrTaskCfg = \
{                                                 \
    .StkBasePtr = &TimerTaskStk[0],               \
    .StkSize    = TIMER_TASK_STK_SIZE,            \
    .Prio       = TIMER_TASK_PRIO,                \
    .RateHz     = 10u                             \
},
#else
#define  TIMER_TASK_CFG
#endif

// ISR Configuration
#define  ISR_STK_SIZE               256u
#define  ISR_CFG                        .ISR = \
{                                              \
    .StkBasePtr  = (CPU_STK*) &ISRStk[0],      \
    .StkSize     = (ISR_STK_SIZE)              \
},

/* Define RTOS_DEBUG_MODE=DEF_ENABLED at the project level,
 * for enabling debug information for Micrium Probe.*/
#if (RTOS_DEBUG_MODE == DEF_ENABLED)
#define STAT_TASK_CFG          .StatTaskCfg = \
{                                             \
    .StkBasePtr = DEF_NULL,                   \
    .StkSize    = 256u,                       \
    .Prio       = KERNEL_STAT_TASK_PRIO_DFLT, \
    .RateHz     = 10u                         \
},

#define  OS_INIT_CFG_APP            { \
    ISR_CFG                           \
    IDLE_TASK_CFG                     \
    TICK_TASK_CFG                     \
    TIMER_TASK_CFG                    \
    STAT_TASK_CFG                     \
    .MsgPoolSize     = 0u,            \
    .TaskStkLimit    = 0u,            \
    .MemSeg          = DEF_NULL       \
}
#else
#define  OS_INIT_CFG_APP            { \
    ISR_CFG                           \
    IDLE_TASK_CFG                     \
    TICK_TASK_CFG                     \
    TIMER_TASK_CFG                    \
    .MsgPoolSize     = 0u,            \
    .TaskStkLimit    = 0u,            \
    .MemSeg          = DEF_NULL       \
}
#endif

#define  COMMON_INIT_CFG_APP        { \
    .CommonMemSegPtr = DEF_NULL       \
}

#define  PLATFORM_MGR_INIT_CFG_APP  { \
    .PoolBlkQtyInit = 0u,             \
    .PoolBlkQtyMax  = 0u              \
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                        LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 *********************************************************************************************************
 */

// Main Task
static  OS_TCB   App_Main_Task_TCB;
static  CPU_STK  App_Main_Task_Stk[App_Main_Task_STK_SIZE];

// Idle Task
#if (OS_CFG_TASK_IDLE_EN == DEF_ENABLED)
static  CPU_STK  IdleTaskStk[IDLE_TASK_STK_SIZE];
#endif

// Tick Task
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
static  CPU_STK  TickTaskStk[TICK_TASK_STK_SIZE];
#endif

// Timer Task
#if (OS_CFG_TMR_EN == DEF_ENABLED)
static  CPU_STK  TimerTaskStk[TIMER_TASK_STK_SIZE];
#endif

// ISR Stack
static  CPU_STK  ISRStk[ISR_STK_SIZE];

const   OS_INIT_CFG             OS_InitCfg          = OS_INIT_CFG_APP;
const   COMMON_INIT_CFG         Common_InitCfg      = COMMON_INIT_CFG_APP;
const   PLATFORM_MGR_INIT_CFG   PlatformMgr_InitCfg = PLATFORM_MGR_INIT_CFG_APP;

enum bg_thermometer_temperature_measurement_flag{
  bg_thermometer_temperature_measurement_flag_units    =0x1,
  bg_thermometer_temperature_measurement_flag_timestamp=0x2,
  bg_thermometer_temperature_measurement_flag_type     =0x4,
};


/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                        EXTERN GLOBAL VARIABLES
 *********************************************************************************************************
 *********************************************************************************************************
 */

extern IAP_STR iap_str;

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 *********************************************************************************************************
 */
static  void     App_Main_Task      (void *p_arg);

static  void     idleHook(void);
static  void     setupHooks(void);
//#ifdef OTA
//static uint8_t boot_to_dfu = 0;
//#endif

static inline uint32_t bg_uint32_to_float(uint32_t mantissa, int32_t exponent);
static inline void bg_thermometer_create_measurement(uint8_t* buffer, uint32_t measurement, int fahrenheit);
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          GLOBAL FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
extern OS_TCB   ApplicationTaskTCB;
extern OS_TCB   Ble_Notify_Task_TCB;
extern OS_TCB   App_Ble_Parse_Task_TCB;
extern OS_TCB   Ble_Event_Task_TCB;
extern OS_TCB   App_Uart_Send_Task_TCB;
extern OS_TCB   App_Uart_Poll_Task_TCB;
extern OS_TCB   App_Uart_Parse_Task_TCB;
extern OS_TCB   BluetoothTaskTCB;
extern OS_TCB   LinklayerTaskTCB;
void system_task_stack_calc(void)
{
	RTOS_ERR err;
	CPU_STK_SIZE free,used;

	OSTaskStkChk (&App_Main_Task_TCB, &free, &used, &err);
	DBG("App_Main_Task  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&ApplicationTaskTCB, &free, &used, &err);
	DBG("ApplicationTask  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&Ble_Notify_Task_TCB, &free, &used, &err);
	DBG("Ble_Notify_Task  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&App_Ble_Parse_Task_TCB, &free, &used, &err);
	DBG("App_Ble_Parse_Task  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&Ble_Event_Task_TCB, &free, &used, &err);
	DBG("Ble_Event_Task  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&App_Uart_Send_Task_TCB, &free, &used, &err);
	DBG("App_Uart_Send_Task  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&App_Uart_Poll_Task_TCB, &free, &used, &err);
	DBG("App_Uart_Poll_Task  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&App_Uart_Parse_Task_TCB, &free, &used, &err);
	DBG("App_Uart_Parse_Task  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&BluetoothTaskTCB, &free, &used, &err);
	DBG("BluetoothTask  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	OSTaskStkChk (&LinklayerTaskTCB, &free, &used, &err);
	DBG("LinklayerTask  used/free:%d/%d  usage:%%%d\r\n",used,free,(used*100)/(used+free));

	DBG("\r\n");
	DBG("\r\n");
}
/*
 *********************************************************************************************************
 *                                                main()
 *
 * Description : This is the standard entry point for C applications. It is assumed that your code will
 *               call main() once you have performed all necessary initialization.
 *
 * Argument(s) : None.
 *
 * Return(s)   : None.
 *
 * Note(s)     : None.
 *********************************************************************************************************
 */
int main(void)
{
  RTOS_ERR  err;

  // Initialize device
  initMcu();
  // Initialize board
  initBoard();
  // Initialize application
  initApp();

  CMU_ClockEnable(cmuClock_PRS, true);

  //system already initialized by enter_DefaultMode_from_RESET
  //BSP_SystemInit();                                           /* Initialize System.                                   */

  // MTM: Not needed anymore
  //OS_ConfigureTickTask(&tickTaskCfg);

  OSInit(&err);                                                 /* Initialize the Kernel.                               */
                                                                /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSTaskCreate(&App_Main_Task_TCB,                         /* Create the Start Task.                               */
               "App_Main_Task",
			   App_Main_Task,
               0,
			   App_Main_Task_PRIO,
               &App_Main_Task_Stk[0],
               (App_Main_Task_STK_SIZE / 10u),
			   App_Main_Task_STK_SIZE,
               0,
               0,
               0,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &err);
  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSStart(&err);                                                /* Start the kernel.                                    */
                                                                /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  return (1);
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
/**
 * Convert mantissa & exponent values to IEEE float type
 */
static inline uint32_t bg_uint32_to_float(uint32_t mantissa, int32_t exponent)
{
  return (mantissa & 0xffffff) | (uint32_t)(exponent << 24);
}

/**
 * Create temperature measurement value from IEEE float and temperature type flag
 */
static inline void bg_thermometer_create_measurement(uint8_t* buffer, uint32_t measurement, int fahrenheit)
{
  buffer[0] = fahrenheit ? bg_thermometer_temperature_measurement_flag_units : 0;
  buffer[1] = measurement & 0xff;
  buffer[2] = (measurement >> 8) & 0xff;
  buffer[3] = (measurement >> 16) & 0xff;
  buffer[4] = (measurement >> 24) & 0xff;
}


/*
 *********************************************************************************************************
 *                                          Ex_MainStartTask()
 *
 * Description : This is the task that will be called by the Startup when all services are initializes
 *               successfully.
 *
 * Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
 *
 * Return(s)   : None.
 *
 * Notes       : None.
 *********************************************************************************************************
 */
static  void  App_Main_Task(void *p_arg)
{
	RTOS_ERR  err;
	uint8_t fw_ver[3] = FW_VER;
	int led_flag = 0;
	int temperature_counter = 20;
	uint32_t heart_counter = 0;

	PP_UNUSED_PARAM(p_arg);                                       /* Prevent compiler warning.                            */

	// MTM: Move to AFTER Micrium OS starts
	BSP_TickInit();                                               /* Initialize Kernel tick source.                       */
	setupHooks();

#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
	OSStatTaskCPUUsageInit(&err);                                 /* Initialize CPU Usage.                                */
																/*   Check error code.                                  */
	APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE),; );
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
	CPU_IntDisMeasMaxCurReset();                                  /* Initialize interrupts disabled measurement.          */
#endif

	BSP_OS_Init();                                                /* Initialize the BSP. It is expected that the BSP ...  */
                                                                /* ... will register all the hardware controller to ... */
                                                                /* ... the platform manager at this moment.             */
	rtt_context_init();
	ble_context_init();
	uart_context_init();




//    mbedtls_aes_context aes_ctx;
//    //密钥数值
//    unsigned char key[16] ={ 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00 };
//    //明文空间
//    unsigned char plain[16] = "helloworld";
//    //解密后明文的空间
//    unsigned char dec_plain[16] = { 0 };
//    //密文空间
//    unsigned char cipher[16] = { 0 };
//    mbedtls_aes_init(&aes_ctx);
//    //设置加密密钥
//    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
//    DBG("\n*********** step1:%s\n", plain);
//    mbedtls_aes_encrypt(&aes_ctx, plain, cipher);
////    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, plain, cipher);
//    DBG("\n*********** step2:%s\n", cipher);
//    //设置解密密钥
//    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);
//    mbedtls_aes_decrypt(&aes_ctx, cipher, dec_plain);
////    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, cipher, dec_plain);
//    DBG("\n*********** step3:%s\n", dec_plain);
//    mbedtls_aes_free(&aes_ctx);



	DBG("boot done the fw is %d.%d.%d\r\n",fw_ver[0],fw_ver[1],fw_ver[2]);

	//查看重启原因是否是升级引起
	mble_record_read(1, &iap_str.finished, 1);
	if(iap_str.finished == 0xAA)
	{
		iap_str.finished = 0;
		mble_record_write(1, &iap_str.finished, 1);
		iap_done(0);
		DBG("bootloader upgrade success\r\n");
	}

	// Start the Thermometer Loop. This call does NOT return.
	while (DEF_TRUE)
	{
		OSTimeDlyHMSM(0, 0, 1, 0,
					  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
					  &err);

		if(led_flag)
		{
			led_flag = 0;
			BSP_LedSet(0);
			BSP_LedClear(1);
		}
		else
		{
			led_flag = 1;
			BSP_LedSet(1);
			BSP_LedClear(0);
		}

		temperature_counter++;
		if (temperature_counter > 40)
		{
			temperature_counter = 20;

			heart_counter++;
			DBG("system heart counter = %d\r\n",heart_counter);
	//			system_task_stack_calc();
		}

		uint8_t temp_buffer[5];
		bg_thermometer_create_measurement(temp_buffer,
										  bg_uint32_to_float(temperature_counter, 0),
										  0);
		gecko_cmd_gatt_server_send_characteristic_notification(0xff, gattdb_temperature_measurement, 5, temp_buffer);
  }

  /* Done starting everyone else so let's exit */
  // MTM: Remove Delete
  //OSTaskDel((OS_TCB *)0, &err);
}

/***************************************************************************//**
 * @brief
 *   This is the idle hook.
 *
 * @detail
 *   This will be called by the Micrium OS idle task when there is no other
 *   task ready to run. We just enter the lowest possible energy mode.
 ******************************************************************************/
void SleepAndSyncProtimer();
static void idleHook(void)
{
  /* Put MCU in the lowest sleep mode available, usually EM2 */
  SleepAndSyncProtimer();
}

/***************************************************************************//**
 * @brief
 *   Setup the Micrium OS hooks. We are only using the idle task hook in this
 *   example. See the Mcirium OS documentation for more information on the
 *   other available hooks.
 ******************************************************************************/
static void setupHooks(void)
{
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  /* Don't allow EM3, since we use LF clocks. */
  SLEEP_SleepBlockBegin(sleepEM2);
  OS_AppIdleTaskHookPtr = idleHook;
  CPU_CRITICAL_EXIT();
}


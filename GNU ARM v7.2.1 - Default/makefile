################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include util/third_party/mbedtls/sl_crypto/src/subdir.mk
-include util/third_party/SEGGER_RTT/subdir.mk
-include protocol/bluetooth/lib/EFR32BG13P/GCC/subdir.mk
-include protocol/bluetooth/ble_stack/src/soc/subdir.mk
-include portable/subdir.mk
-include platform/micrium_os/ports/source/gnu/subdir.mk
-include platform/micrium_os/ports/source/generic/subdir.mk
-include platform/micrium_os/kernel/source/subdir.mk
-include platform/micrium_os/cpu/source/subdir.mk
-include platform/micrium_os/common/source/rtos/subdir.mk
-include platform/micrium_os/common/source/platform_mgr/subdir.mk
-include platform/micrium_os/common/source/lib/subdir.mk
-include platform/micrium_os/common/source/kal/subdir.mk
-include platform/micrium_os/common/source/common/subdir.mk
-include platform/micrium_os/common/source/collections/subdir.mk
-include platform/micrium_os/bsp/siliconlabs/generic/source/subdir.mk
-include platform/emlib/src/subdir.mk
-include platform/emdrv/uartdrv/src/subdir.mk
-include platform/emdrv/sleep/src/subdir.mk
-include platform/emdrv/rtcdrv/src/subdir.mk
-include platform/emdrv/gpiointerrupt/src/subdir.mk
-include platform/emdrv/dmadrv/src/subdir.mk
-include platform/bootloader/api/subdir.mk
-include platform/Device/SiliconLabs/EFR32BG13P/Source/GCC/subdir.mk
-include platform/Device/SiliconLabs/EFR32BG13P/Source/subdir.mk
-include hardware/kit/common/drivers/subdir.mk
-include hardware/kit/common/bsp/subdir.mk
-include app/bluetooth/common/util/subdir.mk
-include app/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: wifi_expander.axf

# Tool invocations
wifi_expander.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "C:\Users\Administrator\SimplicityStudio\v4_workspace\wifi_expander\efr32bg13p632f512gm48.ld" -Xlinker --gc-sections -Xlinker -Map="wifi_expander.map" -mfpu=fpv4-sp-d16 -mfloat-abi=softfp --specs=nano.specs -o wifi_expander.axf -Wl,--start-group "./app/ble.o" "./app/buf_factory.o" "./app/list.o" "./app/rtt.o" "./app/uart.o" "./app/uart_communicate.o" "./app/uart_ptl.o" "./app/bluetooth/common/util/infrastructure.o" "./application_properties.o" "./gatt_db.o" "./init_app.o" "./init_board.o" "./init_mcu.o" "./main.o" "./pti.o" "./hardware/kit/common/bsp/bsp_stk.o" "./hardware/kit/common/bsp/bsp_stk_leds.o" "./hardware/kit/common/drivers/mx25flash_spi.o" "./hardware/kit/common/drivers/udelay.o" "./platform/Device/SiliconLabs/EFR32BG13P/Source/GCC/startup_efr32bg13p.o" "./platform/Device/SiliconLabs/EFR32BG13P/Source/system_efr32bg13p.o" "./platform/bootloader/api/btl_interface.o" "./platform/bootloader/api/btl_interface_storage.o" "./platform/emdrv/dmadrv/src/dmadrv.o" "./platform/emdrv/gpiointerrupt/src/gpiointerrupt.o" "./platform/emdrv/rtcdrv/src/rtcdriver.o" "./platform/emdrv/sleep/src/sleep.o" "./platform/emdrv/uartdrv/src/uartdrv.o" "./platform/emlib/src/em_assert.o" "./platform/emlib/src/em_burtc.o" "./platform/emlib/src/em_cmu.o" "./platform/emlib/src/em_core.o" "./platform/emlib/src/em_cryotimer.o" "./platform/emlib/src/em_crypto.o" "./platform/emlib/src/em_emu.o" "./platform/emlib/src/em_eusart.o" "./platform/emlib/src/em_gpio.o" "./platform/emlib/src/em_i2c.o" "./platform/emlib/src/em_ldma.o" "./platform/emlib/src/em_leuart.o" "./platform/emlib/src/em_msc.o" "./platform/emlib/src/em_rmu.o" "./platform/emlib/src/em_rtcc.o" "./platform/emlib/src/em_se.o" "./platform/emlib/src/em_system.o" "./platform/emlib/src/em_timer.o" "./platform/emlib/src/em_usart.o" "./platform/micrium_os/bsp/siliconlabs/generic/source/bsp_cpu.o" "./platform/micrium_os/bsp/siliconlabs/generic/source/bsp_os.o" "./platform/micrium_os/bsp/siliconlabs/generic/source/bsp_tick_rtcc.o" "./platform/micrium_os/common/source/collections/bitmap.o" "./platform/micrium_os/common/source/collections/map.o" "./platform/micrium_os/common/source/collections/slist.o" "./platform/micrium_os/common/source/common/common.o" "./platform/micrium_os/common/source/kal/kal_kernel.o" "./platform/micrium_os/common/source/lib/lib_ascii.o" "./platform/micrium_os/common/source/lib/lib_math.o" "./platform/micrium_os/common/source/lib/lib_mem.o" "./platform/micrium_os/common/source/lib/lib_str.o" "./platform/micrium_os/common/source/platform_mgr/platform_mgr.o" "./platform/micrium_os/common/source/rtos/rtos_err_str.o" "./platform/micrium_os/cpu/source/cpu_core.o" "./platform/micrium_os/kernel/source/os_cfg_app.o" "./platform/micrium_os/kernel/source/os_core.o" "./platform/micrium_os/kernel/source/os_dbg.o" "./platform/micrium_os/kernel/source/os_flag.o" "./platform/micrium_os/kernel/source/os_mem.o" "./platform/micrium_os/kernel/source/os_mon.o" "./platform/micrium_os/kernel/source/os_msg.o" "./platform/micrium_os/kernel/source/os_mutex.o" "./platform/micrium_os/kernel/source/os_prio.o" "./platform/micrium_os/kernel/source/os_q.o" "./platform/micrium_os/kernel/source/os_sem.o" "./platform/micrium_os/kernel/source/os_stat.o" "./platform/micrium_os/kernel/source/os_task.o" "./platform/micrium_os/kernel/source/os_tick.o" "./platform/micrium_os/kernel/source/os_time.o" "./platform/micrium_os/kernel/source/os_tmr.o" "./platform/micrium_os/kernel/source/os_var.o" "./platform/micrium_os/ports/source/generic/armv6m_v7m_cpu_int.o" "./platform/micrium_os/ports/source/gnu/armv7m_cpu_a.o" "./platform/micrium_os/ports/source/gnu/armv7m_cpu_c.o" "./platform/micrium_os/ports/source/gnu/armv7m_os_cpu_a.o" "./platform/micrium_os/ports/source/gnu/armv7m_os_cpu_c.o" "./portable/mble_api.o" "./protocol/bluetooth/ble_stack/src/soc/rtos_bluetooth.o" "./util/third_party/SEGGER_RTT/SEGGER_RTT.o" "./util/third_party/SEGGER_RTT/SEGGER_RTT_printf.o" "./util/third_party/mbedtls/sl_crypto/src/crypto_aes.o" "./util/third_party/mbedtls/sl_crypto/src/crypto_ble.o" "./util/third_party/mbedtls/sl_crypto/src/crypto_ecp.o" "./util/third_party/mbedtls/sl_crypto/src/crypto_management.o" "./util/third_party/mbedtls/sl_crypto/src/crypto_sha.o" "./util/third_party/mbedtls/sl_crypto/src/trng.o" "C:\Users\Administrator\SimplicityStudio\v4_workspace\wifi_expander\platform\radio\rail_lib\autogen\librail_release\librail_efr32xg13_gcc_release.a" "C:\Users\Administrator\SimplicityStudio\v4_workspace\wifi_expander\protocol\bluetooth\lib\EFR32BG13P\GCC\libbluetooth.a" "C:\Users\Administrator\SimplicityStudio\v4_workspace\wifi_expander\protocol\bluetooth\lib\EFR32BG13P\GCC\libmbedtls.a" "C:\Users\Administrator\SimplicityStudio\v4_workspace\wifi_expander\protocol\bluetooth\lib\EFR32BG13P\GCC\libpsstore.a" -lm -Wl,--end-group -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: wifi_expander.hex'
	arm-none-eabi-objcopy -O ihex "wifi_expander.axf" "wifi_expander.hex"
	@echo ' '

	@echo 'Building bin file: wifi_expander.bin'
	arm-none-eabi-objcopy -O binary "wifi_expander.axf" "wifi_expander.bin"
	@echo ' '

	@echo 'Building s37 file: wifi_expander.s37'
	arm-none-eabi-objcopy -O srec "wifi_expander.axf" "wifi_expander.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "wifi_expander.axf"
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) wifi_expander.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

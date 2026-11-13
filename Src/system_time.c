/*
 ******************************************************************************
 * File Name    : system_time.c
 * Description  : SysTick based system timebase.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include <stdint.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "system_time.h"

#define SYSTEM_CORE_CLOCK_HZ    (16000000u)
#define TIME_TICK_HZ            (1000u)

static volatile uint32_t gu4g_system_time_ms = 0u;

void vdg_Time_Init(void)
{
    (void)SysTick_Config(SYSTEM_CORE_CLOCK_HZ / TIME_TICK_HZ);
}

uint32_t u4g_Time_GetMs(void)
{
    return gu4g_system_time_ms;
}

void SysTick_Handler(void)
{
    gu4g_system_time_ms++;
}

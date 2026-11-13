/*
 ******************************************************************************
 * File Name    : watchdog_driver.c
 * Description  : IWDG configuration.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include <stdint.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "watchdog_driver.h"

void vdg_WDG_Init(void)
{
    IWDG->KR = 0x5555u;
    IWDG->PR = 0x4u;
    IWDG->RLR = 2000u;
    IWDG->KR = 0xAAAAu;
    IWDG->KR = 0xCCCCu;
}

void vdg_WDG_Feed(void)
{
    IWDG->KR = 0xAAAAu;
}

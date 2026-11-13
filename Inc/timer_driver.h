/*
 ******************************************************************************
 * File Name    : timer_driver.h
 * Description  : TIM3/TIM4 driver.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BUZZER_MODE_OFF = 0,     /* Silent. */
    BUZZER_MODE_BEEP,        /* Beep, interval depends on risk level. */
    BUZZER_MODE_EMERGENCY    /* Emergency alarm: long, loud pattern. */
} BuzzerMode_t;

void vdg_TIMER_Init(void);
uint32_t u4g_TIMER_GetTickMs(void);
void vdg_TIMER_ServoSetPercent(uint8_t u1t_percent);
void vdg_TIMER_BuzzerSetMode(BuzzerMode_t xe_mode, uint8_t u1t_risk_level);
bool xbg_TIMER_TakeSecondEvent(void);
bool xbg_TIMER_TakeHalfSecondEvent(void);

#endif /* TIMER_DRIVER_H */

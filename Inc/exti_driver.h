/*
 ******************************************************************************
 * File Name    : exti_driver.h
 * Description  : Button EXTI event driver.
 * Date         : 2026-10-08
 ******************************************************************************
 */

#ifndef EXTI_DRIVER_H
#define EXTI_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BUTTON_SELECT = 0,
    BUTTON_CONFIRM,
    BUTTON_EMERGENCY,
    BUTTON_CANCEL,
    BUTTON_LOCK,
    BUTTON_SHOCK_SENSOR
} ButtonId_t;

void vdg_EXTI_Init(void);
bool xbg_EXTI_GetPress(ButtonId_t xe_button);
bool xbg_EXTI_GetRelease(ButtonId_t xe_button);

#endif /* EXTI_DRIVER_H */

/*
 ******************************************************************************
 * File Name    : button_manager.c
 * Description  : Debounce, long-press and button event handling.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "system_time.h"
#include "exti_driver.h"
#include "button_manager.h"

static uint32_t gu4g_emergency_press_start_ms = 0u;
static bool gxb_emergency_pressed = false;

void vdg_Button_Init(void)
{
    gu4g_emergency_press_start_ms = 0u;
    gxb_emergency_pressed = false;
}

ButtonEvent_t xeig_Button_Process(uint32_t u4t_now_ms)
{
    ButtonEvent_t xe_result = BUTTON_EVENT_NONE;
    bool xb_emergency_ready = false;

    if (xbg_EXTI_GetPress(BUTTON_SELECT) == true)
    {
        xe_result = BUTTON_EVENT_SELECT;
    }
    else
    {
        if (xbg_EXTI_GetPress(BUTTON_CONFIRM) == true)
        {
            xe_result = BUTTON_EVENT_CONFIRM;
        }
        else
        {
            if (xbg_EXTI_GetPress(BUTTON_EMERGENCY) == true)
            {
                gu4g_emergency_press_start_ms = u4t_now_ms;
                gxb_emergency_pressed = true;
            }
            else
            {
                /* No emergency press event. */
            }

            if (gxb_emergency_pressed == true)
            {
                if ((u4t_now_ms - gu4g_emergency_press_start_ms) >=
                    EMERGENCY_LONG_PRESS_MS)
                {
                    xb_emergency_ready = true;
                }
                else
                {
                    /* Long press time not reached. */
                }
            }

            if (xbg_EXTI_GetRelease(BUTTON_EMERGENCY) == true)
            {
                gxb_emergency_pressed = false;
            }
            else
            {
                /* No emergency release event. */
            }

            if (xb_emergency_ready == true)
            {
                gxb_emergency_pressed = false;
                xe_result = BUTTON_EVENT_EMERGENCY;
            }
            else
            {
                if (xbg_EXTI_GetPress(BUTTON_CANCEL) == true)
                {
                    xe_result = BUTTON_EVENT_CANCEL;
                }
                else if (xbg_EXTI_GetPress(BUTTON_LOCK) == true)
                {
                    xe_result = BUTTON_EVENT_LOCK;
                }
                else if (xbg_EXTI_GetPress(BUTTON_SHOCK_SENSOR) == true)
                {
                    xe_result = BUTTON_EVENT_SHOCK;
                }
                else
                {
                    /* No other button event. */
                }
            }
        }
    }

    return xe_result;
}

/*
 ******************************************************************************
 * File Name    : gate_controller.c
 * Description  : Gate command controller.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "board_config.h"
#include "timer_driver.h"
#include "gate_controller.h"

static uint8_t gu1g_gate_command = 20u;
static uint8_t gu1g_gate_target = 20u;

void vdg_Gate_Init(void)
{
    gu1g_gate_command = 20u;
    gu1g_gate_target = 20u;
    vdg_TIMER_ServoSetPercent(gu1g_gate_command);
}

uint8_t u1g_Gate_GetCommand(void)
{
    return gu1g_gate_command;
}

void vdg_Gate_SetAutomaticTarget(uint8_t u1t_target)
{
    if (u1t_target <= PERCENT_FULL_SCALE)
    {
        gu1g_gate_target = u1t_target;
    }
    else
    {
        gu1g_gate_target = PERCENT_FULL_SCALE;
    }
}

void vdg_Gate_SetManualTarget(uint8_t u1t_target)
{
    if (u1t_target <= PERCENT_FULL_SCALE)
    {
        gu1g_gate_command = u1t_target;
        gu1g_gate_target = u1t_target;
    }
    else
    {
        gu1g_gate_command = PERCENT_FULL_SCALE;
        gu1g_gate_target = PERCENT_FULL_SCALE;
    }

    vdg_TIMER_ServoSetPercent(gu1g_gate_command);
}

void vdg_Gate_SetSafeTarget(void)
{
    /* In this model, a larger gate percentage means more discharge. */
    gu1g_gate_target = PERCENT_FULL_SCALE;
}

void vdg_Gate_ForceImmediate(uint8_t u1t_percent)
{
    /* ตั้งทั้ง command และ target เป็นค่าเดียวกันทันที แล้วสั่ง servo
     * โดยตรง ข้าม rate-limit ปกติ ใช้สำหรับเหตุการณ์ safety-critical
     * เช่น LOW_WATER / WARNING / EMERGENCY ที่ต้องตอบสนองทันที          */
    if (u1t_percent <= PERCENT_FULL_SCALE)
    {
        gu1g_gate_command = u1t_percent;
        gu1g_gate_target = u1t_percent;
    }
    else
    {
        gu1g_gate_command = PERCENT_FULL_SCALE;
        gu1g_gate_target = PERCENT_FULL_SCALE;
    }

    vdg_TIMER_ServoSetPercent(gu1g_gate_command);
}

void vdg_Gate_ApplyRateLimited(bool xb_step_event)
{
    if (xb_step_event == true)
    {
        if (gu1g_gate_command < gu1g_gate_target)
        {
            uint8_t u1t_next =
                (uint8_t)(gu1g_gate_command + GATE_RATE_STEP_PERCENT);

            if (u1t_next > gu1g_gate_target)
            {
                gu1g_gate_command = gu1g_gate_target;
            }
            else
            {
                gu1g_gate_command = u1t_next;
            }
        }
        else
        {
            if (gu1g_gate_command > gu1g_gate_target)
            {
                if (gu1g_gate_command >= GATE_RATE_STEP_PERCENT)
                {
                    uint8_t u1t_next =
                        (uint8_t)(gu1g_gate_command -
                                  GATE_RATE_STEP_PERCENT);

                    if (u1t_next < gu1g_gate_target)
                    {
                        gu1g_gate_command = gu1g_gate_target;
                    }
                    else
                    {
                        gu1g_gate_command = u1t_next;
                    }
                }
                else
                {
                    gu1g_gate_command = 0u;
                }
            }
            else
            {
                /* Target already reached. */
            }
        }

        vdg_TIMER_ServoSetPercent(gu1g_gate_command);
    }
    else
    {
        /* No rate-limiting event. */
    }
}

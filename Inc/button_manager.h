/*
 ******************************************************************************
 * File Name    : button_manager.h
 * Description  : Application-level button logic.
 * Date         : 2026-10-08
 ******************************************************************************
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SELECT,
    BUTTON_EVENT_CONFIRM,
    BUTTON_EVENT_EMERGENCY,
    BUTTON_EVENT_CANCEL,
    BUTTON_EVENT_LOCK,
    BUTTON_EVENT_SHOCK
} ButtonEvent_t;

void vdg_Button_Init(void);
ButtonEvent_t xeig_Button_Process(uint32_t u4t_now_ms);

#endif /* BUTTON_MANAGER_H */

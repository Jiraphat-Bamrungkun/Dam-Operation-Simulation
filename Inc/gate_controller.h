/*
 ******************************************************************************
 * File Name    : gate_controller.h
 * Description  : Gate target logic.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#ifndef GATE_CONTROLLER_H
#define GATE_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

void vdg_Gate_Init(void);
uint8_t u1g_Gate_GetCommand(void);
void vdg_Gate_SetAutomaticTarget(uint8_t u1t_target);
void vdg_Gate_SetManualTarget(uint8_t u1t_target);
void vdg_Gate_SetSafeTarget(void);
void vdg_Gate_ApplyRateLimited(bool xb_step_event);
void vdg_Gate_ForceImmediate(uint8_t u1t_percent);

#endif /* GATE_CONTROLLER_H */

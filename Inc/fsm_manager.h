/*
 ******************************************************************************
 * File Name    : fsm_manager.h
 * Description  : DAM-GUARD finite state machine.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#ifndef FSM_MANAGER_H
#define FSM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "dam_model.h"
#include "button_manager.h"

typedef enum
{
    STATE_INIT = 0,
    STATE_NORMAL,
    STATE_WARNING,
    STATE_FLOOD_CONTROL,
    STATE_EMERGENCY,
    STATE_RECOVERY,
    STATE_LOW_WATER,
    STATE_FAULT
} DamState_t;

typedef enum
{
    MODE_AUTO = 0,
    MODE_MANUAL,
    MODE_EARTHQUAKE_LOCK
} ControlMode_t;

typedef enum
{
    PARAM_WARNING_LEVEL = 0,
    PARAM_FLOOD_LEVEL,
    PARAM_CRITICAL_LEVEL,
    PARAM_LOW_WATER_LEVEL,
    PARAM_STABLE_TIME,
    PARAM_MANUAL_GATE,
    PARAM_MODE,
    PARAM_COUNT
} ParameterId_t;

typedef struct
{
    uint8_t warning_level;
    uint8_t flood_level;
    uint8_t critical_level;
    uint8_t low_water_level;
    uint8_t stable_time_s;
    uint8_t manual_gate_percent;
    ControlMode_t mode;
} DamConfig_t;

typedef struct
{
    DamState_t state;
    DamState_t previous_state;
    ParameterId_t selected_parameter;
    bool editing;
    uint32_t state_enter_ms;
    uint32_t recovery_start_ms;
    uint32_t manual_last_action_ms;
    uint8_t fault_code;
    uint8_t edit_value;
} FsmContext_t;

void vdg_FSM_Init(FsmContext_t *pxg_ctx, DamConfig_t *pxg_cfg);
void vdg_FSM_Process(FsmContext_t *pxg_ctx,
                     DamConfig_t *pxg_cfg,
                     DamData_t *pxg_data,
                     ButtonEvent_t xe_event,
                     uint32_t u4t_now_ms);
DamState_t xeg_FSM_GetState(const FsmContext_t *pxg_ctx);
uint8_t u1g_FSM_GetStateCode(const FsmContext_t *pxg_ctx);
void vdg_FSM_ApplyConfigValue(DamConfig_t *pxg_cfg,
                              ParameterId_t xe_param,
                              uint8_t u1t_value);

#endif /* FSM_MANAGER_H */

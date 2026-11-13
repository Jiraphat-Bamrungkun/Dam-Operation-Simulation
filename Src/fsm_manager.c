/*
 ******************************************************************************
 * File Name    : fsm_manager.c
 * Description  : Event-driven FSM and risk-based safety logic.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "board_config.h"
#include "gate_controller.h"
#include "uart_driver.h"
#include "fsm_manager.h"

static bool xbs_FSM_ConfigValid(const DamConfig_t *pxg_cfg);
static void vds_FSM_SetState(FsmContext_t *pxg_ctx,
                             DamState_t xe_state,
                             uint32_t u4t_now_ms);
static uint8_t u1s_FSM_AutoGateTarget(const DamConfig_t *pxg_cfg,
                                      const DamData_t *pxg_data);
static bool xbs_FSM_StableForRecovery(const FsmContext_t *pxg_ctx,
                                      const DamConfig_t *pxg_cfg,
                                      const DamData_t *pxg_data,
                                      uint32_t u4t_now_ms);
static bool xbs_FSM_IsCritical(const DamConfig_t *pxg_cfg,
                               const DamData_t *pxg_data);
static bool xbs_FSM_IsLowWater(const DamConfig_t *pxg_cfg,
                               const DamData_t *pxg_data);

void vdg_FSM_Init(FsmContext_t *pxg_ctx, DamConfig_t *pxg_cfg)
{
    if ((pxg_ctx != NULL) && (pxg_cfg != NULL))
    {
        pxg_cfg->warning_level = DEFAULT_WARNING_LEVEL;
        pxg_cfg->flood_level = DEFAULT_FLOOD_LEVEL;
        pxg_cfg->critical_level = DEFAULT_CRITICAL_LEVEL;
        pxg_cfg->low_water_level = DEFAULT_LOW_WATER_LEVEL;
        pxg_cfg->stable_time_s = DEFAULT_STABLE_TIME_S;
        pxg_cfg->manual_gate_percent = DEFAULT_MANUAL_GATE;
        pxg_cfg->mode = MODE_AUTO;

        pxg_ctx->state = STATE_INIT;
        pxg_ctx->previous_state = STATE_INIT;
        pxg_ctx->selected_parameter = PARAM_WARNING_LEVEL;
        pxg_ctx->editing = false;
        pxg_ctx->state_enter_ms = 0u;
        pxg_ctx->recovery_start_ms = 0u;
        pxg_ctx->manual_last_action_ms = 0u;
        pxg_ctx->fault_code = 0u;
        pxg_ctx->edit_value = pxg_cfg->warning_level;
    }
    else
    {
        /* Invalid pointer. */
    }
}

void vdg_FSM_Process(FsmContext_t *pxg_ctx,
                     DamConfig_t *pxg_cfg,
                     DamData_t *pxg_data,
                     ButtonEvent_t xe_event,
                     uint32_t u4t_now_ms)
{
    if ((pxg_ctx != NULL) &&
        (pxg_cfg != NULL) &&
        (pxg_data != NULL))
    {
        if (xe_event == BUTTON_EVENT_EMERGENCY)
        {
            vds_FSM_SetState(pxg_ctx, STATE_EMERGENCY, u4t_now_ms);
        }
        else
        {
            /* No emergency button event. */
        }

        switch (pxg_ctx->state)
        {
            case STATE_INIT:
                if (xbs_FSM_ConfigValid(pxg_cfg) == true)
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_NORMAL,
                                     u4t_now_ms);
                }
                else
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_FAULT,
                                     u4t_now_ms);
                }
                break;

            case STATE_NORMAL:
                if (xbs_FSM_IsCritical(pxg_cfg, pxg_data) == true)
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_EMERGENCY,
                                     u4t_now_ms);
                }
                else
                {
                    if (xbs_FSM_IsLowWater(pxg_cfg, pxg_data) == true)
                    {
                        vds_FSM_SetState(pxg_ctx,
                                         STATE_LOW_WATER,
                                         u4t_now_ms);
                    }
                    else
                    {
                        if (pxg_data->risk_level >= pxg_cfg->warning_level)
                        {
                            vds_FSM_SetState(pxg_ctx,
                                             STATE_WARNING,
                                             u4t_now_ms);
                        }
                        else
                        {
                            /* Remain in normal state. */
                        }
                    }
                }
                break;

            case STATE_WARNING:
                if (xbs_FSM_IsCritical(pxg_cfg, pxg_data) == true)
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_EMERGENCY,
                                     u4t_now_ms);
                }
                else
                {
                    if (pxg_data->risk_level >= pxg_cfg->flood_level)
                    {
                        vds_FSM_SetState(pxg_ctx,
                                         STATE_FLOOD_CONTROL,
                                         u4t_now_ms);
                    }
                    else
                    {
                        if ((pxg_data->risk_level <
                             (pxg_cfg->warning_level - MIN_THRESHOLD_GAP)) &&
                            (pxg_data->rate_of_rise <= 0))
                        {
                            vds_FSM_SetState(pxg_ctx,
                                             STATE_NORMAL,
                                             u4t_now_ms);
                        }
                        else
                        {
                            /* Remain in warning state. */
                        }
                    }
                }
                break;

            case STATE_FLOOD_CONTROL:
                if (xbs_FSM_IsCritical(pxg_cfg, pxg_data) == true)
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_EMERGENCY,
                                     u4t_now_ms);
                }
                else
                {
                    if ((pxg_data->risk_level < pxg_cfg->flood_level) &&
                        (pxg_data->rate_of_rise <= 0))
                    {
                        vds_FSM_SetState(pxg_ctx,
                                         STATE_WARNING,
                                         u4t_now_ms);
                    }
                    else
                    {
                        /* Continue flood control. */
                    }
                }
                break;

            case STATE_EMERGENCY:
                if (xbs_FSM_StableForRecovery(pxg_ctx,
                                              pxg_cfg,
                                              pxg_data,
                                              u4t_now_ms) == true)
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_RECOVERY,
                                     u4t_now_ms);
                }
                else
                {
                    /* Remain in emergency state. */
                }
                break;

            case STATE_RECOVERY:
                if (xbs_FSM_IsCritical(pxg_cfg, pxg_data) == true)
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_EMERGENCY,
                                     u4t_now_ms);
                }
                else
                {
                    if (xbs_FSM_StableForRecovery(pxg_ctx,
                                                  pxg_cfg,
                                                  pxg_data,
                                                  u4t_now_ms) == true)
                    {
                        vds_FSM_SetState(pxg_ctx,
                                         STATE_NORMAL,
                                         u4t_now_ms);
                    }
                    else
                    {
                        if (pxg_data->risk_level >= pxg_cfg->warning_level)
                        {
                            vds_FSM_SetState(pxg_ctx,
                                             STATE_WARNING,
                                             u4t_now_ms);
                        }
                        else
                        {
                            /* Remain in recovery state. */
                        }
                    }
                }
                break;

            case STATE_LOW_WATER:
                if (xbs_FSM_IsCritical(pxg_cfg, pxg_data) == true)
                {
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_EMERGENCY,
                                     u4t_now_ms);
                }
                else
                {
                    if (pxg_data->water_level >
                        (pxg_cfg->low_water_level + MIN_THRESHOLD_GAP))
                    {
                        vds_FSM_SetState(pxg_ctx,
                                         STATE_RECOVERY,
                                         u4t_now_ms);
                    }
                    else
                    {
                        /* Continue conservation mode. */
                    }
                }
                break;

            case STATE_FAULT:
                if (xe_event == BUTTON_EVENT_CONFIRM)
                {
                    pxg_ctx->fault_code = 0u;
                    vds_FSM_SetState(pxg_ctx,
                                     STATE_INIT,
                                     u4t_now_ms);
                }
                else
                {
                    /* Wait for acknowledge. */
                }
                break;

            default:
                vds_FSM_SetState(pxg_ctx,
                                 STATE_FAULT,
                                 u4t_now_ms);
                break;
        }

        if (pxg_cfg->mode == MODE_EARTHQUAKE_LOCK)
        {
            /* Gate is locked in place for earthquake safety. Do not change gate target. */
        }
        else if (pxg_cfg->mode == MODE_MANUAL)
        {
            vdg_Gate_SetManualTarget(pxg_cfg->manual_gate_percent);
        }
        else
        {
            switch (pxg_ctx->state)
            {
                case STATE_LOW_WATER:
                    vdg_Gate_SetAutomaticTarget(0u);
                    break;
                case STATE_WARNING:
                    vdg_Gate_SetAutomaticTarget(40u);
                    break;
                case STATE_FLOOD_CONTROL:
                    vdg_Gate_SetAutomaticTarget(70u);
                    break;
                case STATE_EMERGENCY:
                    vdg_Gate_SetAutomaticTarget(100u);
                    break;
                default:
                    vdg_Gate_SetAutomaticTarget(
                        u1s_FSM_AutoGateTarget(pxg_cfg, pxg_data));
                    break;
            }
        }

        if (xe_event == BUTTON_EVENT_SELECT)
        {
            if (pxg_ctx->editing == false)
            {
                uint8_t u1t_next =
                    (uint8_t)(pxg_ctx->selected_parameter + 1u);

                if (u1t_next >= PARAM_COUNT)
                {
                    pxg_ctx->selected_parameter = PARAM_WARNING_LEVEL;
                }
                else
                {
                    pxg_ctx->selected_parameter =
                        (ParameterId_t)u1t_next;
                }
            }
            else
            {
                /* Select is ignored while editing. */
            }
        }
        else
        {
            if (xe_event == BUTTON_EVENT_CONFIRM)
            {
                pxg_ctx->editing = (bool)(!pxg_ctx->editing);
            }
            else
            {
                if (xe_event == BUTTON_EVENT_CANCEL)
                {
                    pxg_ctx->editing = false;
                }
                else
                {
                    /* No UI event. */
                }
            }
        }
    }
    else
    {
        /* Invalid pointer. */
    }
}

DamState_t xeg_FSM_GetState(const FsmContext_t *pxg_ctx)
{
    DamState_t xe_state = STATE_FAULT;

    if (pxg_ctx != NULL)
    {
        xe_state = pxg_ctx->state;
    }
    else
    {
        /* Invalid pointer. */
    }

    return xe_state;
}

uint8_t u1g_FSM_GetStateCode(const FsmContext_t *pxg_ctx)
{
    return (uint8_t)xeg_FSM_GetState(pxg_ctx);
}

void vdg_FSM_ApplyConfigValue(DamConfig_t *pxg_cfg,
                              ParameterId_t xe_param,
                              uint8_t u1t_value)
{
    if (pxg_cfg != NULL)
    {
        switch (xe_param)
        {
            case PARAM_WARNING_LEVEL:
                if (u1t_value >= PERCENT_FULL_SCALE - 2u)
                {
                    u1t_value = PERCENT_FULL_SCALE - 2u;
                }
                if (u1t_value <= MIN_THRESHOLD_GAP)
                {
                    u1t_value = MIN_THRESHOLD_GAP;
                }
                pxg_cfg->warning_level = u1t_value;

                if (pxg_cfg->flood_level <= pxg_cfg->warning_level)
                {
                    pxg_cfg->flood_level = (uint8_t)(pxg_cfg->warning_level + MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->critical_level <= pxg_cfg->flood_level)
                {
                    pxg_cfg->critical_level = (uint8_t)(pxg_cfg->flood_level + MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->low_water_level >= pxg_cfg->warning_level)
                {
                    if (pxg_cfg->warning_level > MIN_THRESHOLD_GAP)
                    {
                        pxg_cfg->low_water_level = (uint8_t)(pxg_cfg->warning_level - MIN_THRESHOLD_GAP);
                    }
                    else
                    {
                        pxg_cfg->low_water_level = 0u;
                    }
                }
                break;

            case PARAM_FLOOD_LEVEL:
                if (u1t_value >= PERCENT_FULL_SCALE - 1u)
                {
                    u1t_value = PERCENT_FULL_SCALE - 1u;
                }
                if (u1t_value <= MIN_THRESHOLD_GAP * 2u)
                {
                    u1t_value = MIN_THRESHOLD_GAP * 2u;
                }
                pxg_cfg->flood_level = u1t_value;

                if (pxg_cfg->critical_level <= pxg_cfg->flood_level)
                {
                    pxg_cfg->critical_level = (uint8_t)(pxg_cfg->flood_level + MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->warning_level >= pxg_cfg->flood_level)
                {
                    pxg_cfg->warning_level = (uint8_t)(pxg_cfg->flood_level - MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->low_water_level >= pxg_cfg->warning_level)
                {
                    if (pxg_cfg->warning_level > MIN_THRESHOLD_GAP)
                    {
                        pxg_cfg->low_water_level = (uint8_t)(pxg_cfg->warning_level - MIN_THRESHOLD_GAP);
                    }
                    else
                    {
                        pxg_cfg->low_water_level = 0u;
                    }
                }
                break;

            case PARAM_CRITICAL_LEVEL:
                if (u1t_value > PERCENT_FULL_SCALE)
                {
                    u1t_value = PERCENT_FULL_SCALE;
                }
                if (u1t_value <= MIN_THRESHOLD_GAP * 3u)
                {
                    u1t_value = MIN_THRESHOLD_GAP * 3u;
                }
                pxg_cfg->critical_level = u1t_value;

                if (pxg_cfg->flood_level >= pxg_cfg->critical_level)
                {
                    pxg_cfg->flood_level = (uint8_t)(pxg_cfg->critical_level - MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->warning_level >= pxg_cfg->flood_level)
                {
                    pxg_cfg->warning_level = (uint8_t)(pxg_cfg->flood_level - MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->low_water_level >= pxg_cfg->warning_level)
                {
                    if (pxg_cfg->warning_level > MIN_THRESHOLD_GAP)
                    {
                        pxg_cfg->low_water_level = (uint8_t)(pxg_cfg->warning_level - MIN_THRESHOLD_GAP);
                    }
                    else
                    {
                        pxg_cfg->low_water_level = 0u;
                    }
                }
                break;

            case PARAM_LOW_WATER_LEVEL:
                if (u1t_value >= PERCENT_FULL_SCALE - 3u)
                {
                    u1t_value = PERCENT_FULL_SCALE - 3u;
                }
                pxg_cfg->low_water_level = u1t_value;

                if (pxg_cfg->warning_level <= pxg_cfg->low_water_level)
                {
                    pxg_cfg->warning_level = (uint8_t)(pxg_cfg->low_water_level + MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->flood_level <= pxg_cfg->warning_level)
                {
                    pxg_cfg->flood_level = (uint8_t)(pxg_cfg->warning_level + MIN_THRESHOLD_GAP);
                }
                if (pxg_cfg->critical_level <= pxg_cfg->flood_level)
                {
                    pxg_cfg->critical_level = (uint8_t)(pxg_cfg->flood_level + MIN_THRESHOLD_GAP);
                }
                break;
            case PARAM_STABLE_TIME:
                if (u1t_value < MIN_STABLE_TIME_S)
                {
                    pxg_cfg->stable_time_s = MIN_STABLE_TIME_S;
                }
                else
                {
                    if (u1t_value > MAX_STABLE_TIME_S)
                    {
                        pxg_cfg->stable_time_s = MAX_STABLE_TIME_S;
                    }
                    else
                    {
                        pxg_cfg->stable_time_s = u1t_value;
                    }
                }
                break;
            case PARAM_MANUAL_GATE:
                pxg_cfg->manual_gate_percent = u1t_value;
                break;
            case PARAM_MODE:
                if (u1t_value >= 50u)
                {
                    pxg_cfg->mode = MODE_MANUAL;
                }
                else
                {
                    pxg_cfg->mode = MODE_AUTO;
                }
                break;
            default:
                /* No valid parameter. */
                break;
        }
    }
    else
    {
        /* Invalid pointer. */
    }
}

static bool xbs_FSM_ConfigValid(const DamConfig_t *pxg_cfg)
{
    bool xb_valid = false;

    if (pxg_cfg != NULL)
    {
        if ((pxg_cfg->low_water_level < pxg_cfg->warning_level) &&
            (pxg_cfg->warning_level < pxg_cfg->flood_level) &&
            (pxg_cfg->flood_level < pxg_cfg->critical_level) &&
            (pxg_cfg->stable_time_s >= MIN_STABLE_TIME_S) &&
            (pxg_cfg->stable_time_s <= MAX_STABLE_TIME_S))
        {
            xb_valid = true;
        }
        else
        {
            xb_valid = false;
        }
    }
    else
    {
        xb_valid = false;
    }

    return xb_valid;
}

static void vds_FSM_SetState(FsmContext_t *pxg_ctx,
                             DamState_t xe_state,
                             uint32_t u4t_now_ms)
{
    if (pxg_ctx != NULL)
    {
        if (pxg_ctx->state != xe_state)
        {
            pxg_ctx->previous_state = pxg_ctx->state;
            pxg_ctx->state = xe_state;
            pxg_ctx->state_enter_ms = u4t_now_ms;
            pxg_ctx->recovery_start_ms = u4t_now_ms;

            switch (xe_state)
            {
                case STATE_LOW_WATER:
                    vdg_Gate_ForceImmediate(0u);
                    (void)xbg_UART_SendText("STATE=LOW_WATER GATE=0%\r\n");
                    break;

                case STATE_WARNING:
                    vdg_Gate_ForceImmediate(40u);
                    (void)xbg_UART_SendText("STATE=WARNING GATE=40%\r\n");
                    break;

                case STATE_FLOOD_CONTROL:
                    vdg_Gate_ForceImmediate(70u);
                    (void)xbg_UART_SendText("STATE=FLOOD_CONTROL GATE=70%\r\n");
                    break;

                case STATE_EMERGENCY:
                    vdg_Gate_ForceImmediate(100u);
                    (void)xbg_UART_SendText("STATE=EMERGENCY GATE=100%\r\n");
                    break;

                default:
                    /* Other states use target logic in vdg_FSM_Process() */
                    break;
            }
        }
        else
        {
            /* State remains unchanged. */
        }
    }
    else
    {
        /* Invalid pointer. */
    }
}

static uint8_t u1s_FSM_AutoGateTarget(const DamConfig_t *pxg_cfg,
                                      const DamData_t *pxg_data)
{
    uint8_t u1t_target = 20u;

    if ((pxg_cfg != NULL) && (pxg_data != NULL))
    {
        if (pxg_data->water_level >= pxg_cfg->critical_level)
        {
            u1t_target = 100u;
        }
        else
        {
            if (pxg_data->water_level >= pxg_cfg->flood_level)
            {
                u1t_target = 70u;
            }
            else
            {
                if (pxg_data->water_level >= pxg_cfg->warning_level)
                {
                    u1t_target = 40u;
                }
                else
                {
                    if (pxg_data->water_level <= pxg_cfg->low_water_level)
                    {
                        u1t_target = 0u;
                    }
                    else
                    {
                        u1t_target = 20u;
                    }
                }
            }
        }
    }
    else
    {
        u1t_target = 0u;
    }

    return u1t_target;
}

static bool xbs_FSM_StableForRecovery(const FsmContext_t *pxg_ctx,
                                      const DamConfig_t *pxg_cfg,
                                      const DamData_t *pxg_data,
                                      uint32_t u4t_now_ms)
{
    uint32_t u4t_required = 0u;
    bool xb_stable = false;

    if ((pxg_ctx != NULL) &&
        (pxg_cfg != NULL) &&
        (pxg_data != NULL))
    {
        u4t_required = (uint32_t)pxg_cfg->stable_time_s * 1000u;

        if ((pxg_data->water_level < pxg_cfg->warning_level) &&
            (pxg_data->rate_of_rise <= 0) &&
            (pxg_data->rain_intensity < 60u) &&
            ((u4t_now_ms - pxg_ctx->recovery_start_ms) >= u4t_required))
        {
            xb_stable = true;
        }
        else
        {
            xb_stable = false;
        }
    }
    else
    {
        xb_stable = false;
    }

    return xb_stable;
}

static bool xbs_FSM_IsCritical(const DamConfig_t *pxg_cfg,
                               const DamData_t *pxg_data)
{
    bool xb_result = false;

    if ((pxg_cfg != NULL) && (pxg_data != NULL))
    {
        xb_result =
            (pxg_data->risk_level >= pxg_cfg->critical_level) ||
            (pxg_data->water_level >= pxg_cfg->critical_level);
    }
    else
    {
        xb_result = true;
    }

    return xb_result;
}

static bool xbs_FSM_IsLowWater(const DamConfig_t *pxg_cfg,
                               const DamData_t *pxg_data)
{
    bool xb_result = false;

    if ((pxg_cfg != NULL) && (pxg_data != NULL))
    {
        if ((pxg_data->water_level <= pxg_cfg->low_water_level) &&
            (pxg_data->rain_intensity < 30u) &&
            (pxg_data->rate_of_rise <= 0))
        {
            xb_result = true;
        }
        else
        {
            xb_result = false;
        }
    }
    else
    {
        xb_result = false;
    }

    return xb_result;
}

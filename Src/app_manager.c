/*
 ******************************************************************************
 * File Name    : app_manager.c
 * Description  : Top-level application coordinator.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "board_config.h"
#include "system_time.h"
#include "gpio_driver.h"
#include "adc_driver.h"
#include "uart_driver.h"
#include "exti_driver.h"
#include "button_manager.h"
#include "timer_driver.h"
#include "watchdog_driver.h"
#include "sevenseg.h"
#include "dam_model.h"
#include "gate_controller.h"
#include "fsm_manager.h"
#include "oled_ssd1306.h"
#include "dht11.h"
#include "app_manager.h"

static AdcData_t gxs_adc = {0u, 0u};
static DamData_t gxs_dam = {0u, 0u, 0, 0u, 20u, 0u};
static DamConfig_t gxs_cfg;
static FsmContext_t gxs_fsm;
static uint8_t gu1g_edit_value = 0u;
static bool gxb_previous_editing = false;
static uint32_t gu4g_last_status_ms = 0u;
static uint32_t gu4g_last_model_ms = 0u;
static uint32_t gu4g_last_dht_ms = 0u;
static uint8_t gu1g_temp = 0u;
static uint8_t gu1g_humidity = 0u;

static bool xbs_APP_ConfigValueAllowed(
    const DamConfig_t *pxg_cfg,
    ParameterId_t xe_param,
    uint8_t u1t_value);

static uint8_t u1s_APP_GetSelectedValue(
    const DamConfig_t *pxg_cfg,
    ParameterId_t xe_param);

static void vds_APP_SetSelectedValue(
    DamConfig_t *pxg_cfg,
    ParameterId_t xe_param,
    uint8_t u1t_value);

static uint8_t u1s_APP_MapRainPercent(uint16_t u2t_raw);
static void vds_APP_UpdateOutputs(void);
static void vds_APP_SendStatus(void);

void vdg_APP_Init(void)
{
    vdg_Time_Init();
    vdg_GPIO_Init();
    vdg_Seg7_Init();
    vdg_ADC_Init();
    vdg_UART_Init();
    vdg_EXTI_Init();
    vdg_Button_Init();
    vdg_TIMER_Init();
    vdg_WDG_Init();
    vdg_Gate_Init();
    vdg_DamModel_Init(&gxs_dam);
    vdg_FSM_Init(&gxs_fsm, &gxs_cfg);
    vdg_OLED_Init();
    vdg_DHT11_Init();

    vdg_Seg7_WriteDigit(0u);
    gu4g_last_model_ms = u4g_Time_GetMs();
    gu4g_last_status_ms = gu4g_last_model_ms;
    gu4g_last_dht_ms = gu4g_last_model_ms;
}

void vdg_APP_Run(void)
{
    uint32_t u4t_now = u4g_Time_GetMs();
    ButtonEvent_t xe_button_event = BUTTON_EVENT_NONE;

    if (xbg_ADC_GetLatest(&gxs_adc) == true)
    {
        gxs_dam.rain_intensity =
            u1s_APP_MapRainPercent(gxs_adc.rain_raw);

        if (gxs_fsm.editing == true)
        {
            if (gxs_fsm.selected_parameter == PARAM_MODE)
            {
                if (gxs_adc.config_raw >= (ADC_FULL_SCALE / 2u))
                {
                    gu1g_edit_value = PERCENT_FULL_SCALE;
                }
                else
                {
                    gu1g_edit_value = 0u;
                }
            }
            else if (gxs_fsm.selected_parameter == PARAM_STABLE_TIME)
            {
                uint8_t u1t_pot_percent = u1g_Dam_MapAdcToPercent(gxs_adc.config_raw);
                gu1g_edit_value = (uint8_t)(MIN_STABLE_TIME_S +
                    (((uint32_t)u1t_pot_percent * (uint32_t)(MAX_STABLE_TIME_S - MIN_STABLE_TIME_S)) / 100u));
            }
            else
            {
                gu1g_edit_value =
                    u1g_Dam_MapAdcToPercent(gxs_adc.config_raw);
            }

            gxs_fsm.edit_value = gu1g_edit_value;

            if (gxs_fsm.selected_parameter == PARAM_MANUAL_GATE)
            {
                gxs_cfg.manual_gate_percent = gu1g_edit_value;
            }
        }
        else
        {
            if (gxs_cfg.mode == MODE_MANUAL)
            {
                gxs_cfg.manual_gate_percent =
                    u1g_Dam_MapAdcToPercent(gxs_adc.config_raw);
            }
            else
            {
                /* Configuration input is ignored outside edit mode in Auto. */
            }
        }
    }
    else
    {
        /* No new ADC DMA result is available. */
    }

    static ControlMode_t gxe_saved_mode_before_lock = MODE_AUTO;

    xe_button_event = xeig_Button_Process(u4t_now);

    if (xe_button_event == BUTTON_EVENT_SHOCK)
    {
        /* HW-513 Shock / Vibration Sensor detected earthquake */
        if (gxs_cfg.mode != MODE_EARTHQUAKE_LOCK)
        {
            gxe_saved_mode_before_lock = gxs_cfg.mode;
            gxs_cfg.mode = MODE_EARTHQUAKE_LOCK;
        }

        xe_button_event = BUTTON_EVENT_NONE;
    }
    else if (xe_button_event == BUTTON_EVENT_LOCK)
    {
        /* Blue Button B1 (or Reset button) pressed: Unlock and restore mode */
        if (gxs_cfg.mode == MODE_EARTHQUAKE_LOCK)
        {
            gxs_cfg.mode = gxe_saved_mode_before_lock;
        }
        else
        {
            gxe_saved_mode_before_lock = gxs_cfg.mode;
            gxs_cfg.mode = MODE_EARTHQUAKE_LOCK;
        }

        xe_button_event = BUTTON_EVENT_NONE;
    }
    else
    {
        if (gxs_cfg.mode == MODE_EARTHQUAKE_LOCK)
        {
            /* Ignore all other buttons (Select, Confirm, Emergency, Cancel) while locked */
            xe_button_event = BUTTON_EVENT_NONE;
        }
    }

    if ((xe_button_event == BUTTON_EVENT_CONFIRM) &&
        (gxs_fsm.editing == true))
    {
        if (xbs_APP_ConfigValueAllowed(
                &gxs_cfg,
                gxs_fsm.selected_parameter,
                gxs_fsm.edit_value) == true)
        {
            vds_APP_SetSelectedValue(
                &gxs_cfg,
                gxs_fsm.selected_parameter,
                gxs_fsm.edit_value);
            gxs_fsm.editing = false;
        }
        else
        {
            gxs_fsm.editing = true;
            gxs_fsm.fault_code = 1u;
        }

        xe_button_event = BUTTON_EVENT_NONE;
    }
    else
    {
        /* Non-confirm or non-edit event. */
    }

    if ((u4t_now - gu4g_last_model_ms) >= SAMPLE_PERIOD_MS)
    {
        gu4g_last_model_ms = u4t_now;
        gxs_dam.gate_command = u1g_Gate_GetCommand();
        vdg_DamModel_Update(&gxs_dam);
    }
    else
    {
        /* Model update interval not reached. */
    }

    if (gxs_cfg.mode != MODE_EARTHQUAKE_LOCK)
    {
        vdg_FSM_Process(
            &gxs_fsm,
            &gxs_cfg,
            &gxs_dam,
            xe_button_event,
            u4t_now);

        vdg_Gate_ApplyRateLimited(
            xbg_TIMER_TakeHalfSecondEvent());
    }

    gxs_dam.gate_command = u1g_Gate_GetCommand();

    /* Buzzer is set before the (slow) OLED update so the alarm starts
     * as soon as the FSM changes state. */
    if (gxs_fsm.state == STATE_EMERGENCY)
    {
        vdg_TIMER_BuzzerSetMode(BUZZER_MODE_EMERGENCY, gxs_dam.risk_level);
    }
    else
    {
        if ((gxs_fsm.state == STATE_WARNING) ||
            (gxs_fsm.state == STATE_FLOOD_CONTROL) ||
            (gxs_fsm.state == STATE_FAULT))
        {
            vdg_TIMER_BuzzerSetMode(BUZZER_MODE_BEEP, gxs_dam.risk_level);
        }
        else
        {
            vdg_TIMER_BuzzerSetMode(BUZZER_MODE_OFF, gxs_dam.risk_level);
        }
    }

    /* Read HW-507 (DHT11) every 2 seconds */
    if ((u4t_now - gu4g_last_dht_ms) >= 2000u)
    {
        gu4g_last_dht_ms = u4t_now;
        (void)xbg_DHT11_Read(&gu1g_temp, &gu1g_humidity);
    }

    vds_APP_UpdateOutputs();

    if ((u4t_now - gu4g_last_status_ms) >= SAMPLE_PERIOD_MS)
    {
        gu4g_last_status_ms = u4t_now;
        vds_APP_SendStatus();
    }
    else
    {
        /* Periodic UART report interval not reached. */
    }

    gxb_previous_editing = gxs_fsm.editing;
    vdg_WDG_Feed();
}

static bool xbs_APP_ConfigValueAllowed(
    const DamConfig_t *pxg_cfg,
    ParameterId_t xe_param,
    uint8_t u1t_value)
{
    (void)pxg_cfg;
    (void)xe_param;
    (void)u1t_value;
    return true;
}

static void vds_APP_UpdateOutputs(void)
{
    DamState_t xe_state = gxs_fsm.state;

    vdg_Seg7_WriteDigit((uint8_t)xe_state);

    if (gxs_cfg.mode == MODE_EARTHQUAKE_LOCK)
    {
        /* Earthquake Lock active: Turn ON ALL 4 LEDs simultaneously as a hazard indicator */
        vdg_GPIO_LedSet(LED_GREEN, true);
        vdg_GPIO_LedSet(LED_YELLOW, true);
        vdg_GPIO_LedSet(LED_RED, true);
        vdg_GPIO_LedSet(LED_BLUE, true);
    }
    else
    {
        vdg_GPIO_LedSet(
            LED_GREEN,
            (xe_state == STATE_NORMAL) ||
            (xe_state == STATE_LOW_WATER) ||
            (xe_state == STATE_RECOVERY));

        vdg_GPIO_LedSet(
            LED_YELLOW,
            (xe_state == STATE_WARNING));

        vdg_GPIO_LedSet(
            LED_RED,
            (xe_state == STATE_FLOOD_CONTROL) ||
            (xe_state == STATE_EMERGENCY) ||
            (xe_state == STATE_FAULT));

        vdg_GPIO_LedSet(
            LED_BLUE,
            (gxs_cfg.mode == MODE_MANUAL));
    }

    vdg_OLED_UpdateDisplay(
        gxs_dam.estimated_power_w,
        gxs_dam.water_level,
        gu1g_temp,
        gu1g_humidity,
        (gxs_cfg.mode == MODE_EARTHQUAKE_LOCK));
}

static uint8_t u1s_APP_MapRainPercent(uint16_t u2t_raw)
{
    uint8_t u1t_value = 0u;

    if (u2t_raw <= ADC_RAIN_MIN_RAW)
    {
        u1t_value = 0u;
    }
    else
    {
        if (u2t_raw <= ADC_RAIN_CALIB_RAW)
        {
            u1t_value = (uint8_t)(
                ((uint32_t)(u2t_raw - ADC_RAIN_MIN_RAW) *
                 (uint32_t)ADC_RAIN_CALIB_PERCENT) /
                (uint32_t)(ADC_RAIN_CALIB_RAW - ADC_RAIN_MIN_RAW));
        }
        else
        {
            uint32_t u4t_scaled =
                (uint32_t)ADC_RAIN_CALIB_PERCENT +
                (((uint32_t)(u2t_raw - ADC_RAIN_CALIB_RAW) *
                  (uint32_t)(PERCENT_FULL_SCALE - ADC_RAIN_CALIB_PERCENT)) /
                 (uint32_t)(ADC_FULL_SCALE - ADC_RAIN_CALIB_RAW));

            if (u4t_scaled > PERCENT_FULL_SCALE)
            {
                u4t_scaled = PERCENT_FULL_SCALE;
            }
            else
            {
                /* Value within range. */
            }

            u1t_value = (uint8_t)u4t_scaled;
        }
    }

    if (LDR_RAIN_INVERT != 0u)
    {
        u1t_value = (uint8_t)(PERCENT_FULL_SCALE - u1t_value);
    }
    else
    {
        /* Direct mapping. */
    }

    return u1t_value;
}

static void vds_APP_SendStatus(void)
{
    static char caxt_message[UART_TX_BUFFER_SIZE];
    int s4t_len = 0;
    const char *cpxt_state = "UNKNOWN";
    const char *cpxt_mode = "UNKNOWN";
    const char *cpxt_editing = "OFF";

    switch (gxs_fsm.state)
    {
        case STATE_INIT:
            cpxt_state = "INIT";
            break;
        case STATE_NORMAL:
            cpxt_state = "NORMAL";
            break;
        case STATE_WARNING:
            cpxt_state = "WARNING";
            break;
        case STATE_FLOOD_CONTROL:
            cpxt_state = "FLOOD";
            break;
        case STATE_EMERGENCY:
            cpxt_state = "EMERGENCY";
            break;
        case STATE_RECOVERY:
            cpxt_state = "RECOVERY";
            break;
        case STATE_LOW_WATER:
            cpxt_state = "LOW WATER";
            break;
        case STATE_FAULT:
            cpxt_state = "FAULT";
            break;
        default:
            cpxt_state = "UNKNOWN";
            break;
    }

    switch (gxs_cfg.mode)
    {
        case MODE_AUTO:
            cpxt_mode = "AUTO";
            break;
        case MODE_MANUAL:
            cpxt_mode = "MANUAL";
            break;
        case MODE_EARTHQUAKE_LOCK:
            cpxt_mode = "Earthquake_lock";
            break;
        default:
            cpxt_mode = "UNKNOWN";
            break;
    }

    if (gxs_fsm.editing == true)
    {
        cpxt_editing = "ON";
    }
    else
    {
        cpxt_editing = "OFF";
    }

    const char *cpxt_param_name = "UNKNOWN";
    uint8_t u1t_current_val = u1s_APP_GetSelectedValue(&gxs_cfg, gxs_fsm.selected_parameter);

    switch (gxs_fsm.selected_parameter)
    {
        case PARAM_WARNING_LEVEL:
            cpxt_param_name = "1.WARN_LVL";
            break;
        case PARAM_FLOOD_LEVEL:
            cpxt_param_name = "2.FLOOD_LVL";
            break;
        case PARAM_CRITICAL_LEVEL:
            cpxt_param_name = "3.CRIT_LVL";
            break;
        case PARAM_LOW_WATER_LEVEL:
            cpxt_param_name = "4.LOW_LVL";
            break;
        case PARAM_STABLE_TIME:
            cpxt_param_name = "5.STABLE_SEC";
            break;
        case PARAM_MANUAL_GATE:
            cpxt_param_name = "6.MAN_GATE";
            break;
        case PARAM_MODE:
            cpxt_param_name = "7.MODE";
            break;
        default:
            cpxt_param_name = "UNKNOWN";
            break;
    }

    s4t_len = snprintf(
        caxt_message,
        UART_TX_BUFFER_SIZE,
        "\r\n[STATE]\r\n"
        "State:%s\r\n"
        "Code:%u\r\n"
        "Mode:%s\r\n"
        "\r\n"
        "[SENSOR]\r\n"
        "PotRaw:%u\r\n"
        "RainRaw:%u\r\n"
        "Water:%u%%\r\n"
        "Rain:%u%%\r\n"
        "Rate:%d%%/s\r\n"
        "Risk:%u%%\r\n"
        "\r\n"
        "[CONTROL]\r\n"
        "Gate:%u%%\r\n"
        "Power:%uW\r\n"
        "\r\n"
        "[USER EDIT]\r\n"
        "EditMode:%s\r\n"
        "Param:%s\r\n"
        "SavedVal:%u\r\n"
        "LivePotVal:%u\r\n"
        "--------------------\r\n",
        cpxt_state,
        (unsigned int)((uint8_t)gxs_fsm.state),
        cpxt_mode,
        (unsigned int)gxs_adc.config_raw,
        (unsigned int)gxs_adc.rain_raw,
        (unsigned int)gxs_dam.water_level,
        (unsigned int)gxs_dam.rain_intensity,
        (int)gxs_dam.rate_of_rise,
        (unsigned int)gxs_dam.risk_level,
        (unsigned int)u1g_Gate_GetCommand(),
        (unsigned int)gxs_dam.estimated_power_w,
        cpxt_editing,
        cpxt_param_name,
        (unsigned int)u1t_current_val,
        (unsigned int)gxs_fsm.edit_value);

    if (s4t_len > 0)
    {
        (void)xbg_UART_SendText(caxt_message);
    }
    else
    {
        /* Formatting error occurred. */
    }
}

static uint8_t u1s_APP_GetSelectedValue(
    const DamConfig_t *pxg_cfg,
    ParameterId_t xe_param)
{
    uint8_t u1t_value = 0u;

    if (pxg_cfg != NULL)
    {
        switch (xe_param)
        {
            case PARAM_WARNING_LEVEL:
                u1t_value = pxg_cfg->warning_level;
                break;
            case PARAM_FLOOD_LEVEL:
                u1t_value = pxg_cfg->flood_level;
                break;
            case PARAM_CRITICAL_LEVEL:
                u1t_value = pxg_cfg->critical_level;
                break;
            case PARAM_LOW_WATER_LEVEL:
                u1t_value = pxg_cfg->low_water_level;
                break;
            case PARAM_STABLE_TIME:
                u1t_value = pxg_cfg->stable_time_s;
                break;
            case PARAM_MANUAL_GATE:
                u1t_value = pxg_cfg->manual_gate_percent;
                break;
            case PARAM_MODE:
                if (pxg_cfg->mode == MODE_MANUAL)
                {
                    u1t_value = PERCENT_FULL_SCALE;
                }
                else
                {
                    u1t_value = 0u;
                }
                break;
            default:
                u1t_value = 0u;
                break;
        }
    }
    else
    {
        u1t_value = 0u;
    }

    return u1t_value;
}

static void vds_APP_SetSelectedValue(
    DamConfig_t *pxg_cfg,
    ParameterId_t xe_param,
    uint8_t u1t_value)
{
    if (pxg_cfg != NULL)
    {
        vdg_FSM_ApplyConfigValue(
            pxg_cfg,
            xe_param,
            u1t_value);
    }
    else
    {
        /* Invalid configuration pointer. */
    }
}

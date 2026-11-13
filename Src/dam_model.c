/*
 ******************************************************************************
 * File Name    : dam_model.c
 * Description  : Rain -> inflow -> water level simulation and risk.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "dam_model.h"

static uint8_t u1s_MapAdcToPercent(uint16_t u2t_raw);
static int16_t s2s_CalculateRate(uint8_t u1t_new_level,
                                 uint8_t u1t_old_level);
static uint8_t u1s_CalculateRisk(uint8_t u1t_water,
                                 uint8_t u1t_rain,
                                 int16_t s2t_rate);
static uint16_t u2s_CalculatePower(uint8_t u1t_water,
                                   uint8_t u1t_gate);

static int32_t s4s_water_accumulator = 3000;

void vdg_DamModel_Init(DamData_t *pxg_data)
{
    if (pxg_data != NULL)
    {
        pxg_data->water_level = 30u;
        pxg_data->rain_intensity = 0u;
        pxg_data->rate_of_rise = 0;
        pxg_data->risk_level = 0u;
        pxg_data->gate_command = 20u;
        pxg_data->estimated_power_w = 0u;
        s4s_water_accumulator = 3000;
    }
    else
    {
        /* Invalid pointer. */
    }
}

void vdg_DamModel_Update(DamData_t *pxg_data)
{
    uint8_t u1t_old_level = 0u;
    int32_t s4t_inflow_acc = 0;
    int32_t s4t_outflow_acc = 0;
    int16_t s2t_delta = 0;

    if (pxg_data != NULL)
    {
        u1t_old_level = pxg_data->water_level;

        if ((s4s_water_accumulator / 100) != (int32_t)pxg_data->water_level)
        {
            s4s_water_accumulator = (int32_t)pxg_data->water_level * 100;
        }
        else
        {
            /* Accumulator is aligned with water level. */
        }

        s4t_inflow_acc =
            (int32_t)pxg_data->rain_intensity * (int32_t)RAIN_INFLOW_GAIN;
        s4t_outflow_acc =
            (int32_t)pxg_data->gate_command * (int32_t)GATE_OUTFLOW_GAIN;

        s4s_water_accumulator += (s4t_inflow_acc - s4t_outflow_acc);

        if (s4s_water_accumulator > ((int32_t)PERCENT_FULL_SCALE * 100))
        {
            s4s_water_accumulator = (int32_t)PERCENT_FULL_SCALE * 100;
        }
        else
        {
            if (s4s_water_accumulator < 0)
            {
                s4s_water_accumulator = 0;
            }
            else
            {
                /* Accumulator within bounds. */
            }
        }

        pxg_data->water_level = (uint8_t)(s4s_water_accumulator / 100);

        s2t_delta = s2s_CalculateRate(
            pxg_data->water_level,
            u1t_old_level);

        pxg_data->rate_of_rise = s2t_delta;
        pxg_data->risk_level =
            u1s_CalculateRisk(
                pxg_data->water_level,
                pxg_data->rain_intensity,
                pxg_data->rate_of_rise);
        pxg_data->estimated_power_w =
            u2s_CalculatePower(
                pxg_data->water_level,
                pxg_data->gate_command);
    }
    else
    {
        /* Invalid pointer. */
    }
}

static uint8_t u1s_MapAdcToPercent(uint16_t u2t_raw)
{
    uint16_t u2t_limited = u2t_raw;
    uint8_t u1t_result = 0u;

    if (u2t_limited > ADC_FULL_SCALE)
    {
        u2t_limited = ADC_FULL_SCALE;
    }
    else
    {
        /* Value is already in ADC range. */
    }

    u1t_result = (uint8_t)(((uint32_t)u2t_limited *
                            PERCENT_FULL_SCALE) /
                           ADC_FULL_SCALE);

    return u1t_result;
}

static int16_t s2s_CalculateRate(uint8_t u1t_new_level,
                                 uint8_t u1t_old_level)
{
    return (int16_t)u1t_new_level - (int16_t)u1t_old_level;
}

static uint8_t u1s_CalculateRisk(uint8_t u1t_water,
                                 uint8_t u1t_rain,
                                 int16_t s2t_rate)
{
    uint16_t u2t_rate_risk = 0u;
    uint16_t u2t_risk = 0u;

    if (s2t_rate > 0)
    {
        if ((uint16_t)s2t_rate >= RATE_RISK_FULL_SCALE)
        {
            u2t_rate_risk = PERCENT_FULL_SCALE;
        }
        else
        {
            u2t_rate_risk =
                ((uint16_t)s2t_rate * PERCENT_FULL_SCALE) /
                RATE_RISK_FULL_SCALE;
        }
    }
    else
    {
        u2t_rate_risk = 0u;
    }

    u2t_risk = ((uint16_t)u1t_water * 50u) / 100u;
    u2t_risk += ((uint16_t)u1t_rain * 20u) / 100u;
    u2t_risk += (u2t_rate_risk * 30u) / 100u;

    if (u2t_risk > PERCENT_FULL_SCALE)
    {
        u2t_risk = PERCENT_FULL_SCALE;
    }
    else
    {
        /* Risk is within range. */
    }

    return (uint8_t)u2t_risk;
}

static uint16_t u2s_CalculatePower(uint8_t u1t_water,
                                   uint8_t u1t_gate)
{
    uint32_t u4t_product =
        (uint32_t)u1t_water *
        (uint32_t)u1t_gate *
        MAX_POWER_W;
    uint32_t u4t_result =
        u4t_product /
        (PERCENT_FULL_SCALE * PERCENT_FULL_SCALE);

    if (u4t_result > 65535u)
    {
        u4t_result = 65535u;
    }
    else
    {
        /* Value is within uint16_t range. */
    }

    return (uint16_t)u4t_result;
}

uint8_t u1g_Dam_MapAdcToPercent(uint16_t u2t_raw)
{
    return u1s_MapAdcToPercent(u2t_raw);
}

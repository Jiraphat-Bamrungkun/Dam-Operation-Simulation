/*
 ******************************************************************************
 * File Name    : dam_model.h
 * Description  : Dam water simulation and risk model.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#ifndef DAM_MODEL_H
#define DAM_MODEL_H

#include <stdint.h>

typedef struct
{
    uint8_t water_level;
    uint8_t rain_intensity;
    int16_t rate_of_rise;
    uint8_t risk_level;
    uint8_t gate_command;
    uint16_t estimated_power_w;
} DamData_t;

void vdg_DamModel_Init(DamData_t *pxg_data);
void vdg_DamModel_Update(DamData_t *pxg_data);
uint8_t u1g_Dam_MapAdcToPercent(uint16_t u2t_raw);

#endif /* DAM_MODEL_H */

/*
 ******************************************************************************
 * File Name    : adc_driver.h
 * Description  : ADC1 scan + DMA driver.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint16_t config_raw;
    uint16_t rain_raw;
} AdcData_t;

void vdg_ADC_Init(void);
bool xbg_ADC_GetLatest(AdcData_t *pxg_data);

#endif /* ADC_DRIVER_H */

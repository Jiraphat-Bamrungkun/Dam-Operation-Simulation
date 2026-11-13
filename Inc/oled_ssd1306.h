/*
 ******************************************************************************
 * File Name    : oled_ssd1306.h
 * Description  : Lightweight SSD1306 OLED driver via I2C (PB8=SCL, PB9=SDA).
 * Date         : 2026-10-08
 ******************************************************************************
 */

#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Public API */
void vdg_OLED_Init(void);
void vdg_OLED_UpdatePower(uint16_t u2t_power_w);
void vdg_OLED_UpdateDisplay(uint16_t u2t_power_w, uint8_t u1t_water_percent, uint8_t u1t_temp, uint8_t u1t_humidity, bool xb_eq_lock);

#endif /* OLED_SSD1306_H */

/*
 ******************************************************************************
 * File Name    : gpio_driver.h
 * Description  : GPIO driver for LEDs, buzzer and digital outputs.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    LED_GREEN = 0,
    LED_YELLOW,
    LED_RED,
    LED_BLUE
} LedId_t;

void vdg_GPIO_Init(void);
void vdg_GPIO_LedSet(LedId_t xe_led, bool xb_on);
void vdg_GPIO_BuzzerSet(bool xb_on);
void vdg_GPIO_BuzzerToggle(void);

#endif /* GPIO_DRIVER_H */

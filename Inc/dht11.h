/*
 ******************************************************************************
 * File Name    : dht11.h
 * Description  : HW-507 / DHT11 Temperature and Humidity Sensor Driver.
 * Pin          : PB2 (GPIOB, Pin 2)
 * Date         : 2026-10-08
 ******************************************************************************
 */

#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Initialize DHT11 GPIO pin */
void vdg_DHT11_Init(void);

/* Read temperature (°C) and humidity (%RH) from HW-507 (DHT11).
 * Returns true if read was successful and checksum matched. */
bool xbg_DHT11_Read(uint8_t *pu1t_temp, uint8_t *pu1t_humidity);

#endif /* DHT11_H */

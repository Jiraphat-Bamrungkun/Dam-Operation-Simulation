/*
 ******************************************************************************
 * File Name    : dht11.c
 * Description  : HW-507 / DHT11 Temperature and Humidity Sensor Driver.
 * Pin          : PB2 (GPIOB Pin 2)
 * Date         : 2026-10-08
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "system_time.h"
#include "dht11.h"

#ifndef DHT11_PORT
#define DHT11_PORT GPIOB
#endif

#ifndef DHT11_PIN
#define DHT11_PIN (2u)
#endif

/* Microsecond delay using ARM Cortex-M4 DWT Cycle Counter (16 MHz clock) */
static void vds_DHT11_DelayUs(uint32_t u4t_us)
{
    uint32_t u4t_start = DWT->CYCCNT;
    uint32_t u4t_ticks = u4t_us * 16u; /* 16 cycles per us at 16 MHz */

    while ((DWT->CYCCNT - u4t_start) < u4t_ticks)
    {
        __NOP();
    }
}

static void vds_DHT11_SetOutput(void)
{
    /* Configure PB2 as Output Push-Pull */
    DHT11_PORT->MODER &= ~(0x3u << (DHT11_PIN * 2u));
    DHT11_PORT->MODER |= (0x1u << (DHT11_PIN * 2u));
    DHT11_PORT->OTYPER &= ~(1u << DHT11_PIN);
    DHT11_PORT->PUPDR &= ~(0x3u << (DHT11_PIN * 2u));
}

static void vds_DHT11_SetInput(void)
{
    /* Configure PB2 as Input with Internal Pull-up */
    DHT11_PORT->MODER &= ~(0x3u << (DHT11_PIN * 2u));
    DHT11_PORT->PUPDR &= ~(0x3u << (DHT11_PIN * 2u));
    DHT11_PORT->PUPDR |= (0x1u << (DHT11_PIN * 2u));
}

void vdg_DHT11_Init(void)
{
    /* Enable GPIOB Clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* Enable DWT Cycle Counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    vds_DHT11_SetOutput();
    DHT11_PORT->ODR |= (1u << DHT11_PIN); /* Default HIGH */
}

bool xbg_DHT11_Read(uint8_t *pu1t_temp, uint8_t *pu1t_humidity)
{
    uint8_t u1t_data[5] = {0};
    uint32_t u4t_timeout = 0u;

    if ((pu1t_temp == NULL) || (pu1t_humidity == NULL))
    {
        return false;
    }

    /* Step 1: Send Start Signal (Pull LOW for 20 ms) */
    vds_DHT11_SetOutput();
    DHT11_PORT->ODR &= ~(1u << DHT11_PIN);
    
    uint32_t u4t_start_ms = u4g_Time_GetMs();
    while ((u4g_Time_GetMs() - u4t_start_ms) < 20u)
    {
        /* Wait 20 ms */
    }

    /* Pull HIGH for 30 us */
    DHT11_PORT->ODR |= (1u << DHT11_PIN);
    vds_DHT11_DelayUs(30);

    /* Step 2: Switch to Input Mode */
    vds_DHT11_SetInput();

    /* Wait for DHT11 to pull pin LOW (response start, ~80 us) */
    u4t_timeout = 0u;
    while ((DHT11_PORT->IDR & (1u << DHT11_PIN)) != 0u)
    {
        vds_DHT11_DelayUs(1);
        u4t_timeout++;
        if (u4t_timeout > 150u) { return false; }
    }

    /* Wait for DHT11 to pull pin HIGH (~80 us) */
    u4t_timeout = 0u;
    while ((DHT11_PORT->IDR & (1u << DHT11_PIN)) == 0u)
    {
        vds_DHT11_DelayUs(1);
        u4t_timeout++;
        if (u4t_timeout > 150u) { return false; }
    }

    /* Wait for DHT11 to pull pin LOW (start of data frame) */
    u4t_timeout = 0u;
    while ((DHT11_PORT->IDR & (1u << DHT11_PIN)) != 0u)
    {
        vds_DHT11_DelayUs(1);
        u4t_timeout++;
        if (u4t_timeout > 150u) { return false; }
    }

    /* Step 3: Read 40 bits (5 bytes) */
    for (uint8_t i = 0u; i < 40u; i++)
    {
        /* Wait for pin to go HIGH (start of data bit pulse) */
        u4t_timeout = 0u;
        while ((DHT11_PORT->IDR & (1u << DHT11_PIN)) == 0u)
        {
            vds_DHT11_DelayUs(1);
            u4t_timeout++;
            if (u4t_timeout > 150u) { return false; }
        }

        /* Measure HIGH pulse duration using DWT */
        uint32_t u4t_t_start = DWT->CYCCNT;

        /* Wait for pin to go LOW (end of data bit pulse) */
        u4t_timeout = 0u;
        while ((DHT11_PORT->IDR & (1u << DHT11_PIN)) != 0u)
        {
            vds_DHT11_DelayUs(1);
            u4t_timeout++;
            if (u4t_timeout > 150u) { return false; }
        }

        uint32_t u4t_high_us = (DWT->CYCCNT - u4t_t_start) / 16u;

        uint8_t u1t_byte_idx = i / 8u;
        u1t_data[u1t_byte_idx] <<= 1;

        if (u4t_high_us > 40u)
        {
            /* HIGH pulse > 40 us -> Bit is 1 */
            u1t_data[u1t_byte_idx] |= 1u;
        }
    }

    /* Step 4: Verify Checksum */
    uint8_t u1t_checksum = u1t_data[0] + u1t_data[1] + u1t_data[2] + u1t_data[3];
    if ((u1t_checksum == u1t_data[4]) && (u1t_data[0] != 0u))
    {
        *pu1t_humidity = u1t_data[0];
        *pu1t_temp     = u1t_data[2];
        return true;
    }

    return false;
}

/*
 ******************************************************************************
 * File Name    : gpio_driver.c
 * Description  : GPIO driver implementation.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "gpio_driver.h"

static void vds_GPIO_OutputInit(GPIO_TypeDef *pxg_port, uint8_t u1t_pin);
static uint32_t u4s_GPIO_PinMask(uint8_t u1t_pin);

void vdg_GPIO_Init(void)
{
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN |
                     RCC_AHB1ENR_GPIOBEN |
                     RCC_AHB1ENR_GPIOCEN);

    vds_GPIO_OutputInit(LED_GREEN_PORT, LED_GREEN_PIN);
    vds_GPIO_OutputInit(LED_YELLOW_PORT, LED_YELLOW_PIN);
    vds_GPIO_OutputInit(LED_RED_PORT, LED_RED_PIN);
    vds_GPIO_OutputInit(LED_BLUE_PORT, LED_BLUE_PIN);
    vds_GPIO_OutputInit(GPIOC, BUZZER_PIN);

    LED_GREEN_PORT->ODR &= ~u4s_GPIO_PinMask(LED_GREEN_PIN);
    LED_YELLOW_PORT->ODR &= ~u4s_GPIO_PinMask(LED_YELLOW_PIN);
    LED_RED_PORT->ODR &= ~u4s_GPIO_PinMask(LED_RED_PIN);
    LED_BLUE_PORT->ODR &= ~u4s_GPIO_PinMask(LED_BLUE_PIN);

#if (BUZZER_ACTIVE_LOW != 0u)
    GPIOC->ODR |= u4s_GPIO_PinMask(BUZZER_PIN);  /* High = Silent for Active-Low */
#else
    GPIOC->ODR &= ~u4s_GPIO_PinMask(BUZZER_PIN); /* Low = Silent for Active-High */
#endif
}

void vdg_GPIO_LedSet(LedId_t xe_led, bool xb_on)
{
    GPIO_TypeDef *pxg_port = NULL;
    uint8_t u1t_pin = 0u;

    switch (xe_led)
    {
        case LED_GREEN:
            pxg_port = LED_GREEN_PORT;
            u1t_pin = LED_GREEN_PIN;
            break;
        case LED_YELLOW:
            pxg_port = LED_YELLOW_PORT;
            u1t_pin = LED_YELLOW_PIN;
            break;
        case LED_RED:
            pxg_port = LED_RED_PORT;
            u1t_pin = LED_RED_PIN;
            break;
        case LED_BLUE:
            pxg_port = LED_BLUE_PORT;
            u1t_pin = LED_BLUE_PIN;
            break;
        default:
            pxg_port = NULL;
            break;
    }

    if (pxg_port != NULL)
    {
        if (xb_on == true)
        {
            pxg_port->ODR |= u4s_GPIO_PinMask(u1t_pin);
        }
        else
        {
            pxg_port->ODR &= ~u4s_GPIO_PinMask(u1t_pin);
        }
    }
    else
    {
        /* Invalid LED identifier. */
    }
}

void vdg_GPIO_BuzzerSet(bool xb_on)
{
#if (BUZZER_ACTIVE_LOW != 0u)
    if (xb_on == true)
    {
        GPIOC->ODR &= ~u4s_GPIO_PinMask(BUZZER_PIN);
    }
    else
    {
        GPIOC->ODR |= u4s_GPIO_PinMask(BUZZER_PIN);
    }
#else
    if (xb_on == true)
    {
        GPIOC->ODR |= u4s_GPIO_PinMask(BUZZER_PIN);
    }
    else
    {
        GPIOC->ODR &= ~u4s_GPIO_PinMask(BUZZER_PIN);
    }
#endif
}

void vdg_GPIO_BuzzerToggle(void)
{
    /* Used for a passive buzzer: toggling the pin makes the square wave. */
    GPIOC->ODR ^= u4s_GPIO_PinMask(BUZZER_PIN);
}

static void vds_GPIO_OutputInit(GPIO_TypeDef *pxg_port, uint8_t u1t_pin)
{
    uint32_t u4t_shift = (uint32_t)u1t_pin * 2u;

    pxg_port->MODER &= ~(0x3u << u4t_shift);
    pxg_port->MODER |= (0x1u << u4t_shift);
    pxg_port->OTYPER &= ~u4s_GPIO_PinMask(u1t_pin);
    pxg_port->OSPEEDR |= (0x1u << u4t_shift);
    pxg_port->PUPDR &= ~(0x3u << u4t_shift);
}

static uint32_t u4s_GPIO_PinMask(uint8_t u1t_pin)
{
    return ((uint32_t)1u << u1t_pin);
}

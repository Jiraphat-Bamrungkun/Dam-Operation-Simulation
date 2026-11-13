/*
 ******************************************************************************
 * File Name    : sevenseg.c
 * Description  : 7-Segment BCD state display.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include <stdint.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "sevenseg.h"

#define SEG7_MAX_DIGIT    (9u)

static void vds_Seg7_WriteBit(GPIO_TypeDef *pxg_port,
                              uint8_t u1t_pin,
                              uint8_t u1t_level);

void vdg_Seg7_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    GPIOC->MODER &= ~(0x3u << (SEG7_D0_PIN * 2u));
    GPIOC->MODER |= (0x1u << (SEG7_D0_PIN * 2u));

    GPIOA->MODER &= ~(0x3u << (SEG7_D1_PIN * 2u));
    GPIOA->MODER |= (0x1u << (SEG7_D1_PIN * 2u));

    GPIOB->MODER &= ~(0x3u << (SEG7_D2_PIN * 2u));
    GPIOB->MODER |= (0x1u << (SEG7_D2_PIN * 2u));

    GPIOA->MODER &= ~(0x3u << (SEG7_D3_PIN * 2u));
    GPIOA->MODER |= (0x1u << (SEG7_D3_PIN * 2u));
}

void vdg_Seg7_WriteDigit(uint8_t u1t_digit)
{
    if (u1t_digit <= SEG7_MAX_DIGIT)
    {
        vds_Seg7_WriteBit(GPIOC,
                          SEG7_D0_PIN,
                          (uint8_t)(u1t_digit & 0x01u));
        vds_Seg7_WriteBit(GPIOA,
                          SEG7_D1_PIN,
                          (uint8_t)((u1t_digit >> 1u) & 0x01u));
        vds_Seg7_WriteBit(GPIOB,
                          SEG7_D2_PIN,
                          (uint8_t)((u1t_digit >> 2u) & 0x01u));
        vds_Seg7_WriteBit(GPIOA,
                          SEG7_D3_PIN,
                          (uint8_t)((u1t_digit >> 3u) & 0x01u));
    }
    else
    {
        /* No action. */
    }
}

static void vds_Seg7_WriteBit(GPIO_TypeDef *pxg_port,
                              uint8_t u1t_pin,
                              uint8_t u1t_level)
{
    if (u1t_level != 0u)
    {
        pxg_port->ODR |= ((uint32_t)1u << u1t_pin);
    }
    else
    {
        pxg_port->ODR &= ~((uint32_t)1u << u1t_pin);
    }
}

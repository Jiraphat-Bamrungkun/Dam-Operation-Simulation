/*
 ******************************************************************************
 * File Name    : exti_driver.c
 * Description  : GPIO pull-up + EXTI edge events.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "system_time.h"
#include "exti_driver.h"

static volatile uint8_t gu1g_press_flags = 0u;
static volatile uint8_t gu1g_release_flags = 0u;
static volatile uint32_t gu4g_last_press_ms[6] = {0u, 0u, 0u, 0u, 0u, 0u};
static volatile uint32_t gu4g_last_release_ms[6] = {0u, 0u, 0u, 0u, 0u, 0u};

static uint8_t u1s_ButtonMask(ButtonId_t xe_button);
static void vds_EXTI_ConfigureLine(uint8_t u1t_line,
                                   GPIO_TypeDef *pxg_port,
                                   uint8_t u1t_pin);
static void vds_EXTI_HandlePinEvent(ButtonId_t xe_button,
                                    GPIO_TypeDef *pxg_port,
                                    uint8_t u1t_pin,
                                    uint32_t u4t_now);

void vdg_EXTI_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    vds_EXTI_ConfigureLine(BTN_SELECT_EXTI_LINE, GPIOA, BTN_SELECT_PIN);
    vds_EXTI_ConfigureLine(BTN_CONFIRM_EXTI_LINE, GPIOB, BTN_CONFIRM_PIN);
    vds_EXTI_ConfigureLine(BTN_EMERGENCY_EXTI_LINE,
                           GPIOB,
                           BTN_EMERGENCY_PIN);
    vds_EXTI_ConfigureLine(BTN_CANCEL_EXTI_LINE, GPIOB, BTN_CANCEL_PIN);
    vds_EXTI_ConfigureLine(13u, GPIOC, 13u); /* PC13 = Nucleo Blue Button B1 */
    vds_EXTI_ConfigureLine(SHOCK_SENSOR_EXTI_LINE, SHOCK_SENSOR_PORT, SHOCK_SENSOR_PIN); /* PC0 = HW-513 Shock Sensor DO */

    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}

bool xbg_EXTI_GetPress(ButtonId_t xe_button)
{
    uint8_t u1t_mask = u1s_ButtonMask(xe_button);
    bool xb_result = false;

    if ((gu1g_press_flags & u1t_mask) != 0u)
    {
        gu1g_press_flags &= (uint8_t)(~u1t_mask);
        xb_result = true;
    }
    else
    {
        /* No pending press event. */
    }

    return xb_result;
}

bool xbg_EXTI_GetRelease(ButtonId_t xe_button)
{
    uint8_t u1t_mask = u1s_ButtonMask(xe_button);
    bool xb_result = false;

    if ((gu1g_release_flags & u1t_mask) != 0u)
    {
        gu1g_release_flags &= (uint8_t)(~u1t_mask);
        xb_result = true;
    }
    else
    {
        /* No pending release event. */
    }

    return xb_result;
}

void EXTI0_IRQHandler(void)
{
    uint32_t u4t_now = u4g_Time_GetMs();

    if ((EXTI->PR & EXTI_PR_PR0) != 0u)
    {
        EXTI->PR = EXTI_PR_PR0;
        vds_EXTI_HandlePinEvent(BUTTON_SHOCK_SENSOR, SHOCK_SENSOR_PORT, SHOCK_SENSOR_PIN, u4t_now);
    }
    else
    {
        /* No EXTI0 event. */
    }
}

void EXTI15_10_IRQHandler(void)
{
    uint32_t u4t_now = u4g_Time_GetMs();

    if ((EXTI->PR & EXTI_PR_PR10) != 0u)
    {
        EXTI->PR = EXTI_PR_PR10;
        vds_EXTI_HandlePinEvent(BUTTON_SELECT, GPIOA, BTN_SELECT_PIN, u4t_now);
    }
    else
    {
        /* No EXTI10 event. */
    }

    if ((EXTI->PR & EXTI_PR_PR13) != 0u)
    {
        EXTI->PR = EXTI_PR_PR13;
        vds_EXTI_HandlePinEvent(BUTTON_LOCK, GPIOC, 13u, u4t_now);
    }
    else
    {
        /* No EXTI13 event. */
    }
}

void EXTI4_IRQHandler(void)
{
    uint32_t u4t_now = u4g_Time_GetMs();

    if ((EXTI->PR & EXTI_PR_PR4) != 0u)
    {
        EXTI->PR = EXTI_PR_PR4;
        vds_EXTI_HandlePinEvent(BUTTON_CANCEL, GPIOB, BTN_CANCEL_PIN, u4t_now);
    }
    else
    {
        /* No EXTI4 event. */
    }
}

void EXTI3_IRQHandler(void)
{
    uint32_t u4t_now = u4g_Time_GetMs();

    if ((EXTI->PR & EXTI_PR_PR3) != 0u)
    {
        EXTI->PR = EXTI_PR_PR3;
        vds_EXTI_HandlePinEvent(BUTTON_CONFIRM, GPIOB, BTN_CONFIRM_PIN, u4t_now);
    }
    else
    {
        /* No EXTI3 event. */
    }
}

void EXTI9_5_IRQHandler(void)
{
    uint32_t u4t_now = u4g_Time_GetMs();

    if ((EXTI->PR & EXTI_PR_PR5) != 0u)
    {
        EXTI->PR = EXTI_PR_PR5;
        vds_EXTI_HandlePinEvent(BUTTON_EMERGENCY, GPIOB, BTN_EMERGENCY_PIN, u4t_now);
    }
    else
    {
        /* No EXTI5 event. */
    }
}

static uint8_t u1s_ButtonMask(ButtonId_t xe_button)
{
    uint8_t u1t_mask = 0u;

    switch (xe_button)
    {
        case BUTTON_SELECT:
            u1t_mask = 0x01u;
            break;
        case BUTTON_CONFIRM:
            u1t_mask = 0x02u;
            break;
        case BUTTON_EMERGENCY:
            u1t_mask = 0x04u;
            break;
        case BUTTON_CANCEL:
            u1t_mask = 0x08u;
            break;
        case BUTTON_LOCK:
            u1t_mask = 0x10u;
            break;
        case BUTTON_SHOCK_SENSOR:
            u1t_mask = 0x20u;
            break;
        default:
            u1t_mask = 0u;
            break;
    }

    return u1t_mask;
}

static void vds_EXTI_ConfigureLine(uint8_t u1t_line,
                                   GPIO_TypeDef *pxg_port,
                                   uint8_t u1t_pin)
{
    uint32_t u4t_shift = (uint32_t)u1t_pin * 2u;
    uint32_t u4t_af_index = (uint32_t)u1t_pin >> 2u;
    uint32_t u4t_af_shift = ((uint32_t)u1t_pin & 0x3u) * 4u;

    pxg_port->MODER &= ~(0x3u << u4t_shift);
    pxg_port->PUPDR &= ~(0x3u << u4t_shift);
    pxg_port->PUPDR |= (0x1u << u4t_shift);

    SYSCFG->EXTICR[u4t_af_index] &= ~(0xFu << u4t_af_shift);

    if (pxg_port == GPIOB)
    {
        SYSCFG->EXTICR[u4t_af_index] |= (0x1u << u4t_af_shift);
    }
    else if (pxg_port == GPIOC)
    {
        SYSCFG->EXTICR[u4t_af_index] |= (0x2u << u4t_af_shift);
    }
    else
    {
        /* GPIOA maps to EXTI code 0. */
    }

    EXTI->PR = (1u << u1t_line);
    EXTI->IMR |= (1u << u1t_line);
    EXTI->RTSR |= (1u << u1t_line);
    EXTI->FTSR |= (1u << u1t_line);
}

static void vds_EXTI_HandlePinEvent(ButtonId_t xe_button,
                                    GPIO_TypeDef *pxg_port,
                                    uint8_t u1t_pin,
                                    uint32_t u4t_now)
{
    uint8_t u1t_mask = u1s_ButtonMask(xe_button);
    bool xb_pressed = ((pxg_port->IDR & (1u << u1t_pin)) == 0u);

    if (xb_pressed == true)
    {
        if ((u4t_now - gu4g_last_press_ms[xe_button]) >= BUTTON_DEBOUNCE_MS)
        {
            gu1g_press_flags |= u1t_mask;
            gu4g_last_press_ms[xe_button] = u4t_now;
        }
        else
        {
            /* Press debounced. */
        }
    }
    else
    {
        if ((u4t_now - gu4g_last_release_ms[xe_button]) >= BUTTON_DEBOUNCE_MS)
        {
            gu1g_release_flags |= u1t_mask;
            gu4g_last_release_ms[xe_button] = u4t_now;
        }
        else
        {
            /* Release debounced. */
        }
    }
}

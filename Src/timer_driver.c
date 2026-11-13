/*
 ******************************************************************************
 * File Name    : timer_driver.c
 * Description  : TIM3 Servo PWM and TIM4 1 ms timing event.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "gpio_driver.h"
#include "timer_driver.h"

static volatile uint32_t gu4g_timer_ms = 0u;
static volatile bool gxb_second_event = false;
static volatile bool gxb_half_second_event = false;
/* Buzzer pattern set by the main loop, played by TIM4 ISR every 1 ms.
 * The pattern is timed in the ISR so a slow main loop (e.g. OLED redraw)
 * does not distort the sound. */
static volatile BuzzerMode_t gxe_buzzer_mode = BUZZER_MODE_OFF;
static volatile uint16_t gu2g_buzzer_on_ms = 0u;
static volatile uint16_t gu2g_buzzer_off_ms = 0u;
static volatile bool gxb_buzzer_restart = false;
static uint16_t gu2s_buzzer_phase_ms = 0u;   /* ISR only. */
static bool gxb_buzzer_output = false;       /* ISR only. */

#if (BUZZER_PASSIVE != 0u)
/* Passive buzzer: TIM2 interrupts at 2 x tone frequency and toggles PC4. */
#define TIMER2_TICK_HZ              (1000000u)   /* 16 MHz / 16 */
static volatile bool gxb_tone_enable = false;
static uint32_t gu4s_tone_hz = 0u;
static uint16_t gu2s_siren_ms = 0u;
static bool gxb_siren_high = false;

static void vds_TIMER2_Init(void);
static void vds_TIMER_SetToneHz(uint32_t u4t_hz);
#endif

static void vds_TIMER3_Init(void);
static void vds_TIMER4_Init(void);
static void vds_TIMER_BuzzerTick(void);

void vdg_TIMER_Init(void)
{
    gu4g_timer_ms = 0u;
    gxb_second_event = false;
    gxb_half_second_event = false;
    gxe_buzzer_mode = BUZZER_MODE_OFF;
    gu2g_buzzer_on_ms = 0u;
    gu2g_buzzer_off_ms = 0u;
    gxb_buzzer_restart = false;
    gu2s_buzzer_phase_ms = 0u;
    gxb_buzzer_output = false;

    vds_TIMER3_Init();
    vds_TIMER4_Init();

#if (BUZZER_PASSIVE != 0u)
    gxb_tone_enable = false;
    gu4s_tone_hz = 0u;
    gu2s_siren_ms = 0u;
    gxb_siren_high = false;
    vds_TIMER2_Init();
    NVIC_EnableIRQ(TIM2_IRQn);
#endif

    NVIC_EnableIRQ(TIM4_IRQn);
}

uint32_t u4g_TIMER_GetTickMs(void)
{
    return gu4g_timer_ms;
}

void vdg_TIMER_ServoSetPercent(uint8_t u1t_percent)
{
    uint32_t u4t_compare = SERVO_MIN_US;

    if (u1t_percent <= PERCENT_FULL_SCALE)
    {
        u4t_compare =
            (uint32_t)SERVO_MIN_US +
            (((uint32_t)(SERVO_MAX_US - SERVO_MIN_US) *
              (uint32_t)u1t_percent) /
             PERCENT_FULL_SCALE);
        TIM3->CCR3 = u4t_compare;
    }
    else
    {
        TIM3->CCR3 = SERVO_MIN_US;
    }
}

void vdg_TIMER_BuzzerSetMode(BuzzerMode_t xe_mode, uint8_t u1t_risk_level)
{
    uint16_t u2t_on_ms = 0u;
    uint16_t u2t_off_ms = 0u;

    switch (xe_mode)
    {
        case BUZZER_MODE_EMERGENCY:
            /* เสียงเตือนฉุกเฉิน: ดังยาว ไม่ขึ้นกับ risk */
            u2t_on_ms = BUZZER_EMERGENCY_ON_MS;
            u2t_off_ms = BUZZER_EMERGENCY_OFF_MS;
            break;

        case BUZZER_MODE_BEEP:
            /* ปี๊บเป็นจังหวะ ยิ่ง risk สูงยิ่งถี่ (เหมือนโค้ดเดิม) */
            if (u1t_risk_level >= 80u)
            {
                u2t_on_ms = 150u;
            }
            else
            {
                if (u1t_risk_level >= 50u)
                {
                    u2t_on_ms = 300u;
                }
                else
                {
                    if (u1t_risk_level >= 20u)
                    {
                        u2t_on_ms = 600u;
                    }
                    else
                    {
                        u2t_on_ms = 1000u;
                    }
                }
            }
            u2t_off_ms = u2t_on_ms;
            break;

        case BUZZER_MODE_OFF:
        default:
            xe_mode = BUZZER_MODE_OFF;
            break;
    }

    gu2g_buzzer_on_ms = u2t_on_ms;
    gu2g_buzzer_off_ms = u2t_off_ms;

    if (xe_mode != gxe_buzzer_mode)
    {
        /* Mode changed: start the new pattern with the buzzer ON at once. */
        gxe_buzzer_mode = xe_mode;
        gxb_buzzer_restart = true;
    }
    else
    {
        /* Same mode: only the timing is updated. */
    }
}

static void vds_TIMER_BuzzerTick(void)
{
    if (gxe_buzzer_mode == BUZZER_MODE_OFF)
    {
        gxb_buzzer_output = false;
        gu2s_buzzer_phase_ms = 0u;
        gxb_buzzer_restart = false;
    }
    else
    {
        if (gxb_buzzer_restart == true)
        {
            gxb_buzzer_restart = false;
            gxb_buzzer_output = true;
            gu2s_buzzer_phase_ms = 0u;
        }
        else
        {
            gu2s_buzzer_phase_ms++;

            if (gxb_buzzer_output == true)
            {
                /* OFF time 0 = continuous tone, never switch off. */
                if ((gu2g_buzzer_off_ms != 0u) &&
                    (gu2s_buzzer_phase_ms >= gu2g_buzzer_on_ms))
                {
                    gxb_buzzer_output = false;
                    gu2s_buzzer_phase_ms = 0u;
                }
                else
                {
                    /* Keep sounding. */
                }
            }
            else
            {
                if (gu2s_buzzer_phase_ms >= gu2g_buzzer_off_ms)
                {
                    gxb_buzzer_output = true;
                    gu2s_buzzer_phase_ms = 0u;
                }
                else
                {
                    /* Keep silent. */
                }
            }
        }
    }

#if (BUZZER_PASSIVE != 0u)
    /* Choose the tone: EMERGENCY = siren (low/high note), others = one tone. */
    if (gxe_buzzer_mode == BUZZER_MODE_EMERGENCY)
    {
        gu2s_siren_ms++;
        if (gu2s_siren_ms >= BUZZER_SIREN_STEP_MS)
        {
            gu2s_siren_ms = 0u;
            gxb_siren_high = (bool)(!gxb_siren_high);
        }
        else
        {
            /* Stay on the current siren note. */
        }

        if (gxb_siren_high == true)
        {
            vds_TIMER_SetToneHz(BUZZER_SIREN_HIGH_HZ);
        }
        else
        {
            vds_TIMER_SetToneHz(BUZZER_SIREN_LOW_HZ);
        }
    }
    else
    {
        gu2s_siren_ms = 0u;
        gxb_siren_high = false;
        vds_TIMER_SetToneHz(BUZZER_TONE_HZ);
    }

    /* TIM2 ISR makes the square wave while this is true. */
    gxb_tone_enable = gxb_buzzer_output;
#else
    vdg_GPIO_BuzzerSet(gxb_buzzer_output);
#endif
}

#if (BUZZER_PASSIVE != 0u)
static void vds_TIMER_SetToneHz(uint32_t u4t_hz)
{
    if ((u4t_hz != 0u) && (u4t_hz != gu4s_tone_hz))
    {
        gu4s_tone_hz = u4t_hz;
        /* Toggle twice per period -> interrupt at 2 x frequency. */
        TIM2->ARR = (TIMER2_TICK_HZ / (2u * u4t_hz)) - 1u;
    }
    else
    {
        /* Same tone, nothing to change. */
    }
}

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0u)
    {
        TIM2->SR &= ~TIM_SR_UIF;

        if (gxb_tone_enable == true)
        {
            vdg_GPIO_BuzzerToggle();
        }
        else
        {
            vdg_GPIO_BuzzerSet(false);
        }
    }
    else
    {
        /* No update event. */
    }
}

static void vds_TIMER2_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1 = 0u;
    TIM2->PSC = 16u - 1u;                 /* 16 MHz / 16 = 1 MHz */
    TIM2->ARR = (TIMER2_TICK_HZ / (2u * BUZZER_TONE_HZ)) - 1u;
    gu4s_tone_hz = BUZZER_TONE_HZ;
    TIM2->CR1 |= TIM_CR1_ARPE;            /* New frequency applies cleanly. */
    TIM2->EGR |= TIM_EGR_UG;
    TIM2->SR &= ~TIM_SR_UIF;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1 |= TIM_CR1_CEN;
}
#endif

bool xbg_TIMER_TakeSecondEvent(void)
{
    bool xb_result = gxb_second_event;

    gxb_second_event = false;
    return xb_result;
}

bool xbg_TIMER_TakeHalfSecondEvent(void)
{
    bool xb_result = gxb_half_second_event;

    gxb_half_second_event = false;
    return xb_result;
}

void TIM4_IRQHandler(void)
{
    if ((TIM4->SR & TIM_SR_UIF) != 0u)
    {
        TIM4->SR &= ~TIM_SR_UIF;
        gu4g_timer_ms++;

        vds_TIMER_BuzzerTick();

        if ((gu4g_timer_ms % SAMPLE_PERIOD_MS) == 0u)
        {
            gxb_second_event = true;
        }
        else
        {
            /* Not a full-second boundary. */
        }

        if ((gu4g_timer_ms % GATE_RATE_PERIOD_MS) == 0u)
        {
            gxb_half_second_event = true;
        }
        else
        {
            /* Not a gate-rate boundary. */
        }
    }
    else
    {
        /* No update event. */
    }
}

static void vds_TIMER3_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    GPIOB->MODER &= ~(0x3u << (SERVO_PORT_PIN * 2u));
    GPIOB->MODER |= (0x2u << (SERVO_PORT_PIN * 2u));
    GPIOB->AFR[0] &= ~(0xFu << (SERVO_PORT_PIN * 4u));
    GPIOB->AFR[0] |= (0x2u << (SERVO_PORT_PIN * 4u));

    TIM3->PSC = TIMER3_PRESCALER;
    TIM3->ARR = TIMER3_PERIOD_US - 1u;
    TIM3->CCMR2 &= ~TIM_CCMR2_OC3M;
    TIM3->CCMR2 |= (0x6u << TIM_CCMR2_OC3M_Pos);
    TIM3->CCMR2 |= TIM_CCMR2_OC3PE;
    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CCR3 = SERVO_MIN_US;
    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR |= TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

static void vds_TIMER4_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    TIM4->PSC = TIMER4_PRESCALER;
    TIM4->ARR = TIMER4_PERIOD;
    TIM4->DIER |= TIM_DIER_UIE;
    TIM4->EGR |= TIM_EGR_UG;
    TIM4->CR1 |= TIM_CR1_CEN;
}

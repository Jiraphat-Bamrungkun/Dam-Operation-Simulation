/*
 ******************************************************************************
 * File Name    : board_config.h
 * Description  : DAM-GUARD board pin and system configuration.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>

#ifndef STM32F411xE
#define STM32F411xE
#endif
#include "stm32f4xx.h"

/* Analog input channels. */
#define ADC_CONFIG_CHANNEL          (4u)
#define ADC_RAIN_CHANNEL            (1u)
#define LDR_RAIN_INVERT             (0u)
#define ADC_RAIN_MIN_RAW            (600u)
#define ADC_RAIN_CALIB_RAW          (1000u)
#define ADC_RAIN_CALIB_PERCENT      (10u)

/* PA4 = ADC1_IN4 = potentiometer (A2).
 * PA1 = ADC1_IN1 = LDR / rain input (A1). */

/* User button pins: D2=PA10, D3=PB3, D4=PB5, D5=PB4. */
#define BTN_SELECT_PIN              (10u)
#define BTN_CONFIRM_PIN             (3u)
#define BTN_EMERGENCY_PIN           (5u)
#define BTN_CANCEL_PIN              (4u)

/* Button ports: 0 = GPIOA, 1 = GPIOB. */
#define BTN_SELECT_PORT             (0u)
#define BTN_CONFIRM_PORT            (1u)
#define BTN_EMERGENCY_PORT          (1u)
#define BTN_CANCEL_PORT             (1u)

/* Button EXTI lines. */
#define BTN_SELECT_EXTI_LINE        (10u)
#define BTN_CONFIRM_EXTI_LINE       (3u)
#define BTN_EMERGENCY_EXTI_LINE     (5u)
#define BTN_CANCEL_EXTI_LINE        (4u)

/* LED pins and ports (Blue=PA5, Red=PA6, Yellow=PA7, Green=PB6). */
#define LED_GREEN_PIN               (6u)
#define LED_GREEN_PORT              GPIOB

#define LED_YELLOW_PIN              (7u)
#define LED_YELLOW_PORT             GPIOA

#define LED_RED_PIN                 (6u)
#define LED_RED_PORT                GPIOA

#define LED_BLUE_PIN                (5u)
#define LED_BLUE_PORT               GPIOA

/* Buzzer pin: PC4 (CN10 pin 34). */
#define BUZZER_PIN                  (4u)
#define BUZZER_ACTIVE_LOW           (0u)

/* Buzzer type:
 * 1 = passive buzzer (HW-508 / KY-006): needs a square wave, TIM2 makes the tone.
 * 0 = active buzzer (HW-512 type): pin HIGH = sound. */
#define BUZZER_PASSIVE              (1u)

/* Passive buzzer tone (HW-508 sounds best around 1.5 - 2.5 kHz). */
#define BUZZER_TONE_HZ              (2000u)  /* WARNING / FLOOD / FAULT beep. */
#define BUZZER_SIREN_LOW_HZ         (1500u)  /* EMERGENCY siren, low note.   */
#define BUZZER_SIREN_HIGH_HZ        (2500u)  /* EMERGENCY siren, high note.  */
#define BUZZER_SIREN_STEP_MS        (250u)   /* Time on each siren note.     */

/* Emergency alarm on/off pattern (ms). OFF = 0 -> continuous sound.
 * EMERGENCY: ไซเรนสลับเสียงสูง-ต่ำ ดังต่อเนื่อง ต่างจากเสียงปี๊บของสถานะอื่น */
#define BUZZER_EMERGENCY_ON_MS      (1000u)
#define BUZZER_EMERGENCY_OFF_MS     (0u)

/* 7-Segment BCD pins. */
#define SEG7_D0_PIN                 (7u)
#define SEG7_D1_PIN                 (8u)
#define SEG7_D2_PIN                 (10u)
#define SEG7_D3_PIN                 (9u)

/* Servo output: PB0 / A3 = TIM3 CH3. */
#define SERVO_PORT_PIN              (0u)

/* USART2: PA2 = TX, PA3 = RX. */
#define UART_TX_PIN                 (2u)
#define UART_RX_PIN                 (3u)

/* HW-507 (DHT11) Temperature and Humidity sensor: PB2 */
#define DHT11_PORT                  GPIOB
#define DHT11_PIN                   (2u)

/* HW-513 Shock / Vibration Sensor DO pin: PC0 (A0 / EXTI0) */
#define SHOCK_SENSOR_PORT           GPIOC
#define SHOCK_SENSOR_PIN            (0u)
#define SHOCK_SENSOR_EXTI_LINE      (0u)

/* System constants. */
#define ADC_FULL_SCALE              (4095u)
#define PERCENT_FULL_SCALE          (100u)
#define SAMPLE_PERIOD_MS            (1000u)
#define GATE_RATE_PERIOD_MS         (500u)
#define GATE_RATE_STEP_PERCENT      (5u)
#define EMERGENCY_LONG_PRESS_MS     (2000u)
#define BUTTON_DEBOUNCE_MS          (30u)

/* Default configuration. */
#define DEFAULT_WARNING_LEVEL       (50u)
#define DEFAULT_FLOOD_LEVEL         (70u)
#define DEFAULT_CRITICAL_LEVEL      (90u)
#define DEFAULT_LOW_WATER_LEVEL     (20u)
#define DEFAULT_STABLE_TIME_S       (5u)
#define DEFAULT_MANUAL_GATE         (50u)

/* Configuration constraints. */
#define MIN_THRESHOLD_GAP           (1u)
#define MIN_STABLE_TIME_S           (2u)
#define MAX_STABLE_TIME_S           (60u)

/* Risk model. */
#define RISK_WARNING_LEVEL          (50u)
#define RATE_RISK_FULL_SCALE        (5u)

/* Dam model. */
#define RAIN_INFLOW_GAIN            (5u)
#define GATE_OUTFLOW_GAIN           (6u)
#define MAX_POWER_W                 (500u)

/* Communication. */
#define UART_BAUD_RATE              (115200u)
#define UART_TX_BUFFER_SIZE         (512u)

/* ADC DMA buffer contains 8 complete Pot/LDR pairs. */
#define ADC_BUFFER_LENGTH           (16u)
#define ADC_HALF_LENGTH             (8u)
#define ADC_SAMPLES_PER_CHANNEL     (4u)

/* Timer and servo configuration for 16 MHz timer clock. */
/* TIM4: 16 MHz / 16 = 1 MHz, ARR 999 -> update every 1 ms.
 * (ARR must not be 0: the STM32 counter is blocked when ARR = 0.) */
#define TIMER4_PRESCALER            (16u - 1u)
#define TIMER4_PERIOD               (1000u - 1u)
#define TIMER3_PRESCALER            (16u - 1u)
#define TIMER3_PERIOD_US            (20000u)
#define SERVO_MIN_US                (1000u)
#define SERVO_MAX_US                (2000u)

#endif /* BOARD_CONFIG_H */

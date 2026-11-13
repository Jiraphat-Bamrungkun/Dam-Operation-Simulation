# DAM-GUARD Clean Rebuild (Version 2.0)

**Target MCU:** STMicroelectronics NUCLEO-F411RE / STM32F411RETx (ARM Cortex-M4 @ 16 MHz)  
**Toolchain:** STM32CubeIDE (Managed GCC C Project)  
**Architecture:** Bare-metal modular C with Register-level HAL/LL driver abstractions, Event-driven FSM, DMA ADC, DWT microsecond timing, and EXTI Interrupts.

---

## 1. System Overview

**DAM-GUARD** is an autonomous Dam Safety & Automated Gate Management System designed for reservoir inflow monitoring, automated flood control, hydro-power estimation, environmental climate tracking, and automated seismic/earthquake disaster protection.

### Core System Features
1. **Automated & Manual Gate Control:** Servo motor PWM control (`TIM3_CH3`) with configurable rate-limiting (5%/0.5s).
2. **Automated Earthquake Defense (`MODE_EARTHQUAKE_LOCK`):**
   * **Automated Sensor Trigger:** HW-513 Shock/Vibration Sensor on `PC0` (`EXTI0`).
   * **Total Safety Lockout:** Freezes gate servo position, pauses FSM state transitions, locks out user buttons.
   * **Visual Hazard Warning:** Turns **ALL 4 LEDs (Green, Yellow, Red, Blue) ON** simultaneously.
   * **Manual Safety Reset:** Blue User Button B1 (`PC13`) resets/unlocks the system back to normal operation.
3. **Environmental & Drought Monitoring:**
   * **HW-507 (DHT11)** Temperature (°C) and Humidity (%RH) sensor read via ARM Cortex-M4 **DWT Cycle Counter** 1-wire timing on `PB2`.
   * **Evaporation & Drought Risk Evaluation:** Displays `DROUGHT: WARN(HIGH)`, `WEATHER: HUMID/RAIN`, or `DROUGHT: NORMAL` on OLED.
4. **4-Line SSD1306 I2C OLED Display (`PB8` SCL, `PB9` SDA):**
   * Line 1: `=== DAM MONITOR ===` (System Header)
   * Line 2: `PWR: xxx` (Power Output estimation)
   * Line 3: `TEMP: xxC  HUM: xx%` (HW-507 Ambient Climate)
   * Line 4: `DROUGHT: NORMAL` / `MODE: EQ_LOCK` (Disaster Status)
5. **UART Telemetry Reporter (`USART2` @ 115200 8N1):** Periodic system log reporting state, mode, ADC values, water level, inflow rate, risk level, gate position, estimated power, and parameter edit state.
6. **Hardware Buzzer Alarm (`TIM4` 1ms interrupt):** Alarm patterns for WARNING, FLOOD, FAULT, and EMERGENCY states.

---

## 2. Complete Hardware Pin Mapping

| Peripheral / Component | STM32 Pin | Port / Channel / EXTI | Description |
| :--- | :--- | :--- | :--- |
| **Potentiometer** | `PA4` | `A2 / ADC1_IN4` | System config & Manual gate control |
| **LDR (Rain Inflow)** | `PA1` | `A1 / ADC1_IN1` | Rain intensity sensor |
| **HW-507 (DHT11)** | `PB2` | `GPIOB Pin 2` | 1-Wire Temp & Humidity Sensor (DWT µs) |
| **HW-513 (Shock Sensor)**| `PC0` | `GPIOC Pin 0 / EXTI0`| Earthquake vibration sensor DO pin |
| **SSD1306 OLED SCL** | `PB8` | `GPIOB Pin 8` | I2C Clock (Open-Drain + Pull-up) |
| **SSD1306 OLED SDA** | `PB9` | `GPIOB Pin 9` | I2C Data (Open-Drain + Pull-up) |
| **Select Button** | `PA10`| `D2 / EXTI10` | Navigate parameter edit menu |
| **Confirm Button** | `PB3` | `D3 / EXTI3` | Confirm parameter edit value |
| **Emergency Button** | `PB5` | `D4 / EXTI5` | Manual Emergency Trigger (Long Press) |
| **Cancel Button** | `PB4` | `D5 / EXTI4` | Cancel menu edit mode |
| **Reset / Lock Button** | `PC13` | `GPIOC Pin 13 / EXTI13`| Nucleo B1 Blue Button (Earthquake Unlock) |
| **LED Green** | `PB6` | `GPIOB Pin 6` | Normal / Recovery / Low Water indicator |
| **LED Yellow** | `PA7` | `GPIOA Pin 7` | Warning state indicator |
| **LED Red** | `PA6` | `GPIOA Pin 6` | Flood / Emergency / Fault indicator |
| **LED Blue** | `PA5` | `GPIOA Pin 5` | Manual mode indicator |
| **Buzzer** | `PC4` | `GPIOC Pin 4` | Passive/Active buzzer output (`TIM4` 1ms) |
| **7-Segment Display** | `PC7, PA8, PB10, PA9`| `BCD D0..D3` | State code numerical output (0..7) |
| **Servo Motor** | `PB0` | `TIM3_CH3` | Gate PWM output (1.0 ms - 2.0 ms) |
| **USART2 TX** | `PA2` | `USART2_TX` | Telemetry log output (115200 baud) |
| **USART2 RX** | `PA3` | `USART2_RX` | Telemetry input |

---

## 3. Control Modes (`ControlMode_t`)

```c
typedef enum
{
    MODE_AUTO = 0,
    MODE_MANUAL,
    MODE_EARTHQUAKE_LOCK
} ControlMode_t;
```

1. **`MODE_AUTO`:** Gate position is controlled dynamically by the FSM based on rain inflow, water level, rate of rise, and calculated risk level.
2. **`MODE_MANUAL`:** Gate position is controlled directly via the Potentiometer (`PA4`).
3. **`MODE_EARTHQUAKE_LOCK`:**
   * Triggered automatically by **HW-513 Shock Sensor** (`PC0` / `EXTI0`) or manually via B1 Button (`PC13`).
   * **Gate Freeze:** Servo holds exact current position; FSM state transitions and rate-limiting are paused.
   * **Visual Hazard Alert:** **ALL 4 LEDs (Green, Yellow, Red, Blue) turn ON** simultaneously.
   * **Button Lockout:** All user menu buttons are disabled.
   * **Manual Reset:** Pressing B1 (`PC13`) unlocks the system, restores previous mode, and resumes normal operation.

---

## 4. FSM States (`DamState_t`)

| State Code | Enum Symbol | Description & Trigger Conditions |
| :---: | :--- | :--- |
| `0` | `STATE_INIT` | Hardware initialization and self-test. |
| `1` | `STATE_NORMAL` | Reservoir operating within safe limits. |
| `2` | `STATE_WARNING` | Risk level exceeds `RISK_WARNING_LEVEL` (50%). Buzzer beeps. |
| `3` | `STATE_FLOOD_CONTROL` | Water level reaches `flood_level`. Gate opens to discharge. |
| `4` | `STATE_EMERGENCY` | Water level reaches `critical_level` (90%). Siren alarm sounds. |
| `5` | `STATE_RECOVERY` | Water level returning to safe levels; waiting for stable time. |
| `6` | `STATE_LOW_WATER` | Water level below `low_water_level` (20%). Conservation mode (Gate 0%). |
| `7` | `STATE_FAULT` | System configuration invalid or hardware error. |

---

## 5. Directory Structure & Module Breakdown

```text
DAM_GUARD_Clean/
├── Inc/
│   ├── adc_driver.h        # DMA-driven dual ADC scanner
│   ├── app_manager.h       # Main application coordinator
│   ├── board_config.h      # Master pin definitions & system constants
│   ├── button_manager.h    # Application button event processor
│   ├── dam_model.h         # Water level, inflow & power model
│   ├── dht11.h             # HW-507 (DHT11) driver header
│   ├── exti_driver.h       # EXTI edge interrupt handler header
│   ├── fsm_manager.h       # Finite state machine & mode definitions
│   ├── gate_controller.h   # Servo gate rate limiter
│   ├── gpio_driver.h       # LED & Buzzer GPIO output driver
│   ├── oled_ssd1306.h      # SSD1306 I2C OLED display driver
│   ├── sevenseg.h          # 7-Segment BCD output driver
│   ├── system_time.h       # SysTick 1ms millisecond timebase
│   ├── timer_driver.h      # TIM3 PWM Servo & TIM4 1ms Buzzer driver
│   ├── uart_driver.h       # USART2 115200 baud driver
│   └── watchdog_driver.h   # Independent Watchdog (IWDG)
├── Src/
│   ├── adc_driver.c
│   ├── app_manager.c
│   ├── button_manager.c
│   ├── dam_model.c
│   ├── dht11.c             # Uses ARM Cortex-M4 DWT Cycle Counter for 1us timing
│   ├── exti_driver.c       # Handles EXTI0 (PC0), EXTI3 (PB3), EXTI4 (PB4), EXTI5 (PB5), EXTI10 (PA10), EXTI13 (PC13)
│   ├── fsm_manager.c
│   ├── gate_controller.c
│   ├── gpio_driver.c
│   ├── main.c
│   ├── oled_ssd1306.c      # Full 5x7 ASCII font, 4-line display
│   ├── sevenseg.c
│   ├── system_time.c
│   ├── timer_driver.c
│   ├── uart_driver.c
│   └── watchdog_driver.c
├── STM32F411RETX_FLASH.ld  # Flash linker script
└── README.md
```

---

## 6. How to Build & Flash

1. Open **STM32CubeIDE**.
2. Select **File -> Import -> Existing Projects into Workspace**.
3. Select `DAM_GUARD_Clean` folder as root directory.
4. Click **Finish**.
5. Build the project using **Project -> Build Project** (`Ctrl + B`).
6. Connect NUCLEO-F411RE via USB and click **Run / Debug**.

---

## 7. Status Report Example (UART output @ 115200 baud)

```text
[STATE]
State:NORMAL
Code:1
Mode:Earthquake_lock

[SENSOR]
PotRaw:2048
RainRaw:1024
Water:45%
Rain:10%
Rate:0%/s
Risk:22%

[CONTROL]
Gate:20%
Power:180W

[USER EDIT]
EditMode:OFF
Param:1.WARN_LVL
SavedVal:50
LivePotVal:50
--------------------
```

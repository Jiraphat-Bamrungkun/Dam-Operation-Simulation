/*
 ******************************************************************************
 * File Name    : oled_ssd1306.c
 * Description  : Lightweight SSD1306 I2C driver (PB8=SCL/SCK, PB9=SDA).
 * Date         : 2026-10-08
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "oled_ssd1306.h"

#define SSD1306_I2C_ADDR    (0x78u) /* 8-bit I2C address (0x3C << 1) */
#define SSD1306_SCL_PIN     (8u)    /* PB8 = SCK / SCL (D15) */
#define SSD1306_SDA_PIN     (9u)    /* PB9 = SDA (D14) */

/* 5x7 ASCII Font lookup table */
static const uint8_t gu1g_font5x7_digits[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* '0' */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* '1' */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* '2' */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* '3' */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* '4' */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* '5' */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* '6' */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* '7' */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* '8' */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* '9' */
};

static const uint8_t gu1g_font5x7_alpha[][5] = {
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, /* 'A' */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* 'B' */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* 'C' */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* 'D' */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* 'E' */
    {0x7F, 0x09, 0x09, 0x09, 0x01}, /* 'F' */
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* 'G' */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* 'H' */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* 'I' */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* 'J' */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* 'K' */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* 'L' */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* 'M' */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* 'N' */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* 'O' */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* 'P' */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* 'Q' */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* 'R' */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* 'S' */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* 'T' */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* 'U' */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* 'V' */
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* 'W' */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* 'X' */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* 'Y' */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* 'Z' */
};

/* Special characters */
static const uint8_t gu1g_space[5]   = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t gu1g_colon[5]   = {0x00, 0x36, 0x36, 0x00, 0x00};
static const uint8_t gu1g_equal[5]   = {0x14, 0x14, 0x14, 0x14, 0x14};
static const uint8_t gu1g_minus[5]   = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t gu1g_percent[5] = {0x23, 0x13, 0x08, 0x64, 0x62};
static const uint8_t gu1g_slash[5]   = {0x20, 0x10, 0x08, 0x04, 0x02};
static const uint8_t gu1g_lparen[5]  = {0x00, 0x1C, 0x22, 0x41, 0x00};
static const uint8_t gu1g_rparen[5]  = {0x00, 0x41, 0x22, 0x1C, 0x00};

static void vds_I2C_Delay(void);
static void vds_I2C_Start(void);
static void vds_I2C_Stop(void);
static bool xbs_I2C_WriteByte(uint8_t u1t_data);
static void vds_OLED_WriteCommand(uint8_t u1t_cmd);
static void vds_OLED_WriteData(const uint8_t *pu1t_data, uint16_t u2t_len);
static void vds_OLED_SetCursor(uint8_t u1t_page, uint8_t u1t_col);
static void vds_OLED_DrawChar5x7(uint8_t u1t_page, uint8_t u1t_col, char c_char);
static void vds_OLED_DrawString5x7(uint8_t u1t_page, uint8_t u1t_col, const char *cpxt_str);

void vdg_OLED_Init(void)
{
    /* Enable GPIOB clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* Configure PB8 (SCL) and PB9 (SDA) as Open-Drain Output with Pull-Up */
    GPIOB->MODER &= ~((0x3u << (SSD1306_SCL_PIN * 2u)) | (0x3u << (SSD1306_SDA_PIN * 2u)));
    GPIOB->MODER |= ((0x1u << (SSD1306_SCL_PIN * 2u)) | (0x1u << (SSD1306_SDA_PIN * 2u)));

    GPIOB->OTYPER |= ((1u << SSD1306_SCL_PIN) | (1u << SSD1306_SDA_PIN));
    GPIOB->PUPDR &= ~((0x3u << (SSD1306_SCL_PIN * 2u)) | (0x3u << (SSD1306_SDA_PIN * 2u)));
    GPIOB->PUPDR |= ((0x1u << (SSD1306_SCL_PIN * 2u)) | (0x1u << (SSD1306_SDA_PIN * 2u)));

    /* Set lines HIGH initially */
    GPIOB->ODR |= ((1u << SSD1306_SCL_PIN) | (1u << SSD1306_SDA_PIN));

    for (volatile uint32_t i = 0; i < 50000; i++) { __NOP(); }

    /* SSD1306 Initialization Sequence */
    vds_OLED_WriteCommand(0xAE); /* Display OFF */
    vds_OLED_WriteCommand(0xD5); /* Set Display Clock Divide Ratio */
    vds_OLED_WriteCommand(0x80);
    vds_OLED_WriteCommand(0xA8); /* Set Multiplex Ratio */
    vds_OLED_WriteCommand(0x3F);
    vds_OLED_WriteCommand(0xD3); /* Set Display Offset */
    vds_OLED_WriteCommand(0x00);
    vds_OLED_WriteCommand(0x40); /* Set Start Line 0 */
    vds_OLED_WriteCommand(0x8D); /* Charge Pump Setup */
    vds_OLED_WriteCommand(0x14); /* Enable Charge Pump */
    vds_OLED_WriteCommand(0x20); /* Set Memory Addressing Mode */
    vds_OLED_WriteCommand(0x02); /* Page Addressing Mode */
    vds_OLED_WriteCommand(0xA1); /* Segment Re-map */
    vds_OLED_WriteCommand(0xC8); /* COM Output Scan Direction */
    vds_OLED_WriteCommand(0xDA); /* Set COM Pins Hardware Config */
    vds_OLED_WriteCommand(0x12);
    vds_OLED_WriteCommand(0x81); /* Set Contrast Control */
    vds_OLED_WriteCommand(0xCF);
    vds_OLED_WriteCommand(0xD9); /* Set Pre-charge Period */
    vds_OLED_WriteCommand(0xF1);
    vds_OLED_WriteCommand(0xDB); /* Set VCOMH Deselect Level */
    vds_OLED_WriteCommand(0x40);
    vds_OLED_WriteCommand(0xA4); /* Entire Display ON */
    vds_OLED_WriteCommand(0xA6); /* Normal Display */
    vds_OLED_WriteCommand(0xAF); /* Display ON */

    /* Clear screen pages 0..7 */
    uint8_t u1t_zeros[128] = {0};
    for (uint8_t u1t_p = 0; u1t_p < 8; u1t_p++)
    {
        vds_OLED_SetCursor(u1t_p, 0);
        vds_OLED_WriteData(u1t_zeros, 128);
    }
}

void vdg_OLED_UpdatePower(uint16_t u2t_power_w)
{
    vdg_OLED_UpdateDisplay(u2t_power_w, 0, 0, 0, false);
}

void vdg_OLED_UpdateDisplay(uint16_t u2t_power_w, uint8_t u1t_water_percent, uint8_t u1t_temp, uint8_t u1t_humidity, bool xb_eq_lock)
{
    (void)u1t_water_percent;
    char buf_pwr[32];
    char buf_env[32];
    const char *cpxt_drought = "DROUGHT: NORMAL";

    snprintf(buf_pwr, sizeof(buf_pwr), "PWR: %u", (unsigned int)u2t_power_w);
    
    if (xb_eq_lock == true)
    {
        cpxt_drought = "MODE: EQ_LOCK";
    }
    else
    {
        if ((u1t_temp == 0) && (u1t_humidity == 0))
        {
            snprintf(buf_env, sizeof(buf_env), "TEMP:--C  HUM:--%%");
            cpxt_drought = "SENSOR: READING";
        }
        else
        {
            snprintf(buf_env, sizeof(buf_env), "TEMP:%2uC  HUM:%2u%%", (unsigned int)u1t_temp, (unsigned int)u1t_humidity);
            
            if ((u1t_temp >= 35u) && (u1t_humidity <= 40u))
            {
                cpxt_drought = "DROUGHT: WARN(HIGH)";
            }
            else if (u1t_humidity >= 80u)
            {
                cpxt_drought = "WEATHER: HUMID/RAIN";
            }
            else if (u1t_temp >= 32u)
            {
                cpxt_drought = "DROUGHT: MODERATE";
            }
            else
            {
                cpxt_drought = "DROUGHT: NORMAL";
            }
        }
    }

    /* Page 0: Line 1 - Original header */
    vds_OLED_DrawString5x7(0, 10, "=== DAM MONITOR ===");

    /* Page 2: Line 2 - PWR only (no W / Watt) */
    vds_OLED_DrawString5x7(2, 4, buf_pwr);

    /* Page 4: Line 3 - Temperature & Humidity from HW-507 */
    vds_OLED_DrawString5x7(4, 4, buf_env);

    /* Page 6: Line 4 - Drought Warning / Earthquake Lock Status */
    vds_OLED_DrawString5x7(6, 4, cpxt_drought);
}

static void vds_I2C_Delay(void)
{
    for (volatile uint32_t i = 0; i < 20; i++) { __NOP(); }
}

static void vds_I2C_Start(void)
{
    GPIOB->ODR |= (1u << SSD1306_SDA_PIN);
    GPIOB->ODR |= (1u << SSD1306_SCL_PIN);
    vds_I2C_Delay();
    GPIOB->ODR &= ~(1u << SSD1306_SDA_PIN);
    vds_I2C_Delay();
    GPIOB->ODR &= ~(1u << SSD1306_SCL_PIN);
    vds_I2C_Delay();
}

static void vds_I2C_Stop(void)
{
    GPIOB->ODR &= ~(1u << SSD1306_SDA_PIN);
    GPIOB->ODR |= (1u << SSD1306_SCL_PIN);
    vds_I2C_Delay();
    GPIOB->ODR |= (1u << SSD1306_SDA_PIN);
    vds_I2C_Delay();
}

static bool xbs_I2C_WriteByte(uint8_t u1t_data)
{
    for (int8_t s1t_i = 7; s1t_i >= 0; s1t_i--)
    {
        if ((u1t_data & (1u << (uint8_t)s1t_i)) != 0u)
        {
            GPIOB->ODR |= (1u << SSD1306_SDA_PIN);
        }
        else
        {
            GPIOB->ODR &= ~(1u << SSD1306_SDA_PIN);
        }

        vds_I2C_Delay();
        GPIOB->ODR |= (1u << SSD1306_SCL_PIN);
        vds_I2C_Delay();
        GPIOB->ODR &= ~(1u << SSD1306_SCL_PIN);
    }

    /* ACK clock pulse */
    GPIOB->ODR |= (1u << SSD1306_SDA_PIN);
    vds_I2C_Delay();
    GPIOB->ODR |= (1u << SSD1306_SCL_PIN);
    vds_I2C_Delay();
    GPIOB->ODR &= ~(1u << SSD1306_SCL_PIN);
    vds_I2C_Delay();

    return true;
}

static void vds_OLED_WriteCommand(uint8_t u1t_cmd)
{
    vds_I2C_Start();
    (void)xbs_I2C_WriteByte(SSD1306_I2C_ADDR);
    (void)xbs_I2C_WriteByte(0x00); /* Co=0, D/C#=0 */
    (void)xbs_I2C_WriteByte(u1t_cmd);
    vds_I2C_Stop();
}

static void vds_OLED_WriteData(const uint8_t *pu1t_data, uint16_t u2t_len)
{
    if (pu1t_data != NULL)
    {
        vds_I2C_Start();
        (void)xbs_I2C_WriteByte(SSD1306_I2C_ADDR);
        (void)xbs_I2C_WriteByte(0x40); /* Co=0, D/C#=1 */
        for (uint16_t u2t_i = 0u; u2t_i < u2t_len; u2t_i++)
        {
            (void)xbs_I2C_WriteByte(pu1t_data[u2t_i]);
        }
        vds_I2C_Stop();
    }
}

static void vds_OLED_SetCursor(uint8_t u1t_page, uint8_t u1t_col)
{
    vds_OLED_WriteCommand((uint8_t)(0xB0u | (u1t_page & 0x07u)));
    vds_OLED_WriteCommand((uint8_t)(0x00u | (u1t_col & 0x0Fu)));
    vds_OLED_WriteCommand((uint8_t)(0x10u | ((u1t_col >> 4u) & 0x0Fu)));
}

static void vds_OLED_DrawChar5x7(uint8_t u1t_page, uint8_t u1t_col, char c_char)
{
    const uint8_t *pu1t_glyph = gu1g_space;

    if ((c_char >= '0') && (c_char <= '9'))
    {
        pu1t_glyph = gu1g_font5x7_digits[c_char - '0'];
    }
    else if ((c_char >= 'A') && (c_char <= 'Z'))
    {
        pu1t_glyph = gu1g_font5x7_alpha[c_char - 'A'];
    }
    else if ((c_char >= 'a') && (c_char <= 'z'))
    {
        pu1t_glyph = gu1g_font5x7_alpha[c_char - 'a'];
    }
    else if (c_char == ':') { pu1t_glyph = gu1g_colon; }
    else if (c_char == '=') { pu1t_glyph = gu1g_equal; }
    else if (c_char == '-') { pu1t_glyph = gu1g_minus; }
    else if (c_char == '%') { pu1t_glyph = gu1g_percent; }
    else if (c_char == '/') { pu1t_glyph = gu1g_slash; }
    else if (c_char == '(') { pu1t_glyph = gu1g_lparen; }
    else if (c_char == ')') { pu1t_glyph = gu1g_rparen; }
    else { pu1t_glyph = gu1g_space; }

    vds_OLED_SetCursor(u1t_page, u1t_col);
    vds_OLED_WriteData(pu1t_glyph, 5);

    /* 1 pixel space between chars */
    uint8_t u1t_space = 0x00;
    vds_OLED_WriteData(&u1t_space, 1);
}

static void vds_OLED_DrawString5x7(uint8_t u1t_page, uint8_t u1t_col, const char *cpxt_str)
{
    uint8_t u1t_curr_col = u1t_col;

    if (cpxt_str != NULL)
    {
        while (*cpxt_str != '\0')
        {
            vds_OLED_DrawChar5x7(u1t_page, u1t_curr_col, *cpxt_str);
            u1t_curr_col += 6;
            cpxt_str++;
        }
    }
}

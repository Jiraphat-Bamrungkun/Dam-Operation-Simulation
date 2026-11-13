/*
 ******************************************************************************
 * File Name    : adc_driver.c
 * Description  : ADC1 scan + DMA circular buffer.
 * Date         : 2026-09-22
 ******************************************************************************
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "adc_driver.h"

static volatile uint16_t gu2g_adc_buffer[ADC_BUFFER_LENGTH];
static volatile uint16_t gu2g_config_average = 0u;
static volatile uint16_t gu2g_rain_average = 0u;
static volatile bool gxb_adc_ready = false;
static volatile bool gxb_adc_error = false;

static void vds_ADC_GPIO_Init(void);
static void vds_ADC_DMA_Init(void);
static void vds_ADC_Configure(void);
static void vds_ADC_ProcessHalf(uint16_t u2t_start_index);

void vdg_ADC_Init(void)
{
    gu2g_config_average = 0u;
    gu2g_rain_average = 0u;
    gxb_adc_ready = false;
    gxb_adc_error = false;

    vds_ADC_GPIO_Init();
    vds_ADC_DMA_Init();
    vds_ADC_Configure();
}

bool xbg_ADC_GetLatest(AdcData_t *pxg_data)
{
    bool xb_result = false;
    uint16_t u2t_config = 0u;
    uint16_t u2t_rain = 0u;

    if (pxg_data != NULL)
    {
        if (gxb_adc_error == false)
        {
            if (gxb_adc_ready == true)
            {
                u2t_config = gu2g_config_average;
                u2t_rain = gu2g_rain_average;
                gxb_adc_ready = false;

                pxg_data->config_raw = u2t_config;
                pxg_data->rain_raw = u2t_rain;
                xb_result = true;
            }
            else
            {
                /* No completed DMA block is available. */
            }
        }
        else
        {
            /* ADC DMA error has been latched. */
        }
    }
    else
    {
        /* Invalid destination pointer. */
    }

    return xb_result;
}

void DMA2_Stream0_IRQHandler(void)
{
    uint32_t u4t_flags = DMA2->LISR;

    if ((u4t_flags & DMA_LISR_TEIF0) != 0u)
    {
        DMA2->LIFCR = DMA_LIFCR_CTEIF0;
        gxb_adc_error = true;
        gxb_adc_ready = false;
    }
    else
    {
        /* No DMA transfer error. */
    }

    if ((u4t_flags & DMA_LISR_HTIF0) != 0u)
    {
        DMA2->LIFCR = DMA_LIFCR_CHTIF0;

        if (gxb_adc_error == false)
        {
            vds_ADC_ProcessHalf(0u);
            gxb_adc_ready = true;
        }
        else
        {
            /* Ignore samples while an error is latched. */
        }
    }
    else
    {
        /* No half-transfer event. */
    }

    if ((u4t_flags & DMA_LISR_TCIF0) != 0u)
    {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;

        if (gxb_adc_error == false)
        {
            vds_ADC_ProcessHalf(ADC_HALF_LENGTH);
            gxb_adc_ready = true;
        }
        else
        {
            /* Ignore samples while an error is latched. */
        }
    }
    else
    {
        /* No transfer-complete event. */
    }
}

static void vds_ADC_GPIO_Init(void)
{
    uint32_t u4t_pa4_shift = 8u;
    uint32_t u4t_pa1_shift = 2u;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    GPIOA->MODER &= ~(0x3u << u4t_pa4_shift);
    GPIOA->MODER |= (0x3u << u4t_pa4_shift);
    GPIOA->PUPDR &= ~(0x3u << u4t_pa4_shift);

    GPIOA->MODER &= ~(0x3u << u4t_pa1_shift);
    GPIOA->MODER |= (0x3u << u4t_pa1_shift);
    GPIOA->PUPDR &= ~(0x3u << u4t_pa1_shift);
}

static void vds_ADC_DMA_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    DMA2_Stream0->CR &= ~DMA_SxCR_EN;
    DMA2_Stream0->CR = 0u;
    DMA2_Stream0->FCR = 0u;

    DMA2->LIFCR = DMA_LIFCR_CFEIF0 |
                  DMA_LIFCR_CDMEIF0 |
                  DMA_LIFCR_CTEIF0 |
                  DMA_LIFCR_CHTIF0 |
                  DMA_LIFCR_CTCIF0;

    DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;
    DMA2_Stream0->M0AR = (uint32_t)gu2g_adc_buffer;
    DMA2_Stream0->NDTR = ADC_BUFFER_LENGTH;

    DMA2_Stream0->CR |= DMA_SxCR_MINC;
    DMA2_Stream0->CR |= DMA_SxCR_CIRC;
    DMA2_Stream0->CR |= DMA_SxCR_PSIZE_0;
    DMA2_Stream0->CR |= DMA_SxCR_MSIZE_0;
    DMA2_Stream0->CR |= DMA_SxCR_TCIE;
    DMA2_Stream0->CR |= DMA_SxCR_HTIE;
    DMA2_Stream0->CR |= DMA_SxCR_TEIE;
    DMA2_Stream0->CR |= DMA_SxCR_PL_1;

    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    DMA2_Stream0->CR |= DMA_SxCR_EN;
}

static void vds_ADC_Configure(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC1->CR1 = 0u;
    ADC1->CR1 |= ADC_CR1_SCAN;

    ADC1->CR2 = 0u;
    ADC1->CR2 |= ADC_CR2_DMA;
    ADC1->CR2 |= ADC_CR2_DDS;
    ADC1->CR2 |= ADC_CR2_CONT;

    ADC1->SMPR2 = 0u;
    ADC1->SMPR2 |= ADC_SMPR2_SMP4_1;
    ADC1->SMPR2 |= ADC_SMPR2_SMP1_1;

    ADC1->SQR1 = (1u << ADC_SQR1_L_Pos);
    ADC1->SQR3 = (ADC_CONFIG_CHANNEL << ADC_SQR3_SQ1_Pos) |
                 (ADC_RAIN_CHANNEL << ADC_SQR3_SQ2_Pos);

    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

static void vds_ADC_ProcessHalf(uint16_t u2t_start_index)
{
    uint32_t u4t_config_sum = 0u;
    uint32_t u4t_rain_sum = 0u;
    uint16_t u2t_index = u2t_start_index;
    uint16_t u2t_end_index = (uint16_t)(u2t_start_index + ADC_HALF_LENGTH);

    while (u2t_index < u2t_end_index)
    {
        u4t_config_sum += gu2g_adc_buffer[u2t_index];
        u4t_rain_sum += gu2g_adc_buffer[(uint16_t)(u2t_index + 1u)];
        u2t_index = (uint16_t)(u2t_index + 2u);
    }

    gu2g_config_average =
        (uint16_t)(u4t_config_sum / ADC_SAMPLES_PER_CHANNEL);
    gu2g_rain_average =
        (uint16_t)(u4t_rain_sum / ADC_SAMPLES_PER_CHANNEL);
}

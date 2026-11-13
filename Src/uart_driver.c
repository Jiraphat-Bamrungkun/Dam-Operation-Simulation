/*
 ******************************************************************************
 * File Name    : uart_driver.c
 * Description  : USART2 TX using DMA.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#define STM32F411xE
#include "stm32f4xx.h"
#include "board_config.h"
#include "uart_driver.h"
#include <stddef.h>

static volatile bool gxb_uart_busy = false;
static uint8_t gu1g_uart_tx_buffer[UART_TX_BUFFER_SIZE];

static uint16_t u2s_UART_StringLength(const char *cpxt_text);
static void vds_UART_GPIO_Init(void);
static void vds_UART_DMA_Init(void);

void vdg_UART_Init(void)
{
    vds_UART_GPIO_Init();
    vds_UART_DMA_Init();

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    USART2->CR1 = 0u;
    USART2->BRR = 16000000u / 115200u;
    USART2->CR3 |= USART_CR3_DMAT;
    USART2->CR1 |= USART_CR1_TE;
    USART2->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

bool xbg_UART_SendText(const char *cpxt_text)
{
    uint16_t u2t_length = 0u;
    bool xb_result = false;

    if ((cpxt_text != NULL) && (gxb_uart_busy == false))
    {
        u2t_length = u2s_UART_StringLength(cpxt_text);

        if ((u2t_length > 0u) && (u2t_length <= UART_TX_BUFFER_SIZE))
        {
            uint16_t u2t_index = 0u;

            while (u2t_index < u2t_length)
            {
                gu1g_uart_tx_buffer[u2t_index] =
                    (uint8_t)cpxt_text[u2t_index];
                u2t_index++;
            }

            gxb_uart_busy = true;

            DMA1_Stream6->CR &= ~DMA_SxCR_EN;
            DMA1->HIFCR = DMA_HIFCR_CTCIF6 |
                          DMA_HIFCR_CHTIF6 |
                          DMA_HIFCR_CTEIF6 |
                          DMA_HIFCR_CDMEIF6 |
                          DMA_HIFCR_CFEIF6;

            DMA1_Stream6->PAR = (uint32_t)&USART2->DR;
            DMA1_Stream6->M0AR = (uint32_t)gu1g_uart_tx_buffer;
            DMA1_Stream6->NDTR = u2t_length;
            DMA1_Stream6->CR |= DMA_SxCR_EN;
            xb_result = true;
        }
        else
        {
            gxb_uart_busy = false;
        }
    }
    else
    {
        /* Invalid argument or transmitter is busy. */
    }

    return xb_result;
}

bool xbg_UART_IsBusy(void)
{
    return gxb_uart_busy;
}

void DMA1_Stream6_IRQHandler(void)
{
    uint32_t u4t_flags = DMA1->HISR;

    if ((u4t_flags & DMA_HISR_TCIF6) != 0u)
    {
        DMA1->HIFCR = DMA_HIFCR_CTCIF6;
        DMA1_Stream6->CR &= ~DMA_SxCR_EN;
        gxb_uart_busy = false;
    }
    else
    {
        /* No transmit complete event. */
    }

    if ((u4t_flags & DMA_HISR_TEIF6) != 0u)
    {
        DMA1->HIFCR = DMA_HIFCR_CTEIF6;
        DMA1_Stream6->CR &= ~DMA_SxCR_EN;
        gxb_uart_busy = false;
    }
    else
    {
        /* No transfer error. */
    }
}

static void vds_UART_GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~((0x3u << (UART_TX_PIN * 2u)) |
                      (0x3u << (UART_RX_PIN * 2u)));
    GPIOA->MODER |= ((0x2u << (UART_TX_PIN * 2u)) |
                     (0x2u << (UART_RX_PIN * 2u)));

    GPIOA->AFR[0] &= ~((0xFu << (UART_TX_PIN * 4u)) |
                       (0xFu << (UART_RX_PIN * 4u)));
    GPIOA->AFR[0] |= ((0x7u << (UART_TX_PIN * 4u)) |
                      (0x7u << (UART_RX_PIN * 4u)));
}

static void vds_UART_DMA_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    DMA1_Stream6->CR = 0u;
    DMA1_Stream6->CR |= (4u << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream6->CR |= DMA_SxCR_MINC;
    DMA1_Stream6->CR |= DMA_SxCR_DIR_0;
    DMA1_Stream6->CR |= DMA_SxCR_TCIE;
    DMA1_Stream6->CR |= DMA_SxCR_TEIE;
    DMA1_Stream6->CR |= DMA_SxCR_PL_1;
    DMA1_Stream6->FCR = 0u;
}

static uint16_t u2s_UART_StringLength(const char *cpxt_text)
{
    uint16_t u2t_length = 0u;

    if (cpxt_text != NULL)
    {
        while ((cpxt_text[u2t_length] != '\0') &&
               (u2t_length < UART_TX_BUFFER_SIZE))
        {
            u2t_length++;
        }
    }
    else
    {
        u2t_length = 0u;
    }

    return u2t_length;
}

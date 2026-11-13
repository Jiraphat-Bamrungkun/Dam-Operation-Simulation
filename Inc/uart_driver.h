/*
 ******************************************************************************
 * File Name    : uart_driver.h
 * Description  : USART2 interrupt/DMA driver.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

void vdg_UART_Init(void);
bool xbg_UART_SendText(const char *cpxt_text);
bool xbg_UART_IsBusy(void);

#endif /* UART_DRIVER_H */

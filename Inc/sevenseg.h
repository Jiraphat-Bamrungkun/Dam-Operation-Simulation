/*
 ******************************************************************************
 * File Name    : sevenseg.h
 * Description  : 7-Segment BCD state display.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#ifndef SEVENSEG_H
#define SEVENSEG_H

#include <stdint.h>

void vdg_Seg7_Init(void);
void vdg_Seg7_WriteDigit(uint8_t u1t_digit);

#endif /* SEVENSEG_H */

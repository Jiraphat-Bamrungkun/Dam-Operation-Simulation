/*
 ******************************************************************************
 * File Name    : system_time.h
 * Description  : 1 ms system timebase.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdint.h>

void vdg_Time_Init(void);
uint32_t u4g_Time_GetMs(void);

#endif /* SYSTEM_TIME_H */

/*
 ******************************************************************************
 * File Name    : main.c
 * Description  : DAM-GUARD application entry point.
 * Date         : 2026-09-08
 ******************************************************************************
 */

#include "app_manager.h"

int main(void)
{
    vdg_APP_Init();

    while (1)
    {
        vdg_APP_Run();
    }
}

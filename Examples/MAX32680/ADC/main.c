/******************************************************************************
 * Copyright (C) 2023 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/

/**
 * @file    main.c
 * @brief   ADC demo application
 * @details Continuously monitors the ADC channels
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>

#include <mxc.h>
#include "adc.h"

/***** Definitions *****/

/* Change to #undef USE_INTERRUPTS for polling mode */
// #define USE_INTERRUPTS

/***** Globals *****/
#ifdef USE_INTERRUPTS
volatile unsigned int adc_done = 0;
#endif

static uint16_t adc_val;
uint8_t overflow;

/***** Functions *****/

#ifdef USE_INTERRUPTS
void adc_complete_cb(void *req, int adcRead)
{
    overflow = (adcRead == E_OVERFLOW ? 1 : 0);
    adc_val = adcRead;
    adc_done = 1;
    return;
}
void ADC_IRQHandler(void)
{
    MXC_ADC_Handler();
}
#endif

int main(void)
{
    printf("********** ADC Example **********\n");

    /* Initialize ADC */
    if (MXC_ADC_Init() != E_NO_ERROR) {
        printf("Error Bad Parameter\n");

        while (1) {}
    }

    /* Set up LIMIT0 to monitor high and low trip points */
    MXC_ADC_SetMonitorChannel(MXC_ADC_MONITOR_0, MXC_ADC_CH_4);
    MXC_ADC_SetMonitorHighThreshold(MXC_ADC_MONITOR_0, 0x300);
    MXC_ADC_SetMonitorLowThreshold(MXC_ADC_MONITOR_0, 0x25);
    MXC_ADC_EnableMonitor(MXC_ADC_MONITOR_0);

#ifdef USE_INTERRUPTS
    NVIC_EnableIRQ(ADC_IRQn);
#endif

    while (1) {
        /* Flash LED when starting ADC cycle */
        LED_On(0);
        MXC_TMR_Delay(MXC_TMR0, MSEC(10));
        LED_Off(0);

        /* Convert channel 0 */
#ifdef USE_INTERRUPTS
        adc_done = 0;
        MXC_ADC_StartConversionAsync(MXC_ADC_CH_4, adc_complete_cb);

        while (!adc_done) {}
#else
        adc_val = MXC_ADC_StartConversion(MXC_ADC_CH_4);
        overflow = (adc_val == E_OVERFLOW ? 1 : 0);
#endif

        /* Display results on OLED display, display asterisk if overflow */
        printf("4: 0x%04x%s\n\n", adc_val, overflow ? "*" : " ");

        /* Determine if programmable limits on AIN12 were exceeded */
        if (MXC_ADC_GetFlags() & (MXC_F_ADC_INTR_LO_LIMIT_IF | MXC_F_ADC_INTR_HI_LIMIT_IF)) {
            printf(" %s Limit on AIN12 ",
                   (MXC_ADC_GetFlags() & MXC_F_ADC_INTR_LO_LIMIT_IF) ? "Low" : "High");
            MXC_ADC_ClearFlags(MXC_F_ADC_INTR_LO_LIMIT_IF | MXC_F_ADC_INTR_HI_LIMIT_IF);
        } else {
            printf("                   ");
        }

        printf("\n");

        /* Delay for 1/4 second before next reading */
        MXC_TMR_Delay(MXC_TMR0, MSEC(250));
    }
}

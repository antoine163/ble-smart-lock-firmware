/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 antoine163
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file pwm.c
 * @author antoine163
 * @date 06-04-2024
 * @brief PWM driver
 */

// Include ---------------------------------------------------------------------
#include "pwm.h"

#include <stddef.h>
#include "BlueNRG1_sysCtrl.h"

// Implemented functions -------------------------------------------------------
int pwm_init(pwm_t *dev, MFT_Type *const periph)
{
    if (MFT1 == periph)
        SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_MTFX1, ENABLE);
    else if (MFT2 == periph)
        SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_MTFX2, ENABLE);
    else
        return -1;

    dev->periph = periph;
    dev->period = 0;
    return 0;
}

int pwm_deinit(pwm_t *dev)
{
    if (MFT1 == dev->periph)
        SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_MTFX1, DISABLE);
    else if (MFT2 == dev->periph)
        SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_MTFX2, DISABLE);
    else
        return -1;

    MFT_DeInit(dev->periph);
    dev->periph = NULL;
    return 0;
}

int pwm_config(pwm_t *dev, unsigned int frequency)
{
    // Compute prescaler and period
    unsigned int prescaler = SYST_CLOCK / ( frequency * 256U );
    if (0 == prescaler)
        prescaler = 1;
    else if (256U < prescaler)
        prescaler = 256U;
    dev->period = SYST_CLOCK / ( prescaler * frequency );
 
    // MFTx configuration
    MFT_InitType timer_init;
    MFT_StructInit(&timer_init);
    timer_init.MFT_Mode = MFT_MODE_1;
    timer_init.MFT_Clock1 = MFT_PRESCALED_CLK;
    timer_init.MFT_Clock2 = MFT_NO_CLK;
    timer_init.MFT_Prescaler = prescaler -1;
    timer_init.MFT_CRA = 0;
    timer_init.MFT_CRB = dev->period -1;
    MFT_Init(dev->periph, &timer_init);

    // Connect PWMx output from MFTx to TnA pin
    MFT_TnXEN(dev->periph, MFT_TnA, ENABLE);

    // Start MFT timers
    MFT_Cmd(dev->periph, ENABLE);

    return 0;
}

void pwm_setDc(pwm_t *dev, float duty_cycle)
{
    // Board duty cycle from 0% to 100%
    if (duty_cycle < 0.)
        duty_cycle = 0.;
    else if (duty_cycle > 100.)
        duty_cycle = 100.;

    uint32_t tmcra = (duty_cycle * dev->period) / 100.;
    uint32_t tmcrb = dev->period - tmcra;

    // dev->periph->TNCNT1 = 0; // Reste counter
    dev->periph->TNCRA = (uint16_t)tmcra;
    dev->periph->TNCRB = (uint16_t)tmcrb;
}

void pwm_clearCounter(pwm_t *dev)
{
    dev->periph->TNCNT1 = 0; // Reste counter
}
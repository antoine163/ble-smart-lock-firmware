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
 * @file adc.c
 * @author antoine163
 * @date 25-02-2024
 * @brief ADC driver
 */

// Include ---------------------------------------------------------------------
#include "adc.h"

#include <stddef.h>
#include "BlueNRG1_sysCtrl.h"

// Global variables ------------------------------------------------------------
static adc_t *_adc_dev = NULL;

// Implemented functions -------------------------------------------------------

int adc_init(adc_t *dev, ADC_Type *const periph)
{
    dev->periph = periph;
    dev->ch = ADC_CH_NONE;
    _adc_dev = dev;

    // Enable ADC clk
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_ADC, ENABLE);

    return 0;
}

int adc_deinit(adc_t *dev)
{
    // De init ADC and disable clock
    ADC_DeInit();
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_ADC, DISABLE);

    _adc_dev = NULL;
    dev->periph = NULL;
    return 0;
}

int adc_config(adc_t *dev, adcCh_t ch)
{
    dev->ch = ch;

    // ADC configuration
    ADC_InitType xADC_InitType;
    xADC_InitType.ADC_OSR = ADC_OSR_200;
    xADC_InitType.ADC_Input = dev->ch;
    xADC_InitType.ADC_ConversionMode = ADC_ConversionMode_Single;
    xADC_InitType.ADC_ReferenceVoltage = ADC_ReferenceVoltage_0V6;
    xADC_InitType.ADC_Attenuation = ADC_Attenuation_6dB02;
    ADC_Init(&xADC_InitType);

    // Enable auto offset correction
    ADC_AutoOffsetUpdate(ENABLE);
    ADC_Calibration(ENABLE);

    return 0;
}

float adc_convert_voltage(adc_t *dev)
{
    // Fair 4 conversions semble contourner un peut l'Errata:
    // "ADC does not work properly when a 32 MHz system clock is being used"
    // A condition de ne pas changer de ADC_Input
    for (unsigned int i = 0; i<4; i++)
    {
        ADC_Cmd(ENABLE);

        while (ADC_GetFlagStatus(ADC_FLAG_EOC) == RESET)
        {
        }

        ADC_GetRawData();
    }

    return ADC_GetConvertedData(dev->ch, ADC_ReferenceVoltage_0V6);
}

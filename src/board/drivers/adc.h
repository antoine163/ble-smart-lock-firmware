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
 * @file adc.h
 * @author antoine163
 * @date 24.02.2024
 * @brief ADC driver
 */

#ifndef ADC_H
#define ADC_H

// Include ---------------------------------------------------------------------
#include "BlueNRG1_adc.h"

// Enum ------------------------------------------------------------------------
typedef enum
{
    ADC_CH_NONE = ADC_Input_None,
    ADC_CH_PIN2 = ADC_Input_AdcPin2,
    ADC_CH_PIN1 = ADC_Input_AdcPin1,
    ADC_CH_PIN12_DIFF = ADC_Input_AdcPin12,
    ADC_CH_TEMP = ADC_Input_TempSensor,
    ADC_CH_BATT = ADC_Input_BattSensor,
    ADC_CH_0V6 = ADC_Input_Internal0V60V6
} adcCh_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    ADC_Type *periph;
    adcCh_t ch;
} adc_t;

// Prototype functions ---------------------------------------------------------
int adc_init(adc_t *dev, ADC_Type *const periph);
int adc_deinit(adc_t *dev);
int adc_config(adc_t *dev, adcCh_t ch);

float adc_convert_voltage(adc_t *dev);

#endif // ADC_H

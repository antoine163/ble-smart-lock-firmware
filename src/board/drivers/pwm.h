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
 * @file pwm.h
 * @author antoine163
 * @date 06-04-2024
 * @brief PWM driver
 */

#ifndef PWM_H
#define PWM_H

// Include ---------------------------------------------------------------------
#include "BlueNRG1_mft.h"

// Struct ----------------------------------------------------------------------
typedef struct
{
    MFT_Type *periph;
    uint32_t period;
} pwm_t;

// Prototype functions ---------------------------------------------------------
int pwm_init(pwm_t *dev, MFT_Type *const periph);
int pwm_deinit(pwm_t *dev);
int pwm_config(pwm_t *dev, unsigned int frequency);

void pwm_setDc(pwm_t *dev, float duty_cycle);
void pwm_clearCounter(pwm_t *dev);

#endif // PWM_H

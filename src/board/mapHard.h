/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 antoine163
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

#ifndef MAP_HARD_H
#define MAP_HARD_H

// IN
#define BOND_PIN                GPIO_Pin_7
#define BOND_MODE               GPIO_Input
#define OPENED_PIN              GPIO_Pin_12
#define OPENED_MODE             GPIO_Input

// IN/OUT
#define LOCK_PIN                GPIO_Pin_6
#define LOCK_MODE_OUT           GPIO_Output
#define LOCK_MODE_IN            GPIO_Input

// OUT
#define LED_PIN                 GPIO_Pin_14
#define LED_MODE                GPIO_Output

#define EN_IO_PIN               GPIO_Pin_5
#define EN_IO_MODE              GPIO_Output

// LIGHT
#define LIGHT_RED_PIN           GPIO_Pin_0
#define LIGHT_BLUE_PIN          GPIO_Pin_1
#define LIGHT_GREEN_PIN         GPIO_Pin_2
#define LIGHT_WHITE_PIN         GPIO_Pin_3
#define LIGHT_MODE              GPIO_Output

#define LIGHT_PWM_PIN           GPIO_Pin_4
#define LIGHT_PWM_MODE_OUT      GPIO_Output
#define LIGHT_PWM_MODE_PWM      Serial2_Mode

// UART
#define UART_RX_PIN             GPIO_Pin_11
#define UART_RX_MODE            Serial1_Mode

#define UART_TX_PIN             GPIO_Pin_8
#define UART_TX_MODE            Serial1_Mode

#endif // MAP_HARD_H

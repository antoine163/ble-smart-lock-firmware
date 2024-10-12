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
 * @file taskLight.h
 * @author antoine163
 * @date 07-04-2024
 * @brief Task to manage ambient light and lighting
 */

#ifndef TASK_LIGHT_H
#define TASK_LIGHT_H

// Include ---------------------------------------------------------------------
#include "board.h"

// Prototype functions ---------------------------------------------------------
void taskLightCodeInit();
void taskLightCode(void *parameters);

void taskLightAnimTrans(unsigned int timeToOff,
                        color_t color,
                        unsigned int timeToOn);
void taskLightAnimSin(unsigned int timeToOff,
                      color_t color,
                      float freq);
void taskLightAnimBlink(unsigned int timeToOff,
                        color_t color,
                        unsigned int timeOn,
                        unsigned int timeOff);

#endif // TASK_LIGHT_H

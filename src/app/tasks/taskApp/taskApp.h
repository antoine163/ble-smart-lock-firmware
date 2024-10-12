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
 * @file taskApp.h
 * @author antoine163
 * @date 03-04-2024
 * @brief Task of the application
 */

#ifndef TASK_APP_H
#define TASK_APP_H

// Include ---------------------------------------------------------------------
#include <FreeRTOS.h>
#include "tasks/taskBle/taskBle.h"
#include "board.h"

// Prototype functions ---------------------------------------------------------
void taskAppCodeInit();
void taskAppCode(void *parameters);

int taskAppEnableVerbose(bool enable);
int taskAppResetConfig();

float taskAppGetBrightnessTh();
int taskAppSetBrightnessTh(float th);

uint32_t taskAppGetPin();
int taskAppSetPin(uint32_t pin);

bool taskAppGetVerbose();
int taskAppSetVerbose(bool verbose);

void taskAppUnlock();
void taskAppOpenDoor();

#endif // TASK_APP_H

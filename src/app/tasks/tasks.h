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
 * @file tasks.h
 * @author antoine163
 * @date 02-04-2024
 * @brief All tasks of this firmware
 */

#ifndef TASKS_H
#define TASKS_H

// Include ---------------------------------------------------------------------
#include "taskApp/taskApp.h"
#include "taskBle/taskBle.h"
#include "taskLight/taskLight.h"
#include "taskTerm/taskTerm.h"

// Define ----------------------------------------------------------------------

/**
 * @brief Declaration the list of static tasks.
 *
 * The define @b STATIC_TASK() is defined an used in tasks.c to creat all static
 * tasks.
 *
 * Prototype of STATIC_TASK(task_code, name, stack_size, parameters, priority)
 * The parameters of STATIC_TASK match with the parameters of xTaskCreateStatic()
 * of FreeRTOS.
 * @p taskCode -> pxTaskCode
 * @p name -> pcName
 * @p stackDepth -> ulStackDepth
 * @p parameters -> pvParametersde
 * @p priority -> uxPriority
 */
#define TASKS_STATIC_LIST                                                                             \
    STATIC_TASK(taskAppCode, "App", configMINIMAL_STACK_SIZE / 2 * 2, NULL, tskIDLE_PRIORITY + 2)     \
    STATIC_TASK(taskTermCode, "Term", configMINIMAL_STACK_SIZE / 2 * 3, NULL, tskIDLE_PRIORITY + 1)   \
    STATIC_TASK(taskLightCode, "Light", configMINIMAL_STACK_SIZE / 2 * 2, NULL, tskIDLE_PRIORITY + 1) \
    STATIC_TASK(taskBleCode, "Ble", configMINIMAL_STACK_SIZE / 2 * 3, NULL, tskIDLE_PRIORITY + 3)

// Prototype functions ---------------------------------------------------------

/**
 * @brief Initialise all static tasks of this firmware.
 */

void tasksStaticInit();

/**
 * @brief Create all static tasks of this firmware.
 */
void tasksStaticCreate();

#endif // TASKS_H

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
 * @file tasks.c
 * @author antoine163
 * @date 02-04-2024
 * @brief All tasks of this firmware
 */

// Include ---------------------------------------------------------------------
#include "tasks.h"

#include <FreeRTOS.h>
#include <task.h>

// Global variables ------------------------------------------------------------

// Creat Stack of all static task listed in TASKS_STATIC_LIST
#undef STATIC_TASK
#define STATIC_TASK(taskCode, name, stackDepth, parameters, priority) \
    static StaticTask_t _taskBuffer_##taskCode;                       \
    static StackType_t _tasksStack_##taskCode[stackDepth];
TASKS_STATIC_LIST

void tasksStaticInit()
{
    // Initialise all static task listed in TASKS_STATIC_LIST
#undef STATIC_TASK
#define STATIC_TASK(taskCode, name, stackDepth, parameters, priority) \
    taskCode##Init();

    TASKS_STATIC_LIST
}

// Implemented functions -------------------------------------------------------
void tasksStaticCreate()
{
    // Create all static task listed in TASKS_STATIC_LIST
#undef STATIC_TASK
#define STATIC_TASK(taskCode, name, stackDepth, parameters, priority) \
    xTaskCreateStatic(taskCode,                                       \
                      name,                                           \
                      stackDepth,                                     \
                      parameters,                                     \
                      priority,                                       \
                      _tasksStack_##taskCode,                         \
                      &_taskBuffer_##taskCode);
                      
    TASKS_STATIC_LIST
}
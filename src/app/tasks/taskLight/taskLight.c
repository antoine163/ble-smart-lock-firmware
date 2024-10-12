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

// Include ---------------------------------------------------------------------
#include "taskLight.h"
#include "board.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <math.h>

// Define ----------------------------------------------------------------------
#define _TASK_LIGHT_EVENT_QUEUE_LENGTH 8

// Conf animation
#define ANIM_STEP_TIME 40 // ms

// Enum ------------------------------------------------------------------------
typedef enum
{
    LIGHT_EVENT_ANIM_TRANS,
    LIGHT_EVENT_ANIM_SIN,
    LIGHT_EVENT_ANIM_BLINK
} taskLightEventType_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    void (*func)();
    float currentDc;
    color_t color;
    TickType_t ticksToWait;
    TimeOut_t timeOut;

    bool switchToOff;
    float switchOffDecDc;

    // Specific for sin animation
    float sinInit;
    float sinFreq;
    // Specific for blink animation
    bool blinkOn;
    TickType_t blinkTicksOn;
    TickType_t blinkTicksOff;
    // Specific for transient animation
    float trandIncDc;
} taskLightAnimation_t;

typedef struct
{
    taskLightEventType_t type;
    unsigned int timeToOff;
    color_t color;

    // Specific for sin animation
    float sinFreq;
    // Specific for blink animation
    unsigned int blinkTimeOn;
    unsigned int blinkTimeOff;
    // Specific for transient animation
    unsigned int transTimeToOn;
} taskLightEvent_t;

typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventStaticQueue;
    uint8_t eventQueueStorageArea[sizeof(taskLightEvent_t) * _TASK_LIGHT_EVENT_QUEUE_LENGTH];

    // Light animation
    taskLightAnimation_t anim;
} taskLight_t;

// Global variables ------------------------------------------------------------
static taskLight_t _taskLight = {0};

// Prototype functions ---------------------------------------------------------
TickType_t _taskLightAnim();

void _taskLightInitAnim(unsigned int timeToOff, color_t color);

void _taskLightInitAnimTrans(unsigned int timeToOn);
void _taskLightFuncAnimTrans();

void _taskLightInitAnimSin(float freq, unsigned int timeToOff);
void _taskLightFuncAnimSin();

void _taskLightInitAnimBlink(unsigned int timeOn, unsigned int timeOff);
void _taskLightFuncAnimBlink();

// Implemented functions -------------------------------------------------------
void taskLightCodeInit()
{
    _taskLight.eventQueue = xQueueCreateStatic(_TASK_LIGHT_EVENT_QUEUE_LENGTH,
                                               sizeof(taskLightEvent_t),
                                               _taskLight.eventQueueStorageArea,
                                               &_taskLight.eventStaticQueue);
}

void taskLightCode(__attribute__((unused)) void *parameters)
{
    taskLightEvent_t event;
    TickType_t tickToWait = 0;

    while (1)
    {
        if (xQueueReceive(_taskLight.eventQueue, &event, tickToWait) == pdPASS)
        {
            _taskLightInitAnim(event.timeToOff, event.color);
            switch (event.type)
            {
            case LIGHT_EVENT_ANIM_TRANS: _taskLightInitAnimTrans(event.transTimeToOn); break;
            case LIGHT_EVENT_ANIM_SIN:   _taskLightInitAnimSin(event.sinFreq, event.timeToOff); break;
            case LIGHT_EVENT_ANIM_BLINK: _taskLightInitAnimBlink(event.blinkTimeOn, event.blinkTimeOff); break;
            default:                     break;
            }
        }

        tickToWait = _taskLightAnim();
    }
}

void taskLightAnimTrans(unsigned int timeToOff,
                        color_t color,
                        unsigned int timeToOn)
{
    taskLightEvent_t event = {0};
    event.type = LIGHT_EVENT_ANIM_TRANS;
    event.timeToOff = timeToOff;
    event.color = color;
    event.transTimeToOn = timeToOn;

    xQueueSend(_taskLight.eventQueue, &event, portMAX_DELAY);
}

void taskLightAnimSin(unsigned int timeToOff, color_t color, float freq)
{
    taskLightEvent_t event = {0};
    event.type = LIGHT_EVENT_ANIM_SIN;
    event.timeToOff = timeToOff;
    event.color = color;
    event.sinFreq = freq;

    xQueueSend(_taskLight.eventQueue, &event, portMAX_DELAY);
}

void taskLightAnimBlink(unsigned int timeToOff,
                        color_t color,
                        unsigned int timeOn,
                        unsigned int timeOff)
{
    taskLightEvent_t event = {0};
    event.type = LIGHT_EVENT_ANIM_BLINK;
    event.timeToOff = timeToOff;
    event.color = color;
    event.blinkTimeOn = timeOn;
    event.blinkTimeOff = timeOff;

    xQueueSend(_taskLight.eventQueue, &event, portMAX_DELAY);
}

void _taskLightInitAnimTrans(unsigned int timeToOn)
{
    // Init time on animation
    if (timeToOn == 0)
        _taskLight.anim.trandIncDc = 100.f;
    else
        _taskLight.anim.trandIncDc = 100.f / ((float)timeToOn / (float)ANIM_STEP_TIME);

    // Start transient
    _taskLight.anim.func = _taskLightFuncAnimTrans;
}

void _taskLightFuncAnimTrans()
{
    if (xTaskCheckForTimeOut(&_taskLight.anim.timeOut, &_taskLight.anim.ticksToWait) != pdFALSE)
    {
        // Init time
        _taskLight.anim.ticksToWait = ANIM_STEP_TIME / portTICK_PERIOD_MS;
        vTaskSetTimeOutState(&_taskLight.anim.timeOut);

        _taskLight.anim.currentDc += _taskLight.anim.trandIncDc;
        boardSetLightDc(_taskLight.anim.currentDc);

        // Stop transient
        if (_taskLight.anim.currentDc >= 100.f)
        {
            _taskLight.anim.currentDc = 100.f;
            _taskLight.anim.func = NULL;
        }
    }
}

void _taskLightInitAnimSin(float freq, unsigned int timeToOff)
{
    // Init sin animation
    _taskLight.anim.sinFreq = freq;

    if (_taskLight.anim.switchToOff == false)
    {
        _taskLight.anim.currentDc = fmin(_taskLight.anim.currentDc, 99.9f);
        _taskLight.anim.currentDc = fmax(0.1f, _taskLight.anim.currentDc);

        float tick = xTaskGetTickCount();
        _taskLight.anim.sinInit =
            -(tick * (M_TWOPI * _taskLight.anim.sinFreq * portTICK_PERIOD_MS / 1000.f)) - acosf(_taskLight.anim.currentDc / 50.f - 1.f) + M_PI;

        if (isnanf(_taskLight.anim.sinInit) != false)
            _taskLight.anim.sinInit = 0.f;
        else if (isinf(_taskLight.anim.sinInit) != false)
            _taskLight.anim.sinInit = 100.f;
    }
    else
    {
        float tick = xTaskGetTickCount();
        _taskLight.anim.sinInit =
            -((tick + (float)(timeToOff / portTICK_PERIOD_MS) * _taskLight.anim.currentDc / 100.f) * (M_TWOPI * _taskLight.anim.sinFreq * portTICK_PERIOD_MS / 1000.f)) - acosf(0.f / 50.f - 1.f) + M_PI;
    }

    _taskLight.anim.func = _taskLightFuncAnimSin;
}

void _taskLightFuncAnimSin()
{
    if (xTaskCheckForTimeOut(&_taskLight.anim.timeOut, &_taskLight.anim.ticksToWait) != pdFALSE)
    {
        // Init time
        _taskLight.anim.ticksToWait = ANIM_STEP_TIME / portTICK_PERIOD_MS;
        vTaskSetTimeOutState(&_taskLight.anim.timeOut);

        float tick = xTaskGetTickCount();
        _taskLight.anim.currentDc =
            50.f * (1.f - cosf(_taskLight.anim.sinInit + tick * (M_TWOPI * _taskLight.anim.sinFreq * portTICK_PERIOD_MS / 1000.f)));
        boardSetLightDc(_taskLight.anim.currentDc);
    }
}

void _taskLightInitAnimBlink(unsigned int timeOn,
                             unsigned int timeOff)
{
    // Init blick animation
    _taskLight.anim.blinkTicksOn = timeOn / portTICK_PERIOD_MS;
    _taskLight.anim.blinkTicksOff = timeOff / portTICK_PERIOD_MS;

    // Init time
    _taskLight.anim.ticksToWait = _taskLight.anim.blinkTicksOff;
    vTaskSetTimeOutState(&_taskLight.anim.timeOut);

    _taskLight.anim.blinkOn = false;
    _taskLight.anim.func = _taskLightFuncAnimBlink;
}

void _taskLightFuncAnimBlink()
{
    if (xTaskCheckForTimeOut(&_taskLight.anim.timeOut, &_taskLight.anim.ticksToWait) != pdFALSE)
    {
        // Init time
        if (_taskLight.anim.blinkOn == true)
        {
            _taskLight.anim.blinkOn = false;
            _taskLight.anim.ticksToWait = _taskLight.anim.blinkTicksOff;
            _taskLight.anim.currentDc = 0.f;
            boardSetLightDc(_taskLight.anim.currentDc);
        }
        else
        {
            _taskLight.anim.blinkOn = true;
            _taskLight.anim.ticksToWait = _taskLight.anim.blinkTicksOn;
            _taskLight.anim.currentDc = 100.f;
            boardSetLightDc(_taskLight.anim.currentDc);
        }
        vTaskSetTimeOutState(&_taskLight.anim.timeOut);
    }
}

TickType_t _taskLightAnim()
{
    if (_taskLight.anim.func == NULL)
        return portMAX_DELAY;

    if (_taskLight.anim.switchToOff == false)
        _taskLight.anim.func();
    // Switch to off
    else if (xTaskCheckForTimeOut(&_taskLight.anim.timeOut, &_taskLight.anim.ticksToWait) != pdFALSE)
    {
        // Init time
        _taskLight.anim.ticksToWait = ANIM_STEP_TIME / portTICK_PERIOD_MS;
        vTaskSetTimeOutState(&_taskLight.anim.timeOut);

        _taskLight.anim.currentDc -= _taskLight.anim.switchOffDecDc;
        boardSetLightDc(_taskLight.anim.currentDc);

        // Set finale color
        if (_taskLight.anim.currentDc <= 0.f)
        {
            _taskLight.anim.currentDc = 0.f;
            _taskLight.anim.switchToOff = false;
            boardSetLightColor(_taskLight.anim.color);
        }
    }

    return _taskLight.anim.ticksToWait;
}

void _taskLightInitAnim(unsigned int timeToOff, color_t color)
{
    // Init time off animation
    if (timeToOff == 0)
        _taskLight.anim.switchOffDecDc = 100.f;
    else
        _taskLight.anim.switchOffDecDc = 100.f / ((float)timeToOff / (float)ANIM_STEP_TIME);

    _taskLight.anim.switchToOff = (COLOR_OFF != _taskLight.anim.color) &&
                                  (_taskLight.anim.color != color);

    // Init time
    _taskLight.anim.ticksToWait = 0;
    vTaskSetTimeOutState(&_taskLight.anim.timeOut);

    // Set finale color
    if ((_taskLight.anim.switchToOff == false) &&
        (_taskLight.anim.color != color))
    {
        _taskLight.anim.currentDc = 0.f;
        boardSetLightDc(0.f);
        boardSetLightColor(color);
    }

    _taskLight.anim.color = color;
}

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

// Include ---------------------------------------------------------------------
#include "taskApp.h"
#include "board.h"
#include "tasks/taskLight/taskLight.h"
#include "tasks/taskBle/taskBle.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

// Define ----------------------------------------------------------------------
#define _TASK_APP_EVENT_QUEUE_LENGTH 8
#define _TASK_APP_DEFAULT_BRIGHTNESS_THRESHOLD 50.f // 50%

// Enum ------------------------------------------------------------------------
typedef enum
{
    APP_STATUS_BLE_ERROR,    //!< Indicates a ble radio error.
    APP_STATUS_BONDING,      //!< Indicates that the device is bonding.
    APP_STATUS_DISCONNECTED, //!< Indicates that the device is disconnected.
    APP_STATUS_CONNECTED,    //!< Indicates that the device is connected.
    APP_STATUS_UNLOCKED      //!< Indicates that the device is unlocked.
} taskAppStatus_t;

typedef enum
{
    APP_EVENT_BLE_ERR,
    APP_EVENT_BLE_CONNECTED,
    APP_EVENT_BLE_DISCONNECTED,
    APP_EVENT_BLE_UNLOCK,
    APP_EVENT_BLE_LOCK,
    APP_EVENT_BLE_OPEN,
    APP_EVENT_BUTTON_BOND,
    APP_EVENT_DOOR_STATE
} taskAppEventType_t;

// Struct ----------------------------------------------------------------------
typedef struct
{
    taskAppEventType_t type;
} taskAppEvent_t;

typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventStaticQueue;
    uint8_t eventQueueStorageArea[sizeof(taskAppEvent_t) * _TASK_APP_EVENT_QUEUE_LENGTH];

    // App status
    taskAppStatus_t status;

    // App conf
    float brightnessTh;
} taskApp_t;

// Global variables ------------------------------------------------------------
static taskApp_t _taskApp = {0};

// Private prototype functions -------------------------------------------------
void _taskAppEventBleErrHandle();
void _taskAppEventBleDisconnectedHandle();
void _taskAppEventBleConnectedHandle();
void _taskAppEventBleUnlockHandle();
void _taskAppEventBleLockHandle();
void _taskAppEventBleOpenHandle();
void _taskAppEventDoorStateHandle();

// Implemented functions -------------------------------------------------------
void taskAppCodeInit()
{
    _taskApp.eventQueue = xQueueCreateStatic(_TASK_APP_EVENT_QUEUE_LENGTH,
                                             sizeof(taskAppEvent_t),
                                             _taskApp.eventQueueStorageArea,
                                             &_taskApp.eventStaticQueue);

    _taskApp.status = APP_STATUS_DISCONNECTED;

    // Todo: Lir la valeur à partie de la flash
    _taskApp.brightnessTh = _TASK_APP_DEFAULT_BRIGHTNESS_THRESHOLD;
}

void taskAppCode(__attribute__((unused)) void *parameters)
{
    taskAppEvent_t event;

    // Make discoverable Ble
    taskBleEventDiscoverable();

    while (1)
    {
        xQueueReceive(_taskApp.eventQueue, &event, portMAX_DELAY);

        switch (event.type)
        {
        case APP_EVENT_BLE_ERR:
            _taskAppEventBleErrHandle();
            break;

        case APP_EVENT_BLE_CONNECTED:
            _taskAppEventBleConnectedHandle();
            break;

        case APP_EVENT_BLE_DISCONNECTED:
            _taskAppEventBleDisconnectedHandle();
            break;

        case APP_EVENT_BLE_UNLOCK:
            _taskAppEventBleUnlockHandle();
            break;

        case APP_EVENT_BLE_LOCK:
            _taskAppEventBleLockHandle();
            break;

        case APP_EVENT_BLE_OPEN:
            _taskAppEventBleOpenHandle();
            break;

        case APP_EVENT_DOOR_STATE:
            _taskAppEventDoorStateHandle();
            break;

        default:
            break;
        }
    }
}

float taskAppGetBrightnessTh()
{
    return _taskApp.brightnessTh;
}

void taskAppSetBrightnessTh(float th)
{
    _taskApp.brightnessTh = th;
    // Todo: notifier tashApp pour sauvegarder la nouvelle valeur dans la flash.
}

// Handle event implemented fonction
void _taskAppEventBleErrHandle()
{
    boardPrintf("App: ble radio error !\r\n");

    boardLedOn();
    taskLightSetColor(COLOR_RED, 0);

    _taskApp.status = APP_STATUS_BLE_ERROR;
}

void _taskAppEventBleDisconnectedHandle()
{
    boardPrintf("App: device disconnected.\r\n");
    boardLock();

    if (boardIsOpen() == true)
    {
        // Ble device is disconnected but the door is open.
        // Turns on the red light to try to warn the user.
        taskLightSetColor(COLOR_RED, 0);

        // Todo: étendre la lumière rouge dans 15min s'il n'y à pas de nouveau
        // événement.
    }
    else
    {
        taskLightSetColor(COLOR_OFF, 0);
        taskBleEventDiscoverable();
    }

    _taskApp.status = APP_STATUS_DISCONNECTED;
}

void _taskAppEventBleConnectedHandle()
{
    boardPrintf("App: device connected.\r\n");

    if (boardIsOpen() == true)
    {
        // turn on the exterior lighting if it's too dark
        if (boardGetBrightness() <= _taskApp.brightnessTh)
            taskLightSetColor(COLOR_WHITE, 0);
        else
            taskLightSetColor(COLOR_OFF, 0); // Todo allumer la bend LED en blanc mais pas l'éclairage
    }
    else
    {
        taskLightSetColor(COLOR_GREEN, 0);
    }

    _taskApp.status = APP_STATUS_CONNECTED;
}

void _taskAppEventBleUnlockHandle()
{
    boardPrintf("App: unlock the lock.\r\n");
    boardUnlock();
    taskLightSetColor(COLOR_BLUE, 0);
    _taskApp.status = APP_STATUS_UNLOCKED;
}

void _taskAppEventBleLockHandle()
{
    boardPrintf("App: Lock the lock.\r\n");
    boardLock();

    if (boardIsOpen() == true)
    {
        // turn on the exterior lighting if it's too dark
        if (boardGetBrightness() <= _taskApp.brightnessTh)
            taskLightSetColor(COLOR_WHITE, 0);
        else
            taskLightSetColor(COLOR_OFF, 0); // Todo allumer la bend LED en blanc mais pas l'éclairage
    }
    else
    {
        taskLightSetColor(COLOR_GREEN, 0);
    }

    _taskApp.status = APP_STATUS_CONNECTED;
}

void _taskAppEventBleOpenHandle()
{
    if (boardIsLocked() == true)
    {
        boardPrintf("App: the lock is loked, can't open.\r\n");
        return;
    }

    boardPrintf("App: open the lock.\r\n");
    boardOpen();
}

void _taskAppEventDoorStateHandle()
{
    taskBleEventDoorState();

    if (boardIsOpen() == true)
    {
        boardPrintf("App: door is open.\r\n");

        switch (_taskApp.status)
        {
        case APP_STATUS_DISCONNECTED:
            taskLightSetColor(COLOR_RED, 0);
            break;

        case APP_STATUS_CONNECTED:
        case APP_STATUS_UNLOCKED:
        {
            // turn on the exterior lighting if it's too dark
            if (boardGetBrightness() <= _taskApp.brightnessTh)
                taskLightSetColor(COLOR_WHITE, 0);
            else
                taskLightSetColor(COLOR_OFF, 0); // Todo allumer la bend LED en blanc mais pas l'éclairage
            break;
        }

        default:
            break;
        }
    }
    else
    {
        boardPrintf("App: door is close.\r\n");

        switch (_taskApp.status)
        {
        case APP_STATUS_DISCONNECTED:
            taskLightSetColor(COLOR_OFF, 0);
            taskBleEventDiscoverable();
            break;

        case APP_STATUS_CONNECTED:
            taskLightSetColor(COLOR_GREEN, 0);
            break;

        case APP_STATUS_UNLOCKED:
            taskLightSetColor(COLOR_BLUE, 0);
            break;

        default:
            break;
        }
    }
}

// Send event implemented fonction
void taskAppEventBondFromISR(BaseType_t *pxHigherPriorityTaskWoken)
{
    if (_taskApp.eventQueue != NULL)
    {
        taskAppEvent_t event = {
            .type = APP_EVENT_BUTTON_BOND};
        xQueueSendFromISR(_taskApp.eventQueue, &event, pxHigherPriorityTaskWoken);
    }
}

void taskAppEventDoorStateFromISR(BaseType_t *pxHigherPriorityTaskWoken)
{
    if (_taskApp.eventQueue != NULL)
    {
        taskAppEvent_t event = {
            .type = APP_EVENT_DOOR_STATE};
        xQueueSendFromISR(_taskApp.eventQueue, &event, pxHigherPriorityTaskWoken);
    }
}

void taskAppEventBleErr()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_ERR};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleConnected()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_CONNECTED};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleDisconnected()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_DISCONNECTED};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleUnlock()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_UNLOCK};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

void taskAppEventBleOpen()
{
    taskAppEvent_t event = {
        .type = APP_EVENT_BLE_OPEN};
    xQueueSend(_taskApp.eventQueue, &event, portMAX_DELAY);
}

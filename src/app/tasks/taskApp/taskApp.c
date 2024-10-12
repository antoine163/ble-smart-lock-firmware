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
#include "BlueNRG1_flash.h"
#include "tasks/taskBle/taskBle.h"
#include "tasks/taskLight/taskLight.h"

#include <string.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

// Define ----------------------------------------------------------------------
#define _TASK_APP_FLASH_ERASE_GUARD_TIME 25 // 25 ms
#define _TASK_APP_FLASH_WRITE_GUARD_TIME 1  // 1 ms

#define _TASK_APP_RESTART_DELAY_TICK      (1 * 60 * 1000 / portTICK_PERIOD_MS)  //!< Tick to wait before restarting following error detection: 1min
#define _TASK_APP_OFF_LIGHT_DELAY_TICK    (15 * 60 * 1000 / portTICK_PERIOD_MS) //!< Tick to wait before torn off light following disconnection: 15min
#define _TASK_APP_CLEAR_BONDED_DELAY_TICK (3 * 1000 / portTICK_PERIOD_MS)       //!< Tick to wait before clear all bonded devices from push bond button: 3s
#define _TASK_APP_EXIT_BOND_DELAY_TICK    (10 * 1000 / portTICK_PERIOD_MS)      //!< Tick to wait before exit the bond mode from active it: 10s

#define _TASK_APP_EVENT_QUEUE_LENGTH 8
#define _TASK_APP_DATA_STORAGE_PAGE  (N_PAGES - 3)

#define _TASK_APP_CHECK_VERBOSE(verbose)  ((verbose == true) || (verbose == false))
#define _TASK_APP_CHECK_PIN(pin)          (pin <= 999999)
#define _TASK_APP_CHECK_BRIGHTNESS_TH(th) ((th >= 0.) && (th <= 100.))

#define _TASK_APP_MIN(v1, v2) (v1 < v2) ? v1 : v2

#define _TASK_APP_DEFAULT_FIX_PIN 215426

// Flag tools
#define _TASK_APP_FLAG_SET(name_flag) \
    _taskApp.flags |= _TASK_APP_FLAG_##name_flag;

#define _TASK_APP_FLAG_CLEAR(name_flag) \
    _taskApp.flags &= (~_TASK_APP_FLAG_##name_flag);

#define _TASK_APP_FLAG_IS(name_flag) \
    ((_taskApp.flags & _TASK_APP_FLAG_##name_flag) != 0)

#define _TASK_APP_LAST_FLAG_IS(name_flag) \
    ((_taskApp.lastFlags & _TASK_APP_FLAG_##name_flag) != 0)

// Enum ------------------------------------------------------------------------

typedef enum
{
    _TASK_APP_FLAG_BLE_NONE = 0x00,
    _TASK_APP_FLAG_BLE_ERROR = 0x01,
    _TASK_APP_FLAG_BONDING = 0x02,
    _TASK_APP_FLAG_UNLOCKED = 0x04,
    _TASK_APP_FLAG_OPENED = 0x08,
    _TASK_APP_FLAG_CONNECTED = 0x10
} taskAppFlags_t;

/**
 * @brief Enum representing the different events for the application task.
 */
typedef enum
{
    _TASK_APP_EVENT_BOARD,    /**< Event related to the board. */
    _TASK_APP_EVENT_BLE,      /**< Event related to the BLE. */
    _TASK_APP_EVENT_WRITE_NVM /**< Event for writing to non-volatile memory. */
} taskAppEvent_t;

// Struct ----------------------------------------------------------------------

/**
 * @brief Struct representing the non-volatile memory data for the application task.
 */
typedef struct
{
    bool verbose;       /**< Verbose mode flag. */
    uint32_t pin;       /**< PIN number. */
    float brightnessTh; /**< Brightness threshold value. */
} taskAppNvmData_t;

/**
 * @brief Struct representing an item of an application task event.
 */
typedef struct
{
    taskAppEvent_t event; /**< Type of the event. */
    union
    {
        boardEvent_t boardEvent;     /**< Event related to the board. */
        bleEvent_t bleEvent;         /**< Event related to the board. */
        taskAppNvmData_t nvmNewData; /**< New non-volatile memory data. */
    };
} taskAppEventItem_t;

typedef struct
{
    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventQueueBuffer;
    uint8_t eventQueueStorageArea[sizeof(taskAppEventItem_t) * _TASK_APP_EVENT_QUEUE_LENGTH];

    // App flags
    taskAppFlags_t flags;
    taskAppFlags_t lastFlags;

    // Timeout to manage reset follows an error.
    TickType_t ticksToRestart;
    TimeOut_t timeOutRestart;

    // Timeout to manage light off follows an disconnection.
    TickType_t ticksToOffLight;
    TimeOut_t timeOutOffLight;

    // Timeout to manage clean all bonded device follows pushed bond button.
    TickType_t ticksToClearBonded;
    TimeOut_t timeOutClearBonded;
    bool clearBondedLightFlash;

    // Timeout to manage the exit from bond mode after activating it.
    TickType_t ticksToExitBond;
    TimeOut_t timeOutExitBond;
} taskApp_t;

// Global variables ------------------------------------------------------------
NO_INIT_SECTION(const volatile taskAppNvmData_t _taskAppNvmData, ".noinit.app_flash_data");
static taskApp_t _taskApp = {0};
static const taskAppNvmData_t _taskAppNvmDefaultData = {
    .verbose = false,
    .pin = _TASK_APP_DEFAULT_FIX_PIN,
    .brightnessTh = 50.f};

// Private prototype functions -------------------------------------------------
void _taskAppUpdateLight();
void _taskAppSetLightOn();

void _taskAppBleEventErrHandle();
void _taskAppBleEventDisconnectedHandle();
void _taskAppBleEventConnectedHandle();

void _taskAppBoardEventDoorStateHandle();
void _taskAppBoardEventButtonBondStateHandle();

int _taskAppNvmWrite(const taskAppNvmData_t *newData);
int _taskAppNvmInit();

// Implemented functions -------------------------------------------------------
void taskAppCodeInit()
{
    _taskApp.eventQueue = xQueueCreateStatic(_TASK_APP_EVENT_QUEUE_LENGTH,
                                             sizeof(taskAppEventItem_t),
                                             _taskApp.eventQueueStorageArea,
                                             &_taskApp.eventQueueBuffer);

    _taskApp.flags = _TASK_APP_FLAG_BLE_NONE;
    _taskApp.lastFlags = _TASK_APP_FLAG_BLE_NONE;
    _taskApp.ticksToRestart = portMAX_DELAY;
    _taskApp.ticksToOffLight = portMAX_DELAY;
    _taskApp.ticksToClearBonded = portMAX_DELAY;
    _taskApp.ticksToExitBond = portMAX_DELAY;

    _taskAppNvmInit();

    // Enable or not debug log
    boardDgbEnable(taskAppGetVerbose());
}

void taskAppCode(__attribute__((unused)) void *parameters)
{
    taskAppEventItem_t eventItem;

    // Set brightens threshold BLE attribut
    taskBleUpdateAtt(BLE_ATT_BRIGHTNESS_TH,
                     (const void *)&_taskAppNvmData.brightnessTh,
                     sizeof(_taskAppNvmData.brightnessTh));

    // Enable level irruption for open/close door state
    boardOpenItSetLevel( true );
    
    while (1)
    {
        // Get next timeout
        TickType_t ticksToWait = portMAX_DELAY;
        ticksToWait = _TASK_APP_MIN(ticksToWait, _taskApp.ticksToRestart);
        ticksToWait = _TASK_APP_MIN(ticksToWait, _taskApp.ticksToOffLight);
        ticksToWait = _TASK_APP_MIN(ticksToWait, _taskApp.ticksToClearBonded);
        ticksToWait = _TASK_APP_MIN(ticksToWait, _taskApp.ticksToExitBond);

        if (xQueueReceive(_taskApp.eventQueue, &eventItem, ticksToWait) == pdPASS)
        {
            switch (eventItem.event)
            {
            case _TASK_APP_EVENT_BOARD:
            {
                switch (eventItem.boardEvent)
                {
                case BOARD_EVENT_DOOR_STATE:        _taskAppBoardEventDoorStateHandle(); break;
                case BOARD_EVENT_BUTTON_BOND_STATE: _taskAppBoardEventButtonBondStateHandle(); break;
                }
                break;
            }

            case _TASK_APP_EVENT_BLE:
            {
                switch (eventItem.bleEvent)
                {
                case BLE_EVENT_ERR:          _taskAppBleEventErrHandle(); break;
                case BLE_EVENT_CONNECTED:    _taskAppBleEventConnectedHandle(); break;
                case BLE_EVENT_DISCONNECTED: _taskAppBleEventDisconnectedHandle(); break;
                }
                break;
            }

            case _TASK_APP_EVENT_WRITE_NVM:
            {
                int n = _taskAppNvmWrite(&eventItem.nvmNewData);
                if (n == 0)
                    boardDgb("App: NVM memory written!\r\n");
                else
                    boardDgb("App: NVM memory writ fail!\r\n");

                break;
            }
            }
        }

        // Manage timeout

        if ((_taskApp.ticksToRestart != portMAX_DELAY) &&
            (xTaskCheckForTimeOut(&_taskApp.timeOutRestart, &_taskApp.ticksToRestart) != pdFALSE))
        {
            boardReset();
        }

        if ((_taskApp.ticksToOffLight != portMAX_DELAY) &&
            (xTaskCheckForTimeOut(&_taskApp.timeOutOffLight, &_taskApp.ticksToOffLight) != pdFALSE))
        {
            _taskApp.ticksToOffLight = portMAX_DELAY;
            taskLightAnimTrans(0, COLOR_OFF, 0);
        }

        if ((_taskApp.ticksToClearBonded != portMAX_DELAY) &&
            (xTaskCheckForTimeOut(&_taskApp.timeOutClearBonded, &_taskApp.ticksToClearBonded) != pdFALSE))
        {
            _taskApp.ticksToClearBonded = portMAX_DELAY;

            if (_taskApp.clearBondedLightFlash == false)
            {
                taskBleClearAllPairing();
                taskLightAnimBlink(0, COLOR_WHITE, 80, 120);

                // Wait tow flash befor restore light state
                _taskApp.ticksToClearBonded = 600 / portTICK_PERIOD_MS;
                vTaskSetTimeOutState(&_taskApp.timeOutClearBonded);
                _taskApp.clearBondedLightFlash = true;
            }
            else
            {
                _taskApp.clearBondedLightFlash = false;

                // Restart following a whitelist cleanup.
                boardReset();
            }
        }

        if ((_taskApp.ticksToExitBond != portMAX_DELAY) &&
            (xTaskCheckForTimeOut(&_taskApp.timeOutExitBond, &_taskApp.ticksToExitBond) != pdFALSE))
        {
            // Disable exit bond timeout
            _taskApp.ticksToExitBond = portMAX_DELAY;

            taskBleSetBondMode(false);
            _TASK_APP_FLAG_CLEAR(BONDING);
            _taskAppUpdateLight();
        }
    }
}

int taskAppEnableVerbose(bool enable)
{
    if (!_TASK_APP_CHECK_VERBOSE(enable))
        return -1;

    if (enable == _taskAppNvmData.verbose)
        return 0;

    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmData,
           sizeof(taskAppNvmData_t));
    eventItem.nvmNewData.verbose = enable;

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);

    return 0;
}

int taskAppResetConfig()
{
    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmDefaultData,
           sizeof(taskAppNvmData_t));

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);
    return 0;
}

float taskAppGetBrightnessTh()
{
    return _taskAppNvmData.brightnessTh;
}

int taskAppSetBrightnessTh(float th)
{
    if (!_TASK_APP_CHECK_BRIGHTNESS_TH(th))
        return -1;

    if (th == _taskAppNvmData.brightnessTh)
        return 0;

    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmData,
           sizeof(taskAppNvmData_t));
    eventItem.nvmNewData.brightnessTh = th;

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);

    if (taskBleIsCurrent() == false)
        taskBleUpdateAtt(BLE_ATT_BRIGHTNESS_TH, &th, sizeof(th));

    return 0;
}

uint32_t taskAppGetPin()
{
    return _taskAppNvmData.pin;
}

int taskAppSetPin(uint32_t pin)
{
    if (!_TASK_APP_CHECK_PIN(pin))
        return -1;

    if (pin == _taskAppNvmData.pin)
        return 0;

    if (taskBleSetPin(pin) != 0)
        return -1;

    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmData,
           sizeof(taskAppNvmData_t));
    eventItem.nvmNewData.pin = pin;

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);

    return 0;
}

bool taskAppGetVerbose()
{
    return _taskAppNvmData.verbose;
}

int taskAppSetVerbose(bool verbose)
{
    if (!_TASK_APP_CHECK_VERBOSE(verbose))
        return -1;

    if (verbose == _taskAppNvmData.verbose)
        return 0;

    taskAppEventItem_t eventItem = {.event = _TASK_APP_EVENT_WRITE_NVM};
    memcpy((void *)&eventItem.nvmNewData,
           (const void *)&_taskAppNvmData,
           sizeof(taskAppNvmData_t));
    eventItem.nvmNewData.verbose = verbose;

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);

    boardDgbEnable(verbose);
    return 0;
}

void taskAppUnlock()
{
    if _TASK_APP_FLAG_IS (CONNECTED)
    {
        boardDgb("App: unlock the lock.\r\n");
        boardUnlock();

        _TASK_APP_FLAG_SET(UNLOCKED);
        _taskAppUpdateLight();
    }
    else
        boardDgb("App: Can't unlock if unconnected device.\r\n");
}

void taskAppOpenDoor()
{
    if (boardIsLocked() == true)
    {
        boardDgb("App: the lock is loked, can't open.\r\n");
        return;
    }

    boardDgb("App: open the Door.\r\n");
    boardOpen();
}

void _taskAppUpdateLight()
{
    // Disable turn off the light in 15 minutes if there are no new events.
    _taskApp.ticksToOffLight = portMAX_DELAY;

    // ERROR ?
    if _TASK_APP_FLAG_IS (BLE_ERROR)
    {
        taskLightAnimTrans(0, COLOR_RED, 0);
        // Note: the device will be reset in 1min
    }
    // Bond mode ?
    else if _TASK_APP_FLAG_IS (BONDING)
    {
        if (!_TASK_APP_LAST_FLAG_IS(BONDING))
            taskLightAnimSin(200, COLOR_GREEN, 1);
    }
    // Connected ?
    else if _TASK_APP_FLAG_IS (CONNECTED)
    {
        if _TASK_APP_FLAG_IS (OPENED)
            _taskAppSetLightOn();
        else if _TASK_APP_FLAG_IS (UNLOCKED)
            taskLightAnimTrans(200, COLOR_BLUE, 500);
        else
            taskLightAnimSin(200, COLOR_BLUE, 0.2f);
    }
    // Disconnected ?
    else
    {
        if _TASK_APP_FLAG_IS (OPENED)
        {
            // Last flag is connected ?
            if _TASK_APP_LAST_FLAG_IS (CONNECTED)
            {
                // Ble device is disconnected but the door is open.
                // Turns on the red light to try to warn the user.
                taskLightAnimBlink(0, COLOR_RED, 100, 500);
            }
            else
            {
                // Here, the door was opened with the key.
                _taskAppSetLightOn();
            }

            // Turn off the light in 15 minutes if there are no new events.
            _taskApp.ticksToOffLight = _TASK_APP_OFF_LIGHT_DELAY_TICK;
            vTaskSetTimeOutState(&_taskApp.timeOutOffLight);
        }
        else if _TASK_APP_LAST_FLAG_IS (CONNECTED)
            taskLightAnimTrans(4000, COLOR_OFF, 0);
        else
            taskLightAnimTrans(200, COLOR_OFF, 0);
    }

    _taskApp.lastFlags = _taskApp.flags;
}

void _taskAppSetLightOn()
{
    if (boardGetBrightness() <= _taskAppNvmData.brightnessTh)
        taskLightAnimTrans(200, COLOR_WHITE_LIGHT, 200);
    else
        taskLightAnimTrans(200, COLOR_YELLOW, 200);
}

// Handle event implemented fonction
void _taskAppBleEventErrHandle()
{
    boardDgb("App: ble radio error !\r\n");
    boardLedOn();

    // Init time to restart into 1min
    _taskApp.ticksToRestart = _TASK_APP_RESTART_DELAY_TICK;
    vTaskSetTimeOutState(&_taskApp.timeOutRestart);

    _TASK_APP_FLAG_SET(BLE_ERROR);
    _taskAppUpdateLight();
}

void _taskAppBleEventDisconnectedHandle()
{
    boardDgb("App: device disconnected.\r\n");
    boardLock();

    _TASK_APP_FLAG_CLEAR(UNLOCKED);
    _TASK_APP_FLAG_CLEAR(CONNECTED);
    _taskAppUpdateLight();
}

void _taskAppBleEventConnectedHandle()
{
    boardDgb("App: device connected.\r\n");

    // Disable exit bond timeout
    // If we are connected, we are definitely not in bond mode.
    _taskApp.ticksToExitBond = portMAX_DELAY;

    _TASK_APP_FLAG_SET(CONNECTED);
    _TASK_APP_FLAG_CLEAR(BONDING);
    _taskAppUpdateLight();
}

void _taskAppBoardEventDoorStateHandle()
{
    if (boardIsOpen() == true)
    {
        boardDgb("App: door is open.\r\n");

        // Update BLE characteristic
        uint8_t state = 1;
        taskBleUpdateAtt(BLE_ATT_DOOR_STATE, &state, 1);
        _TASK_APP_FLAG_SET(OPENED);
        _taskAppUpdateLight();
    }
    else
    {
        boardDgb("App: door is close.\r\n");

        // Update BLE characteristic
        uint8_t state = 0;
        taskBleUpdateAtt(BLE_ATT_DOOR_STATE, &state, 1);
        _TASK_APP_FLAG_CLEAR(OPENED);
        _taskAppUpdateLight();
    }
}

void _taskAppBoardEventButtonBondStateHandle()
{
    if (boardButtonBondState() == true)
    {
        if _TASK_APP_FLAG_IS (BONDING)
        {
            // Disable exit bond timeout
            _taskApp.ticksToExitBond = portMAX_DELAY;

            taskBleSetBondMode(false);
            _TASK_APP_FLAG_CLEAR(BONDING);
            _taskAppUpdateLight();
        }
        else
        {
            // Enable timeout to clear all bonded devices if the button is not
            // release during 3s.
            _taskApp.ticksToClearBonded = _TASK_APP_CLEAR_BONDED_DELAY_TICK;
            vTaskSetTimeOutState(&_taskApp.timeOutClearBonded);
        }
    }
    else
    {
        if (_taskApp.clearBondedLightFlash == false)
        {
            // The bond button is released, and if the timeout is not reached and
            // the pairing mode is not active, we activate it.
            if ((_taskApp.ticksToClearBonded != portMAX_DELAY) &&
                !_TASK_APP_FLAG_IS(BONDING))
            {
                taskBleSetBondMode(true);
                _TASK_APP_FLAG_SET(BONDING);
                _taskAppUpdateLight();

                // Enable timeout to exit bond mode in 10s.
                _taskApp.ticksToExitBond = _TASK_APP_EXIT_BOND_DELAY_TICK;
                vTaskSetTimeOutState(&_taskApp.timeOutExitBond);
            }

            // Disable timeout to clear all bonded devices.
            _taskApp.ticksToClearBonded = portMAX_DELAY;
        }
    }
}

// Attention, ensure that the radio is not currently executing and will not be
// during the write operation before calling this function.
int _taskAppNvmWrite(const taskAppNvmData_t *newData)
{
    if (newData == NULL)
        return 0;

    // Wait a sufficiently long time before the next radio activity to proceed
    // with the erase operation.
    while (taskBleNextRadioTime_ms() < _TASK_APP_FLASH_ERASE_GUARD_TIME)
        taskYIELD();
    taskBlePauseRadio();

    // Erase the DATA_STORAGE_PAGE before erase operation
    FLASH_ErasePage(_TASK_APP_DATA_STORAGE_PAGE);

    // Wait for the end of erase operation
    while (FLASH_GetFlagStatus(Flash_CMDDONE) != SET)
        taskYIELD();

    // Can resume the radio at the end of the erase operation.
    taskBleResumeRadio();

    // Program all words of taskAppNvmData_t
    for (size_t i = 0; i < sizeof(taskAppNvmData_t); i += N_BYTES_WORD)
    {
        // Wait a sufficiently long time before the next radio activity to proceed
        // with the write operation.
        while (taskBleNextRadioTime_ms() < _TASK_APP_FLASH_WRITE_GUARD_TIME)
            taskYIELD();
        taskBlePauseRadio();

        // Program the word
        uint32_t word = ((const uint32_t *)newData)[i / N_BYTES_WORD];
        FLASH_ProgramWord(((uint32_t)&_taskAppNvmData) + i, word);

        // Wait for the end of write operation
        while (FLASH_GetFlagStatus(Flash_CMDDONE) != SET)
            taskYIELD();

        // Can resume the radio at the end of the write operation.
        taskBleResumeRadio();
    }

    return 0;
}

int _taskAppNvmInit()
{
    // Check possible value
    if (!_TASK_APP_CHECK_VERBOSE(_taskAppNvmData.verbose) ||
        !_TASK_APP_CHECK_PIN(_taskAppNvmData.pin) ||
        !_TASK_APP_CHECK_BRIGHTNESS_TH(_taskAppNvmData.brightnessTh))
    {
        if (_taskAppNvmWrite(&_taskAppNvmDefaultData) == 0)
            boardDgb("App: NVM memory written with default values!\r\n");
        else
            boardDgb("App NVM memory writ fail with default values!\r\n");
    }

    return 0;
}

// Send event implemented fonction
void boardSendEventFromISR(boardEvent_t event,
                           BaseType_t *pxHigherPriorityTaskWoken)
{
    if (_taskApp.eventQueue != NULL)
    {
        taskAppEventItem_t eventItem = {
            .event = _TASK_APP_EVENT_BOARD,
            .boardEvent = event};
        xQueueSendFromISR(_taskApp.eventQueue, &eventItem,
                          pxHigherPriorityTaskWoken);
    }
}

// Warning: this function is run in task BLE
void taskBleSendEvent(bleEvent_t event)
{
    taskAppEventItem_t eventItem = {
        .event = _TASK_APP_EVENT_BLE,
        .bleEvent = event};

    xQueueSend(_taskApp.eventQueue, &eventItem, portMAX_DELAY);
}

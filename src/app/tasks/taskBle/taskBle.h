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
 * @file taskBle.h
 * @author antoine163
 * @date 02-04-2024
 * @brief Task of BLE radio
 */

#ifndef TASK_BLE_H
#define TASK_BLE_H

// Include ---------------------------------------------------------------------
#include "bluenrg1_api.h"
#include "sm.h"
#include <stddef.h>

// Enum ------------------------------------------------------------------------

/**
 * @brief Enumeration of BLE attribute types.
 *
 * This enum defines the various BLE attribute types used in the application.
 */
typedef enum
{
    BLE_ATT_LOCK_STATE,   /**< Attribute representing the lock state (lock/unlock)*/
    BLE_ATT_DOOR_STATE,   /**< Attribute representing the door state (open/close) */
    BLE_ATT_OPEN_DOOR,    /**< Attribute representing the action to open the door */
    BLE_ATT_BRIGHTNESS,   /**< Attribute representing the brightness level */
    BLE_ATT_BRIGHTNESS_TH /**< Attribute representing the brightness threshold */
} bleAtt_t;

/**
 * @brief Enumeration of BLE task events.
 *
 * This enum defines the various events that can be sent by the BLE task.
 */
typedef enum
{
    BLE_EVENT_ERR,          /**< Event indicating an error occurred */
    BLE_EVENT_CONNECTED,    /**< Event indicating a successful connection */
    BLE_EVENT_DISCONNECTED, /**< Event indicating a disconnection */
} bleEvent_t;

// Prototype functions ---------------------------------------------------------
void taskBleCodeInit();
void taskBleCode(void *parameters);

bool taskBleIsCurrent();
void taskBlePauseRadio();
void taskBleResumeRadio();
unsigned int taskBleNextRadioTime_ms();
int taskBleSetPin(unsigned int pin);

void taskBleSetBondMode(bool enable);
int taskBleGetBonded(Bonded_Device_Entry_t *bondedDevices);
int taskBleClearAllPairing();
int taskBleUpdateAtt(bleAtt_t att, const void *buf, size_t nbyte);

// Function called by the BLE task to send an event to the App task
// This function is implemented in the App task
void taskBleSendEvent(bleEvent_t event);

#endif // TASK_BLE_H

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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABle: FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file taskBle.c
 * @author antoine163
 * @date 02-04-2024
 * @brief Task of Ble: radio
 */

// Include ---------------------------------------------------------------------
#include "taskBle.h"
#include "bleConfig.h"
#include "board.h"
#include "itConfig.h"
#include "tasks/taskApp/taskApp.h"

#include <string.h>

#include "bluenrg1_gap.h"
#include "bluenrg1_gatt_server.h"
#include "bluenrg1_hal.h"
#include "bluenrg1_stack.h"
#include "sleep.h"
#include "sm.h"

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

// Define ----------------------------------------------------------------------
#define _TASK_BLE_DEFAULT_NAME             "Ble Smart Lock"
#define _TASK_BLE_NAME_MAX_SIZE            16
#define _TASK_BLE_BOND_ADV_INTERVAL_MIN_MS 160
#define _TASK_BLE_BOND_ADV_INTERVAL_MAX_MS 320
#define _TASK_BLE_ADV_INTERVAL_MIN_MS      600
#define _TASK_BLE_ADV_INTERVAL_MAX_MS      900

#define _TASK_BLE_EVENT_QUEUE_LENGTH 8

// Flag tools
#define _TASK_BLE_FLAG_SET(name_flag) \
    _taskBle.flags |= _TASK_BLE_FLAG_##name_flag;

#define _TASK_BLE_FLAG_CLEAR(name_flag) \
    _taskBle.flags &= (~_TASK_BLE_FLAG_##name_flag);

#define _TASK_BLE_FLAG_IS(name_flag) \
    ((_taskBle.flags & _TASK_BLE_FLAG_##name_flag) != 0)

// Enum ------------------------------------------------------------------------
/**
 * @brief Enum representing various BLE task flasg.
 */
typedef enum
{
    _TASK_BLE_FLAG_DO_ADVERTISING = 0x01,         /**< Perform BLE advertising. */
    _TASK_BLE_FLAG_DO_SLAVE_SECURITY_REQ = 0x02,  /**< Perform security request as BLE slave. */
    _TASK_BLE_FLAG_DO_CONFIGURE_WHITELIST = 0x04, /**< Configure BLE whitelist. */
    _TASK_BLE_FLAG_DO_NOTIFY_READ_REQ = 0x08,     /**< Notify or request to read. */
    _TASK_BLE_FLAG_BONDING = 0x40,                /**< Bonding in progress */
    _TASK_BLE_FLAG_CONNECTED = 0x80,              /**< device connected */

} taskBleFlag_t;

/**
 * @brief Enumeration for BLE task events.
 *
 * This enum defines the events specific to BLE tasks.
 */
typedef enum
{
    _TASK_BLE_EVENT_IT /**< BLE interrupt event */
} taskBleEvent_t;

// Struct ----------------------------------------------------------------------

typedef struct
{
    // Handle of this task
    TaskHandle_t taskHandle;

    // Event queue
    QueueHandle_t eventQueue;
    StaticQueue_t eventQueueBuffer;
    uint8_t eventQueueStorageArea[sizeof(taskBleEvent_t) * _TASK_BLE_EVENT_QUEUE_LENGTH];

    // Mutex to protect BLE stack
    SemaphoreHandle_t bleStackMutex;
    StaticSemaphore_t bleStackMutexBuffer;

    // GAP handle
    uint16_t serviceGapHandle;
    uint16_t devNameCharGapHandle;
    uint16_t appearanceCharGapHandle;

    // Ble stack status
    tBleStatus bleStatus;

    // Time of next ble state
    uint32_t nextStateSysTime;

    // Ble handle
    /**
     * @brief Connection handle.
     */
    uint16_t connectionHandle;

    /**
     * @brief Flags
     */
    taskBleFlag_t flags;

    /**
     * @brief Service handle of application.
     */
    uint16_t serviceAppHandle;

    /**
     * @brief Characteristic, state of the lock.
     *
     * This characteristic is write only, its type is uint8_t and can take the
     * following values:
     * - 0x00: Locked
     * - 0x01: Unlocked
     */
    uint16_t lockStateCharAppHandle;

    /**
     * @brief Characteristic, state of the door.
     *
     * This characteristic is read only, its type is uint8_t and can take the
     * following values:
     * - 0x00: Close
     * - 0x01: Open
     */
    uint16_t doorStateCharAppHandle;

    /**
     * @brief Characteristic, open the door electronically.
     *
     * This characteristic is write only, its type is uint8_t and can take the
     * following values:
     * - 0x01: Open the door
     */
    uint16_t openDoorCharAppHandle;

    /**
     * @brief Characteristic, ambient brightness.
     *
     * This characteristic is read only, its type is float and can take the
     * following values:
     * - 0.0% to 100.0%
     */
    uint16_t brightnessCharAppHandle;

    /**
     * @brief Characteristic, brightness threshold.
     *
     * Adjusting the brightness threshold (between day and night).
     *
     * This characteristic is read/write, its type is float and can take the
     * following values:
     * - 0.0% to 100.0%
     */
    uint16_t brightnessThCharAppHandle;
} taskBle_t;

// Global variables ------------------------------------------------------------
static taskBle_t _taskBle = {0};

// Private prototype functions -------------------------------------------------
tBleStatus _taskBleInitDevice();
tBleStatus _taskBleAddServices();
const char *_taskBleStatusToStr(tBleStatus status);

void _taskBleUpdateWhitelist();
void _taskBleMakeDiscoverable(bool bond);
uint16_t _taskBleGetCharHandleFromAtt(bleAtt_t att);
void _taskBleManageFlags();

// Implemented functions -------------------------------------------------------
void taskBleCodeInit()
{
    // Init event queue
    _taskBle.eventQueue = xQueueCreateStatic(_TASK_BLE_EVENT_QUEUE_LENGTH,
                                             sizeof(taskBleEvent_t),
                                             _taskBle.eventQueueStorageArea,
                                             &_taskBle.eventQueueBuffer);

    // Init mutex
    _taskBle.bleStackMutex = xSemaphoreCreateMutexStatic(&_taskBle.bleStackMutexBuffer);

    // Init Ble
    _taskBle.bleStatus = BlueNRG_Stack_Initialization(&BlueNRG_Stack_Init_params);
    if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
        boardDgb("Ble: stack init: %s\r\n",
                 _taskBleStatusToStr(_taskBle.bleStatus));

    if (_taskBle.bleStatus == BLE_STATUS_SUCCESS)
    {
        _taskBle.bleStatus = _taskBleInitDevice();
        if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
            boardDgb("Ble: init device error: %s\r\n",
                     _taskBleStatusToStr(_taskBle.bleStatus));
    }

    if (_taskBle.bleStatus == BLE_STATUS_SUCCESS)
    {
        _taskBle.bleStatus = _taskBleAddServices();
        if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
            boardDgb("Ble: add service error: %s\r\n",
                     _taskBleStatusToStr(_taskBle.bleStatus));
    }

    if (_taskBle.bleStatus == BLE_STATUS_SUCCESS)
        boardDgb("Ble: initialised with success\r\n");
}

// Implemented functions -------------------------------------------------------
void taskBleCode(__attribute__((unused)) void *parameters)
{
    taskBleEvent_t event;
    _taskBle.taskHandle = xTaskGetCurrentTaskHandle();

    if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
        taskBleSendEvent(BLE_EVENT_ERR);

    xSemaphoreTake(_taskBle.bleStackMutex, portMAX_DELAY);
    // Update first the whitelist.
    _taskBleUpdateWhitelist();
    // Make the device discoverable and linkable only to the device whitelist.
    _taskBleMakeDiscoverable(false);
    xSemaphoreGive(_taskBle.bleStackMutex);

    while (1)
    {
        xQueueReceive(_taskBle.eventQueue, &event, portMAX_DELAY);

        xSemaphoreTake(_taskBle.bleStackMutex, portMAX_DELAY);
        switch (event)
        {
        case _TASK_BLE_EVENT_IT:
        {
            boardLedOn();
            while (BlueNRG_Stack_Perform_Deep_Sleep_Check() == SLEEPMODE_RUNNING)
            {
                BTLE_StackTick();
                _taskBleManageFlags();
            }
            boardLedOff();
            break;
        }
        default: break;
        }
        xSemaphoreGive(_taskBle.bleStackMutex);
    }
}

bool taskBleIsCurrent()
{
    TaskHandle_t taskHandle = xTaskGetCurrentTaskHandle();
    return (taskHandle != NULL) && (taskHandle == _taskBle.taskHandle);
}

unsigned int taskBleNextRadioTime_ms()
{
    return HAL_VTimerDiff_ms_sysT32(
        _taskBle.nextStateSysTime,
        HAL_VTimerGetCurrentTime_sysT32());
}

int taskBleSetPin(unsigned int pin)
{
    tBleStatus bleStatus = aci_gap_set_authentication_requirement(
        BONDING,
        MITM_PROTECTION_REQUIRED,
        SC_IS_SUPPORTED,
        KEYPRESS_IS_NOT_SUPPORTED,
        7,  // Minimum encryption key size
        16, // Maximum encryption key size
        USE_FIXED_PIN_FOR_PAIRING,
        pin, // Fixed Pin
        STATIC_RANDOM_ADDR);

    if (bleStatus != BLE_STATUS_SUCCESS)
    {
        boardDgb("Ble: Failed to set PIN: %s: %s\r\n",
                 _taskBleStatusToStr(bleStatus));
        return -1;
    }

    return 0;
}

void taskBleSetBondMode(bool enable)
{
    xSemaphoreTake(_taskBle.bleStackMutex, portMAX_DELAY);
    if (_TASK_BLE_FLAG_IS(BONDING) == !enable)
        _taskBleMakeDiscoverable(enable);
    xSemaphoreGive(_taskBle.bleStackMutex);
}

int taskBleGetBonded(Bonded_Device_Entry_t *bondedDevices)
{
    int ret = -1;
    xSemaphoreTake(_taskBle.bleStackMutex, portMAX_DELAY);

    uint8_t bondedLen = 0;
    tBleStatus bleStatus = aci_gap_get_bonded_devices(&bondedLen, bondedDevices);
    if (bleStatus != BLE_STATUS_SUCCESS)
    {
        boardDgb("Ble: Failed to get bonded device: %s\r\n",
                 _taskBleStatusToStr(bleStatus));
    }
    else
        ret = (int)bondedLen;

    xSemaphoreGive(_taskBle.bleStackMutex);

    return ret;
}

int taskBleClearAllPairing()
{
    int n = -1;

    xSemaphoreTake(_taskBle.bleStackMutex, portMAX_DELAY);
    _taskBle.bleStatus = aci_gap_clear_security_db();
    if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
    {
        boardDgb("Ble: clear security data base error: %s\r\n",
                 _taskBleStatusToStr(_taskBle.bleStatus));
    }
    else
    {
        n = 0;
        boardDgb("Ble: security data base cleared\r\n");

        _taskBleUpdateWhitelist();
        _taskBleMakeDiscoverable(_TASK_BLE_FLAG_IS(BONDING));
    }
    xSemaphoreGive(_taskBle.bleStackMutex);
    return n;
}

int taskBleUpdateAtt(bleAtt_t att, const void *buf, size_t nbyte)
{
    int n = -1;
    uint16_t charHandle = _taskBleGetCharHandleFromAtt(att);

    if (charHandle == 0xffff)
        return -1;
    if (nbyte == 0)
        return 0;

    xSemaphoreTake(_taskBle.bleStackMutex, portMAX_DELAY);
    _taskBle.bleStatus = aci_gatt_update_char_value_ext(
        _taskBle.connectionHandle,
        _taskBle.serviceAppHandle,
        charHandle,
        1,     /* Update_Type: (1)GATT_NOTIFICATION */
        nbyte, /* Char_Length */
        0,     /* Value_Offset */
        nbyte, /* Value_Length */
        (uint8_t *)buf);

    if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
    {
        boardDgb("Ble: update char value error: %s\r\n",
                 _taskBleStatusToStr(_taskBle.bleStatus));
    }
    else
        n = nbyte;
    xSemaphoreGive(_taskBle.bleStackMutex);

    return n;
}

void taskBlePauseRadio()
{
    if (_taskBle.bleStackMutex != NULL)
        xSemaphoreTake(_taskBle.bleStackMutex, portMAX_DELAY);
}

void taskBleResumeRadio()
{
    if (_taskBle.bleStackMutex != NULL)
        xSemaphoreGive(_taskBle.bleStackMutex);
}

void _taskBleManageFlags()
{
    if _TASK_BLE_FLAG_IS (DO_SLAVE_SECURITY_REQ)
    {
        _TASK_BLE_FLAG_CLEAR(DO_SLAVE_SECURITY_REQ);

        _taskBle.bleStatus = aci_gap_slave_security_req(
            _taskBle.connectionHandle);
        if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
        {
            boardDgb("Ble: slave security request error: %s\r\n",
                     _taskBleStatusToStr(_taskBle.bleStatus));
            taskBleSendEvent(BLE_EVENT_ERR);
        }
    }

    if _TASK_BLE_FLAG_IS (DO_CONFIGURE_WHITELIST)
    {
        _TASK_BLE_FLAG_CLEAR(DO_CONFIGURE_WHITELIST);
        _taskBleUpdateWhitelist();
    }

    if _TASK_BLE_FLAG_IS (DO_ADVERTISING)
    {
        _TASK_BLE_FLAG_CLEAR(DO_ADVERTISING);
        _taskBleMakeDiscoverable(_TASK_BLE_FLAG_IS(BONDING));
    }

    if _TASK_BLE_FLAG_IS (DO_NOTIFY_READ_REQ)
    {
        _TASK_BLE_FLAG_CLEAR(DO_NOTIFY_READ_REQ);

        // Update brightness befor read
        float brightness = boardGetBrightness();
        aci_gatt_update_char_value(
            _taskBle.serviceAppHandle,
            _taskBle.brightnessCharAppHandle,
            0,             /* Val_Offset */
            sizeof(float), /* Char_Value_Length */
            (uint8_t *)&brightness);

        aci_gatt_allow_read(_taskBle.connectionHandle);
    }
}

void BLE_IT_HANDLER()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    RAL_Isr();

    taskBleEvent_t event = _TASK_BLE_EVENT_IT;
    xQueueSendFromISR(_taskBle.eventQueue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

tBleStatus _taskBleInitDevice()
{
    tBleStatus bleStatus;
    uint8_t deviceName[] = _TASK_BLE_DEFAULT_NAME;
    uint8_t bdaddr[] = {
        ROM_INFO->UNIQUE_ID_1,
        ROM_INFO->UNIQUE_ID_2,
        ROM_INFO->UNIQUE_ID_3,
        ROM_INFO->UNIQUE_ID_4,
        ROM_INFO->UNIQUE_ID_5,
        ROM_INFO->UNIQUE_ID_6};

    // Configure Ble: device public address
    bleStatus = aci_hal_write_config_data(
        CONFIG_DATA_PUBADDR_OFFSET,
        CONFIG_DATA_PUBADDR_LEN,
        bdaddr);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    /* Set the TX power -14 dBm */
    bleStatus = aci_hal_set_tx_power_level(1, 0);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Enable "LE Enhanced Connection Complete Event"
    static uint8_t LE_Event_Mask[8] = {0x1F, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    bleStatus = hci_le_set_event_mask(LE_Event_Mask);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Enable the callback end_of_radio_activity_event
    bleStatus = aci_hal_set_radio_activity_mask(0x00ff);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Init Ble: GATT layer
    bleStatus = aci_gatt_init();
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Init Ble: GAP layer
    bleStatus = aci_gap_init(
        GAP_PERIPHERAL_ROLE,
        0x02, // Privacy controller enabled
        _TASK_BLE_NAME_MAX_SIZE,
        &_taskBle.serviceGapHandle,
        &_taskBle.devNameCharGapHandle,
        &_taskBle.appearanceCharGapHandle);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Update device name
    bleStatus = aci_gatt_update_char_value_ext(
        0, _taskBle.serviceGapHandle, _taskBle.devNameCharGapHandle,
        0x00, // GATT_LOCAL_UPDATE
        sizeof(deviceName) - 1,
        0, sizeof(deviceName) - 1, deviceName);

    // Update Appearance (from Bluetooth SIG)
    // - Category (bits 15 to 6) : 0x01C Access Control
    // - Sub-categor (bits 5 to 0) : 0x08 Access Lock
    // - Value : 0x0708 // Door Lock
    uint16_t appearanceVal = 0x0708;
    bleStatus = aci_gatt_update_char_value(
        _taskBle.serviceGapHandle, _taskBle.appearanceCharGapHandle,
        0x00, sizeof(appearanceVal), (uint8_t *)&appearanceVal);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Set the proper security I/O capability
    bleStatus = aci_gap_set_io_capability(IO_CAP_DISPLAY_ONLY);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Set the proper security I/O authentication
    bleStatus = aci_gap_set_authentication_requirement(
        BONDING,
        MITM_PROTECTION_REQUIRED,
        SC_IS_SUPPORTED,
        KEYPRESS_IS_NOT_SUPPORTED,
        7,  // Minimum encryption key size
        16, // Maximum encryption key size
        USE_FIXED_PIN_FOR_PAIRING,
        taskAppGetPin(), // Fixed Pin
        STATIC_RANDOM_ADDR);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    // Set scan response
    uint8_t scanResponseData[31] = "  " _TASK_BLE_DEFAULT_NAME;
    scanResponseData[0] = 1 + sizeof(_TASK_BLE_DEFAULT_NAME) - 1;
    scanResponseData[1] = 0x09;
    _taskBle.bleStatus = hci_le_set_scan_response_data(scanResponseData[0] + 1, scanResponseData);
    if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    return bleStatus;
}

tBleStatus _taskBleAddServices()
{
    tBleStatus bleStatus;

    // UUID from https://www.famkruithof.net/uuid/uuidgen
    // 44707b20-3459-11ee-aea4-0800200c9a66
    // 44707b21-3459-11ee-aea4-0800200c9a66
    // 44707b22-3459-11ee-aea4-0800200c9a66
    // 44707b23-3459-11ee-aea4-0800200c9a66
    // 44707b24-3459-11ee-aea4-0800200c9a66
    // 44707b25-3459-11ee-aea4-0800200c9a66
    Service_UUID_t serviceAppUuid = {.Service_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x20, 0x7b, 0x70, 0x44}};
    Char_UUID_t lockStateCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x21, 0x7b, 0x70, 0x44}};
    Char_UUID_t doorStateCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x22, 0x7b, 0x70, 0x44}};
    Char_UUID_t openDoorCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x23, 0x7b, 0x70, 0x44}};
    Char_UUID_t brightnessCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x24, 0x7b, 0x70, 0x44}};
    Char_UUID_t brightnessThCharAppUuid = {.Char_UUID_128 = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xa4, 0xae, 0xee, 0x11, 0x59, 0x34, 0x25, 0x7b, 0x70, 0x44}};

    bleStatus = aci_gatt_add_service(
        UUID_TYPE_128, &serviceAppUuid,
        PRIMARY_SERVICE,
        1 + NUM_APP_GATT_ATTRIBUTES, // 1 (service) + N characteristic
        &_taskBle.serviceAppHandle);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    bleStatus = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &lockStateCharAppUuid,
        sizeof(uint8_t), // Maximum length of the characteristic value
        CHAR_PROP_WRITE,
        ATTR_PERMISSION_AUTHEN_WRITE | ATTR_PERMISSION_AUTHOR_WRITE | ATTR_PERMISSION_ENCRY_WRITE,
        GATT_NOTIFY_ATTRIBUTE_WRITE,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.lockStateCharAppHandle);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    bleStatus = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &doorStateCharAppUuid,
        sizeof(uint8_t), // Maximum length of the characteristic value
        CHAR_PROP_READ | CHAR_PROP_NOTIFY,
        ATTR_PERMISSION_AUTHEN_READ | ATTR_PERMISSION_AUTHOR_READ | ATTR_PERMISSION_ENCRY_READ,
        GATT_DONT_NOTIFY_EVENTS,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.doorStateCharAppHandle);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    bleStatus = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &openDoorCharAppUuid,
        sizeof(uint8_t), // Maximum length of the characteristic value
        CHAR_PROP_WRITE,
        ATTR_PERMISSION_AUTHEN_WRITE | ATTR_PERMISSION_AUTHOR_WRITE | ATTR_PERMISSION_ENCRY_WRITE,
        GATT_NOTIFY_ATTRIBUTE_WRITE,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.openDoorCharAppHandle);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    bleStatus = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &brightnessCharAppUuid,
        sizeof(float), // Maximum length of the characteristic value
        CHAR_PROP_READ,
        ATTR_PERMISSION_AUTHEN_READ | ATTR_PERMISSION_AUTHOR_READ | ATTR_PERMISSION_ENCRY_READ,
        GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.brightnessCharAppHandle);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    bleStatus = aci_gatt_add_char(
        _taskBle.serviceAppHandle,
        UUID_TYPE_128, &brightnessThCharAppUuid,
        sizeof(float), // Maximum length of the characteristic value
        CHAR_PROP_WRITE | CHAR_PROP_READ,
        ATTR_PERMISSION_AUTHEN_WRITE | ATTR_PERMISSION_AUTHOR_WRITE | ATTR_PERMISSION_ENCRY_WRITE |
            ATTR_PERMISSION_AUTHEN_READ | ATTR_PERMISSION_AUTHOR_READ | ATTR_PERMISSION_ENCRY_READ,
        GATT_NOTIFY_ATTRIBUTE_WRITE,
        0x10, // Minimum encryption key size
        0x00, // Characteristic value has a fixed length
        &_taskBle.brightnessThCharAppHandle);
    if (bleStatus != BLE_STATUS_SUCCESS)
        return bleStatus;

    return bleStatus;
}

void _taskBleUpdateWhitelist()
{
    do
    {
        uint8_t bondedLen;
        Bonded_Device_Entry_t bonded[MAX_NUM_BONDED_DEVICES];
        Whitelist_Identity_Entry_t whitelist[MAX_NUM_BONDED_DEVICES];

        _taskBle.bleStatus = aci_gap_get_bonded_devices(&bondedLen, bonded);
        if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
            break;

        for (uint8_t i = 0; i < bondedLen; i++)
        {
            whitelist[i].Peer_Identity_Address_Type = bonded[i].Address_Type;
            memcpy(whitelist[i].Peer_Identity_Address, bonded[i].Address, 6);
        }

        _taskBle.bleStatus = aci_gap_configure_whitelist();
        if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
            break;

        if (bondedLen > 0)
            _taskBle.bleStatus = aci_gap_add_devices_to_resolving_list(
                bondedLen, whitelist, 1);
    } while (0);

    if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
    {
        boardDgb("Ble: update whitelist error: %s\r\n",
                 _taskBleStatusToStr(_taskBle.bleStatus));
        taskBleSendEvent(BLE_EVENT_ERR);
    }
}

void _taskBleMakeDiscoverable(bool bond)
{
    _TASK_BLE_FLAG_CLEAR(BONDING);

    if _TASK_BLE_FLAG_IS (CONNECTED)
    {
        // Force disconnection advertising.
        _taskBle.bleStatus = aci_gap_terminate(
            _taskBle.connectionHandle,
            0x13); // Remote User Terminated Connection

        // Next step from event: hci_disconnection_complete_event
        // Wait disconnection befor tourne in bond mode if enable
        if (bond == true)
            _TASK_BLE_FLAG_SET(BONDING);
        return;
    }

    // Disables advertising.
    _taskBle.bleStatus = aci_gap_set_non_discoverable();

    do
    {
        if ((_taskBle.bleStatus != BLE_STATUS_SUCCESS) &&
            (_taskBle.bleStatus != BLE_STATUS_NOT_ALLOWED))
            break;

        if (bond == true)
        {
            _taskBle.bleStatus = aci_gap_set_undirected_connectable(
                (_TASK_BLE_BOND_ADV_INTERVAL_MIN_MS * 1000) / 625,
                (_TASK_BLE_BOND_ADV_INTERVAL_MAX_MS * 1000) / 625,
                RESOLVABLE_PRIVATE_ADDR,
                NO_WHITE_LIST_USE);
            if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
                break;

            _TASK_BLE_FLAG_SET(BONDING);
            boardDgb("Ble: discoverable in bond mode.\r\n");
        }
        else
        {
            _taskBle.bleStatus = aci_gap_set_undirected_connectable(
                (_TASK_BLE_ADV_INTERVAL_MIN_MS * 1000) / 625,
                (_TASK_BLE_ADV_INTERVAL_MAX_MS * 1000) / 625,
                RESOLVABLE_PRIVATE_ADDR,
                WHITE_LIST_FOR_ALL);
            if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
                break;

            boardDgb("Ble: discoverable.\r\n");
        }
    } while (0);

    if (_taskBle.bleStatus != BLE_STATUS_SUCCESS)
    {
        boardDgb("Ble: make discoverable error: %s\r\n",
                 _taskBleStatusToStr(_taskBle.bleStatus));
        taskBleSendEvent(BLE_EVENT_ERR);
    }
}

const char *_taskBleStatusToStr(tBleStatus status)
{
    switch (status)
    {
    // Standard Error Codes
    case BLE_STATUS_SUCCESS:                        return "success";
    case BLE_ERROR_UNKNOWN_HCI_COMMAND:             return "unknown hci command";
    case BLE_ERROR_UNKNOWN_CONNECTION_ID:           return "unknown connextion id";
    case BLE_ERROR_HARDWARE_FAILURE:                return "hardware failure";
    case BLE_ERROR_AUTHENTICATION_FAILURE:          return "authentication failure";
    case BLE_ERROR_KEY_MISSING:                     return "key missing";
    case BLE_ERROR_MEMORY_CAPACITY_EXCEEDED:        return "memory capacity exceeded";
    case BLE_ERROR_CONNECTION_TIMEOUT:              return "connection timeout";
    case BLE_ERROR_COMMAND_DISALLOWED:              return "command disallowed";
    case BLE_ERROR_UNSUPPORTED_FEATURE:             return "unsupported feature";
    case BLE_ERROR_INVALID_HCI_CMD_PARAMS:          return "invalid hci cmd params";
    case BLE_ERROR_TERMINATED_REMOTE_USER:          return "terminated remote user";
    case BLE_ERROR_TERMINATED_LOCAL_HOST:           return "terminated local_host";
    case BLE_ERROR_UNSUPP_RMT_FEATURE:              return "unsupp rmt feature";
    case BLE_ERROR_UNSPECIFIED:                     return "unspecified";
    case BLE_ERROR_PROCEDURE_TIMEOUT:               return "procedure timeout";
    case BLE_ERROR_INSTANT_PASSED:                  return "instant passed";
    case BLE_ERROR_PARAMETER_OUT_OF_RANGE:          return "parameter out of range";
    case BLE_ERROR_HOST_BUSY_PAIRING:               return "host busy pairing";
    case BLE_ERROR_CONTROLLER_BUSY:                 return "controller busy";
    case BLE_ERROR_DIRECTED_ADVERTISING_TIMEOUT:    return "directed advertising timeout";
    case BLE_ERROR_CONNECTION_END_WITH_MIC_FAILURE: return "connection end with mic failure";
    case BLE_ERROR_CONNECTION_FAILED_TO_ESTABLISH:  return "connection failed to establish";

    // Generic/System error codes
    case BLE_STATUS_UNKNOWN_CONNECTION_ID: return "unknown connection id";
    case BLE_STATUS_FAILED:                return "failed";
    case BLE_STATUS_INVALID_PARAMS:        return "invalid params";
    case BLE_STATUS_BUSY:                  return "busy";
    case BLE_STATUS_PENDING:               return "pending";
    case BLE_STATUS_NOT_ALLOWED:           return "not allowed";
    case BLE_STATUS_ERROR:                 return "error";
    case BLE_STATUS_OUT_OF_MEMORY:         return "out of memory";

    // L2CAP error codes
    case BLE_STATUS_INVALID_CID: return "invalid cid";

    // Security Manager error codes
    case BLE_STATUS_DEV_IN_BLACKLIST:  return "device in blacklist";
    case BLE_STATUS_CSRK_NOT_FOUND:    return "csrk not found";
    case BLE_STATUS_IRK_NOT_FOUND:     return "irk not found";
    case BLE_STATUS_DEV_NOT_FOUND:     return "device not found";
    case BLE_STATUS_SEC_DB_FULL:       return "sec db full";
    case BLE_STATUS_DEV_NOT_BONDED:    return "device not bonded";
    case BLE_INSUFFICIENT_ENC_KEYSIZE: return "insufficient enc keysize";

    // Gatt layer Error Codes
    case BLE_STATUS_INVALID_HANDLE:         return "invalid handle";
    case BLE_STATUS_OUT_OF_HANDLE:          return "out of handle";
    case BLE_STATUS_INVALID_OPERATION:      return "invalid operation";
    case BLE_STATUS_CHARAC_ALREADY_EXISTS:  return "charac already exists";
    case BLE_STATUS_INSUFFICIENT_RESOURCES: return "insufficient resources";
    case BLE_STATUS_SEC_PERMISSION_ERROR:   return "satisfy permission error";

    // GAP layer Error Codes
    case BLE_STATUS_ADDRESS_NOT_RESOLVED: return "address not resolved";

    // Link Layer error Codes
    case BLE_STATUS_NO_VALID_SLOT:       return "no valid slot";
    case BLE_STATUS_SCAN_WINDOW_SHORT:   return "scan window short";
    case BLE_STATUS_NEW_INTERVAL_FAILED: return "new interval failed";
    case BLE_STATUS_INTERVAL_TOO_LARGE:  return "interval too large";
    case BLE_STATUS_LENGTH_FAILED:       return "length failed";

    // flash error codes
    case FLASH_READ_FAILED:  return "Flash read failed";
    case FLASH_WRITE_FAILED: return "Flash write failed";
    case FLASH_ERASE_FAILED: return "Flash erase failed";

    // Profiles Library Error Codes
    case BLE_STATUS_TIMEOUT:                     return "timeout";
    case BLE_STATUS_PROFILE_ALREADY_INITIALIZED: return "profile already initialized";
    case BLE_STATUS_NULL_PARAM:                  return "null param";

    default: return "Unknow error";
    }
    return "";
}

uint16_t _taskBleGetCharHandleFromAtt(bleAtt_t att)
{
    switch (att)
    {
    case BLE_ATT_LOCK_STATE:    return _taskBle.lockStateCharAppHandle;
    case BLE_ATT_DOOR_STATE:    return _taskBle.doorStateCharAppHandle;
    case BLE_ATT_OPEN_DOOR:     return _taskBle.openDoorCharAppHandle;
    case BLE_ATT_BRIGHTNESS:    return _taskBle.brightnessCharAppHandle;
    case BLE_ATT_BRIGHTNESS_TH: return _taskBle.brightnessThCharAppHandle;
    default:                    break;
    }
    return 0xffff;
}

// It's not very clean but I don't want to make the taskBle_t structure visible
// outside of this file.
#include "bleEvents.c"

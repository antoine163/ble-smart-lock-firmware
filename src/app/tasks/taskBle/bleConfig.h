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

/**
 * @file bleConfig.h
 * @author antoine163
 * @date 15.07.2023
 * @brief This file is recovered and adjusted from examples of ST.
 * It contains all the information needed to init the BlueNRG-1 stack.
 * These constants and variables are used from the BlueNRG-1 stack to reserve
 * RAM and FLASH according the application requests.
 */

#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include "bluenrg1_stack.h"
#include "bluenrg_x_device.h"
#include "stack_user_cfg.h"

/* Default number of link */
#define MIN_NUM_LINK (1)
/* Default number of GAP and GATT services */
#define DEFAULT_NUM_GATT_SERVICES (2)
/* Default number of GAP and GATT attributes */
#define DEFAULT_NUM_GATT_ATTRIBUTES (11)

/* Number of services requests from "Ble Smart Lock" */
#define NUM_APP_GATT_SERVICES 1 /* 1 "Ble Smart Lock" service */

/* Number of attributes requests from the "Ble Smart Lock" demo */
#define NUM_APP_GATT_ATTRIBUTES 11 /* 11 attributes x BLE "Ble Smart Lock" service characteristics */
// | Total  |     Characteristic     | Number of GATT Attribute Records         |
// | ------ | ---------------------- | ---------------------------------------- |
// |   2    | lock state             | (1) declaration + (1) value              |
// |   3    | door state             | (1) declaration + (1) value + (1) notify |
// |   2    | open door              | (1) declaration + (1) value              |
// |   2    | brightness             | (1) declaration + (1) value              |
// |   2    | brightness threshold   | (1) declaration + (1) value              |
//   -----
//    11

/* OTA characteristics maximum lenght */
#define OTA_MAX_ATT_SIZE (0)

#define MAX_CHAR_LEN(a, b) ((a) > (b)) ? (a) : (b)

/* Set supported max value for attribute size: it is the biggest attribute size enabled by the application. */
#define USER_MAX_ATT_SIZE (4)
#define APP_MAX_ATT_SIZE  MAX_CHAR_LEN(OTA_MAX_ATT_SIZE, USER_MAX_ATT_SIZE)

/* Number of links needed for "Ble Smart Lock".
 * Only 1 the default
 */
#define NUM_LINKS (MIN_NUM_LINK)

/* Number of GATT attributes needed for "Ble Smart Lock". */
#define NUM_GATT_ATTRIBUTES (DEFAULT_NUM_GATT_ATTRIBUTES + NUM_APP_GATT_ATTRIBUTES)

/* Number of GATT services needed for "Ble Smart Lock". */
#define NUM_GATT_SERVICES (DEFAULT_NUM_GATT_SERVICES + NUM_APP_GATT_SERVICES)

/* Array size for the attribute value */
#define ATT_VALUE_ARRAY_SIZE (44 + 16 + 106) /* 44 Only GATT & GAP default services + 16 max device name + 106 app characteristic*/
// (GATT + GAP) = 36 (default) + 16 (max device name)
// | Total  |     Characteristic     | Size of the Value Array        |
// | ------ | ---------------------- | ------------------------------ |
// |   20   | lock state             | (19) UUID 128 + (1) value size |
// |   20   | door state             | (19) UUID 128 + (1) value size |
// |   20   | open door              | (19) UUID 128 + (1) value size |
// |   23   | brightness             | (19) UUID 128 + (4) value size |
// |   23   | brightness threshold   | (19) UUID 128 + (4) value size |
//   -----
//    106

/* Flash security database size */
#define FLASH_SEC_DB_SIZE (0x400)

/* Flash server database size */
#define FLASH_SERVER_DB_SIZE (0x400)

/* Set supported max value for ATT_MTU enabled by the application */
#define MAX_ATT_MTU (DEFAULT_ATT_MTU)

/* Set supported max value for attribute size: it is the biggest attribute size enabled by the application */
#define MAX_ATT_SIZE (APP_MAX_ATT_SIZE)

/* Set the minumum number of prepare write requests needed for a long write procedure for a characteristic with len > 20bytes:
 *
 * It returns 0 for characteristics with len <= 20bytes
 *
 * NOTE: If prepare write requests are used for a characteristic (reliable write on multiple characteristics), then
 * this value should be set to the number of prepare write needed by the application.
 *
 *  [New parameter added on BLE stack v2.x]
 */
#define PREPARE_WRITE_LIST_SIZE PREP_WRITE_X_ATT(MAX_ATT_SIZE)

/* Additional number of memory blocks  to be added to the minimum  */
#define OPT_MBLOCKS (6) /* 6:  for reaching the max throughput: ~220kbps (same as BLE stack 1.x) */

/* Set the number of memory block for packet allocation */
#define MBLOCKS_COUNT (MBLOCKS_CALC(PREPARE_WRITE_LIST_SIZE, MAX_ATT_MTU, NUM_LINKS) + OPT_MBLOCKS)

/* RAM reserved to manage all the data stack according the number of links,
 * number of services, number of attributes and attribute value length
 */
NO_INIT(uint32_t dyn_alloc_a[TOTAL_BUFFER_SIZE(NUM_LINKS, NUM_GATT_ATTRIBUTES, NUM_GATT_SERVICES, ATT_VALUE_ARRAY_SIZE, MBLOCKS_COUNT, CONTROLLER_DATA_LENGTH_EXTENSION_ENABLED) >> 2]);

/* FLASH reserved to store all the security database information and
 * and the server database information
 */
NO_INIT_SECTION(uint32_t stacklib_flash_data[TOTAL_FLASH_BUFFER_SIZE(FLASH_SEC_DB_SIZE, FLASH_SERVER_DB_SIZE) >> 2], ".noinit.stacklib_flash_data");

/* FLASH reserved to store: security root keys, static random address, public address */
NO_INIT_SECTION(uint8_t stacklib_stored_device_id_data[56], ".noinit.stacklib_stored_device_id_data");

/* Maximum duration of the connection event */
#define MAX_CONN_EVENT_LENGTH (0xFFFFFFFF)

/* Sleep clock accuracy */
#if (LS_SOURCE == LS_SOURCE_INTERNAL_RO)
/* Sleep clock accuracy in Slave mode */
#define SLAVE_SLEEP_CLOCK_ACCURACY (500)
/* Sleep clock accuracy in Master mode */
#define MASTER_SLEEP_CLOCK_ACCURACY (MASTER_SCA_500ppm)
#else
/* Sleep clock accuracy in Slave mode */
#define SLAVE_SLEEP_CLOCK_ACCURACY  (100)
/* Sleep clock accuracy in Master mode */
#define MASTER_SLEEP_CLOCK_ACCURACY (MASTER_SCA_100ppm)
#endif

/* Low Speed Oscillator source */
#if (LS_SOURCE == LS_SOURCE_INTERNAL_RO)
#define LOW_SPEED_SOURCE (1) // Internal RO
#else
#define LOW_SPEED_SOURCE (0) // External 32 KHz
#endif

/* High Speed start up time */
#define HS_STARTUP_TIME (328) // 800 us

/* Radio Config Hot Table */
extern uint8_t hot_table_radio_config[];

/* Low level hardware configuration data for the device */
#define CONFIG_TABLE                        \
    {                                       \
        (uint32_t *)hot_table_radio_config, \
            MAX_CONN_EVENT_LENGTH,          \
            SLAVE_SLEEP_CLOCK_ACCURACY,     \
            MASTER_SLEEP_CLOCK_ACCURACY,    \
            LOW_SPEED_SOURCE,               \
            HS_STARTUP_TIME                 \
    }

/* This structure contains memory and low level hardware configuration data for the device */
const BlueNRG_Stack_Initialization_t BlueNRG_Stack_Init_params = {
    (uint8_t *)stacklib_flash_data,
    FLASH_SEC_DB_SIZE,
    FLASH_SERVER_DB_SIZE,
    (uint8_t *)stacklib_stored_device_id_data,
    (uint8_t *)dyn_alloc_a,
    TOTAL_BUFFER_SIZE(NUM_LINKS, NUM_GATT_ATTRIBUTES, NUM_GATT_SERVICES, ATT_VALUE_ARRAY_SIZE, MBLOCKS_COUNT, CONTROLLER_DATA_LENGTH_EXTENSION_ENABLED),
    NUM_GATT_ATTRIBUTES,
    NUM_GATT_SERVICES,
    ATT_VALUE_ARRAY_SIZE,
    NUM_LINKS,
    0, /* reserved for future use */
    PREPARE_WRITE_LIST_SIZE,
    MBLOCKS_COUNT,
    MAX_ATT_MTU,
    CONFIG_TABLE};

#endif // BLE_CONFIG_H

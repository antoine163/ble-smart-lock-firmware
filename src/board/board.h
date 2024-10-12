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

#ifndef BOARD_H
#define BOARD_H

// Includes --------------------------------------------------------------------
#include "BlueNRG1_gpio.h"
#include "mapHard.h"

#include <FreeRTOS.h>

// Define ---------------------------------------------------------------------
#define MAX_TIMEOUT (unsigned int)(-1)

// Enum ------------------------------------------------------------------------
/**
 * @brief Enum representing different colors.
 */
typedef enum
{
    COLOR_OFF,        /**< No color / off */
    COLOR_RED,        /**< Red color */
    COLOR_GREEN,      /**< Green color */
    COLOR_BLUE,       /**< Blue color */
    COLOR_YELLOW,     /**< Yellow color */
    COLOR_CYAN,       /**< Cyan color */
    COLOR_MAGENTA,    /**< Magenta color */
    COLOR_WHITE,      /**< White color */
    COLOR_WHITE_LIGHT /**< Light white color */
} color_t;

/**
 * @brief Enum representing different board events.
 */
typedef enum
{
    BOARD_EVENT_BUTTON_BOND_STATE, /**< Event for button bond state */
    BOARD_EVENT_DOOR_STATE,        /**< Event for door state */
} boardEvent_t;

// Prototype functions ---------------------------------------------------------
void boardInit();
void boardReset();

void boardDgbEnable(bool enable);
int boardDgb(char const *format, ...);

void boardWriteChar(char c);
int boardPrintf(char const *format, ...);
char boardReadChar(unsigned int timeout);

void boardSetLightColor(color_t color);
void boardSetLightDc(float dc);

float boardGetBrightness();

void boardLock();
void boardUnlock();
bool boardIsLocked();

void boardOpen();
bool boardIsOpen();
void boardOpenItSetLevel(bool low);

// Function called by the ISR board to send an event to the App task
// This function is implemented in the App task
void boardSendEventFromISR(boardEvent_t event,
                           BaseType_t *pxHigherPriorityTaskWoken);

// Prototype static functions --------------------------------------------------
static inline void boardLedOn()
{
    GPIO_SetBits(LED_PIN);
}

static inline void boardLedOff()
{
    GPIO_ResetBits(LED_PIN);
}

static inline void boardLedToggel()
{
    GPIO_ToggleBits(LED_PIN);
}

static inline bool boardButtonBondState()
{
    return GPIO_ReadBit(BOND_PIN) == Bit_SET;
}

#endif // BOARD_H

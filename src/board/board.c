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

// Includes --------------------------------------------------------------------
#include "board.h"

#include "BlueNRG1_sysCtrl.h"
#include "BlueNRG1_wdg.h"
#include "misc.h"
#include "sleep.h"

#include "itConfig.h"

#include "drivers/adc.h"
#include "drivers/pwm.h"
#include "drivers/uart.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "freertos_ble.h"
#include <FreeRTOS.h>
#include <semphr.h>

#include <math.h>

// Define ----------------------------------------------------------------------
#define _BOARD_UART_RX_FIFO_SIZE 32
#define _BOARD_UART_TX_FIFO_SIZE 1024

/** Set the watchdog reload interval in [s] = (WDT_LOAD + 3) / (clock frequency in Hz). */
#define _BOARD_RC32K_FREQ       32768
#define _BOARD_RELOAD_TIME(sec) ((sec * _BOARD_RC32K_FREQ) - 3)
#define _BOARD_WDG_TIME         15

// Struct ----------------------------------------------------------------------
typedef struct
{
    // Serial section
    uart_t serial;
    uint8_t serialBufTx[_BOARD_UART_TX_FIFO_SIZE];
    uint8_t serialBufRx[_BOARD_UART_RX_FIFO_SIZE];
    SemaphoreHandle_t serialMutex;
    StaticSemaphore_t serialMutexBuffer;

    // Light section
    pwm_t lightPwm;
    color_t lightColor;
    float lightDc;

    // Sensor section
    adc_t sensorAdc;

    // Lock
    bool loked;

    // Enable debugging messages
    bool verbose;
} board_t;

// Prototype static functions --------------------------------------------------
static void _boardInitGpio();
static void _boardInitUart();
static void _boardInitPwm();
static void _boardInitAdc();
static void _boardInitWdg();
static void _boardEnableIo(bool enable);
static int _boardVprintf(char const *format, va_list ap);

// Global variables ------------------------------------------------------------
board_t _board = {
    .serialMutex = NULL,
    .lightColor = COLOR_OFF,
    .lightDc = 0.,
    .loked = true,
    .verbose = true};

// Implemented functions -------------------------------------------------------

/**
 * @brief Init clock, power, gpio, uart, pwm, adc, of the board.
 */
void boardInit()
{
    // Init system
    SystemInit();

    // Make semaphores
    _board.serialMutex = xSemaphoreCreateMutexStatic(&_board.serialMutexBuffer);

    // Init peripherals
    _boardInitGpio();
    _boardInitUart();
    _boardInitPwm();
    _boardInitAdc();
    _boardInitWdg();

    // Set wakeup Mask to keep wakeup while the opened pin is hight (dio12).
    BlueNRG_SetWakeupMask(
        WAKEUP_IO12, WAKEUP_IOx_HIGH << WAKEUP_IO12_SHIFT_MASK);
}

void boardReset()
{
    // Restart following a whitelist cleanup.
    boardDgb("App: Rebooting ...\r\n");
    vTaskDelay(1); // wait to print message
    NVIC_SystemReset();
}

void boardDgbEnable(bool enable)
{
    _board.verbose = enable;
}

int boardDgb(char const *format, ...)
{
    int n = 0;
    if (_board.verbose == true)
    {
        va_list ap;
        va_start(ap, format);
        n = _boardVprintf(format, ap);
        va_end(ap);
    }

    return n;
}

void boardWriteChar(char c)
{
    xSemaphoreTake(_board.serialMutex, portMAX_DELAY);
    uart_write(&_board.serial, &c, 1);
    xSemaphoreGive(_board.serialMutex);
}

int boardPrintf(char const *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int n = _boardVprintf(format, ap);
    va_end(ap);

    return n;
}

char boardReadChar(unsigned int timeout)
{
    char c = '\0';
    uart_waitRead(&_board.serial, timeout);
    xSemaphoreTake(_board.serialMutex, portMAX_DELAY);
    uart_read(&_board.serial, &c, 1);
    xSemaphoreGive(_board.serialMutex);
    return c;
}

int _boardVprintf(char const *format, va_list ap)
{
    xSemaphoreTake(_board.serialMutex, portMAX_DELAY);
    static char str[256];
    int n = vsnprintf(str, sizeof(str), format, ap);
    n = uart_write(&_board.serial, str, n);
    xSemaphoreGive(_board.serialMutex);

    return n;
}

void _boardEnableIo(bool enable)
{
    if (enable == true)
    {
        if (GPIO_ReadBit(EN_IO_PIN) == Bit_RESET)
        {
            GPIO_WriteBit(EN_IO_PIN, Bit_SET);
            vTaskDelay(100 / portTICK_PERIOD_MS); // Wait stabilizing
        }
    }
    else
        GPIO_WriteBit(EN_IO_PIN, Bit_RESET);
}

void boardSetLightColor(color_t color)
{
    color_t lastColor = _board.lightColor;
    _board.lightColor = color;

    // Off all color
    GPIO_ResetBits(LIGHT_RED_PIN |
                   LIGHT_BLUE_PIN |
                   LIGHT_GREEN_PIN |
                   LIGHT_WHITE_PIN);

    // Set PWM duty cycle
    boardSetLightDc(_board.lightDc);
    if (lastColor != _board.lightColor)
        pwm_clearCounter(&_board.lightPwm);

    // On color
    switch (color)
    {
    case COLOR_OFF:
    {
        _boardEnableIo(false);

        // Disable PWM Pin
        GPIO_InitType GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
        GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_OUT;
        GPIO_Init(&GPIO_InitStruct);
        break;
    }
    case COLOR_RED:
    {
        GPIO_SetBits(LIGHT_RED_PIN);
        break;
    }
    case COLOR_GREEN:
    {
        GPIO_SetBits(LIGHT_GREEN_PIN);
        break;
    }
    case COLOR_BLUE:
    {
        GPIO_SetBits(LIGHT_BLUE_PIN);
        break;
    }
    case COLOR_YELLOW:
    {
        GPIO_SetBits(LIGHT_RED_PIN);
        GPIO_SetBits(LIGHT_GREEN_PIN);
        break;
    }
    case COLOR_CYAN:
    {
        GPIO_SetBits(LIGHT_GREEN_PIN);
        GPIO_SetBits(LIGHT_BLUE_PIN);
        break;
    }
    case COLOR_MAGENTA:
    {
        GPIO_SetBits(LIGHT_RED_PIN);
        GPIO_SetBits(LIGHT_BLUE_PIN);
        break;
    }
    case COLOR_WHITE:
    {
        GPIO_SetBits(LIGHT_RED_PIN);
        GPIO_SetBits(LIGHT_GREEN_PIN);
        GPIO_SetBits(LIGHT_BLUE_PIN);
        break;
    }
    case COLOR_WHITE_LIGHT:
    {
        GPIO_SetBits(LIGHT_WHITE_PIN);
        break;
    }
    }

    if ((lastColor == COLOR_OFF) &&
        (color != COLOR_OFF))
    {
        // Enable PWM Pin
        GPIO_InitType GPIO_InitStruct;
        GPIO_StructInit(&GPIO_InitStruct);
        GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
        GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_PWM;
        GPIO_Init(&GPIO_InitStruct);

        _boardEnableIo(true);
    }
}

void boardSetLightDc(float dc)
{
    // To try to make the perceived brightness more natural in relation to the
    // cyclic ratio
    float expdc = 1.0067836549063043 * expf((dc - 100) * 0.05) * 100 - 0.678365490630423;

    // Set duty cycle
    if (_board.lightColor == COLOR_WHITE_LIGHT)
        pwm_setDc(&_board.lightPwm, expdc);
    else
        pwm_setDc(&_board.lightPwm, 100. - expdc);

    _board.lightDc = dc;
}

float boardGetBrightness()
{
    _boardEnableIo(true);
    adc_config(&_board.sensorAdc, ADC_CH_PIN1);
    float val = adc_convert_voltage(&_board.sensorAdc);
    _boardEnableIo(_board.lightColor != COLOR_OFF);

    // Convert in %
    return 100.0f - val * 100.0f / 3.3f;
}

void boardLock()
{
    GPIO_InitType GPIO_InitStruct = {
        .GPIO_Pin = LOCK_PIN,
        .GPIO_Mode = LOCK_MODE_OUT,
        .GPIO_HighPwr = DISABLE,
        .GPIO_Pull = DISABLE};
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);

    _board.loked = true;
}

void boardUnlock()
{
    GPIO_InitType GPIO_InitStruct = {
        .GPIO_Pin = LOCK_PIN,
        .GPIO_Mode = LOCK_MODE_IN,
        .GPIO_HighPwr = DISABLE,
        .GPIO_Pull = DISABLE};
    GPIO_Init(&GPIO_InitStruct);

    _board.loked = false;
}

bool boardIsLocked()
{
    return _board.loked;
}

void boardOpen()
{
    GPIO_InitType GPIO_InitStruct = {
        .GPIO_Pin = LOCK_PIN,
        .GPIO_Mode = LOCK_MODE_OUT,
        .GPIO_HighPwr = DISABLE,
        .GPIO_Pull = DISABLE};
    GPIO_Init(&GPIO_InitStruct);

    GPIO_SetBits(LOCK_PIN);
    vTaskDelay(150 / portTICK_PERIOD_MS);
    GPIO_ResetBits(LOCK_PIN);

    if (_board.loked == false)
    {
        GPIO_InitStruct.GPIO_Mode = LOCK_MODE_IN;
        GPIO_Init(&GPIO_InitStruct);
    }
}

bool boardIsOpen()
{
    return (GPIO_ReadBit(OPENED_PIN) == Bit_SET) ? true : false;
}

void boardOpenItSetLevel(bool open)
{
    // Re-Configure door status (open/close) Interrupt
    GPIO_EXTIConfigType GPIO_EXTIStructure = {
        .GPIO_Pin = OPENED_PIN,
        .GPIO_IrqSense = GPIO_IrqSense_Level,
        .GPIO_Event = open ? GPIO_Event_High : GPIO_Event_Low};
    GPIO_EXTIConfig(&GPIO_EXTIStructure);
}

// Implemented static functions ------------------------------------------------

void _boardInitGpio()
{
    // Init Gpio
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_GPIO, ENABLE);

    GPIO_InitType GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);

    // ---- Pins section ----

    // Init bond button pin
    GPIO_InitStruct.GPIO_Pin = BOND_PIN;
    GPIO_InitStruct.GPIO_Mode = BOND_MODE;
    GPIO_Init(&GPIO_InitStruct);
    // Init opened feedback pin
    GPIO_InitStruct.GPIO_Pin = OPENED_PIN;
    GPIO_InitStruct.GPIO_Mode = OPENED_MODE;
    GPIO_Init(&GPIO_InitStruct);
    // Init lock pin
    GPIO_InitStruct.GPIO_Pin = LOCK_PIN;
    GPIO_InitStruct.GPIO_Mode = LOCK_MODE_OUT;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);
    // Init Led pin
    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = LED_MODE;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);
    // Init enable IO pin
    GPIO_InitStruct.GPIO_Pin = EN_IO_PIN;
    GPIO_InitStruct.GPIO_Mode = EN_IO_MODE;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);

    // ---- Light section ----

    // Init light pins
    GPIO_InitStruct.GPIO_Pin = LIGHT_RED_PIN |
                               LIGHT_GREEN_PIN |
                               LIGHT_BLUE_PIN |
                               LIGHT_WHITE_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_MODE;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);
    // Init light pwm pin
    GPIO_InitStruct.GPIO_Pin = LIGHT_PWM_PIN;
    GPIO_InitStruct.GPIO_Mode = LIGHT_PWM_MODE_OUT;
    GPIO_ResetBits(GPIO_InitStruct.GPIO_Pin);
    GPIO_Init(&GPIO_InitStruct);

    // ---- Uart section ----

    // Init Rx Pin
    GPIO_InitStruct.GPIO_Pin = UART_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = UART_RX_MODE;
    GPIO_Init(&GPIO_InitStruct);

    // Init Tx Pin
    GPIO_InitStruct.GPIO_Pin = UART_TX_PIN;
    GPIO_InitStruct.GPIO_Mode = UART_TX_MODE;
    GPIO_Init(&GPIO_InitStruct);

    // ---- GPIO Interrupt section ----

    // Enable GPIO Interrupt
    NVIC_InitType nvicConfig = {
        .NVIC_IRQChannel = GPIO_IRQn,
        .NVIC_IRQChannelPreemptionPriority = GPIO_IT_PRIORITY,
        .NVIC_IRQChannelCmd = ENABLE};
    NVIC_Init(&nvicConfig);

    // Configure bond button Interrupt
    GPIO_EXTIConfigType GPIO_EXTIStructure;
    GPIO_EXTIStructure.GPIO_Pin = BOND_PIN;
    GPIO_EXTIStructure.GPIO_IrqSense = GPIO_IrqSense_Edge;
    GPIO_EXTIStructure.GPIO_Event = GPIO_Event_Both;
    GPIO_EXTIConfig(&GPIO_EXTIStructure);

    // Configure door status (open/close) Interrupt
    GPIO_EXTIStructure.GPIO_Pin = OPENED_PIN;
    GPIO_EXTIStructure.GPIO_IrqSense = GPIO_IrqSense_Edge;
    GPIO_EXTIStructure.GPIO_Event = GPIO_Event_Both;
    GPIO_EXTIConfig(&GPIO_EXTIStructure);

    // Clear pending GPIO Interrupt
    GPIO_ClearITPendingBit(BOND_PIN | OPENED_PIN);

    // Enable GPIO interrupt
    GPIO_EXTICmd(BOND_PIN | OPENED_PIN, ENABLE);
}

void _boardInitUart()
{
    // Init uart
    uart_init(&_board.serial, UART,
              _board.serialBufTx, sizeof(_board.serialBufTx),
              _board.serialBufRx, sizeof(_board.serialBufRx));

    uart_config(&_board.serial,
                UART_BAUDRATE_115200,
                UART_DATA_8BITS,
                UART_PARITY_NO,
                UART_STOPBIT_1);
}

void _boardInitPwm()
{
    // Init pwm
    pwm_init(&_board.lightPwm, MFT1);
    // Configure the PWM to 100Hz
    pwm_config(&_board.lightPwm, 100);
}

void _boardInitAdc()
{
    // Init adc
    adc_init(&_board.sensorAdc, ADC);
    adc_config(&_board.sensorAdc, ADC_CH_PIN1);
}

void _boardInitWdg()
{
    // Enable watchdog clocks
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_WDG, ENABLE);

    // Set watchdog reload period at 15 seconds
    WDG_SetReload(_BOARD_RELOAD_TIME(_BOARD_WDG_TIME));

    // Enable WDG
    WDG_Enable();
}

// ISR handler -----------------------------------------------------------------
void GPIO_IT_HANDLER()
{
    // Bond button
    if (GPIO_GetITPendingBit(BOND_PIN) == SET)
    {
        GPIO_ClearITPendingBit(BOND_PIN);

        // Send event to App task
        BaseType_t xHigherPriorityTaskWoken;
        boardSendEventFromISR(
            BOARD_EVENT_BUTTON_BOND_STATE,
            &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    // Door state (open/close)
    if (GPIO_GetITPendingBit(OPENED_PIN) == SET)
    {
        boardOpenItSetLevel(!boardIsOpen());
        GPIO_ClearITPendingBit(OPENED_PIN);

        // Send event to App task
        BaseType_t xHigherPriorityTaskWoken;
        boardSendEventFromISR(
            BOARD_EVENT_DOOR_STATE,
            &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Note: App_SleepMode_Check run in IDLE task
SleepModes App_SleepMode_Check(__attribute__((unused)) SleepModes sleepMode)
{
    WDG_SetReload(_BOARD_RELOAD_TIME(_BOARD_WDG_TIME));

#ifdef DEBUG
    // Do not enter deep sleep in debug to keep enable the SWD.
    return SLEEPMODE_CPU_HALT;
#else
    // Do not enter deep sleep until the first 3s.
    // To allow an easier connection with the SWD.
    if (xTaskGetTickCount() < pdMS_TO_TICKS(3000))
        return SLEEPMODE_CPU_HALT;

    // Do not enter deep sleep until Tx uart is not complet.
    if (uart_waitWrite(&_board.serial, 0) == 0)
        return SLEEPMODE_CPU_HALT;

    // Do not enter deep sleep while there is door is open.
    if (boardIsOpen() == true)
        return SLEEPMODE_CPU_HALT;

    // Do not enter deep sleep while there is the light in not off.
    if (_board.lightColor != COLOR_OFF)
        return SLEEPMODE_CPU_HALT;

    // Allow enter deep sleep.
    return SLEEPMODE_NOTIMER;
#endif
}
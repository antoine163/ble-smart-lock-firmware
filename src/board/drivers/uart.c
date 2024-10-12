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
 * @file uart.c
 * @author antoine163
 * @date 01-04-2024
 * @brief UART driver
 */

// Include ---------------------------------------------------------------------
#include "uart.h"

#include "BlueNRG1_sysCtrl.h"
#include "misc.h"

#include "itConfig.h"

// Global variables ------------------------------------------------------------
static uart_t *_usart_dev = NULL;

// Implemented functions -------------------------------------------------------
int uart_init(uart_t *dev,
              UART_Type *const periph,
              uint8_t *bufTx, size_t sizeBufTx,
              uint8_t *bufRx, size_t sizeBufRx)
{
    dev->periph = periph;
    _usart_dev = dev;

    fifo_init(&dev->fifoTx, bufTx, sizeBufTx);
    fifo_init(&dev->fifoRx, bufRx, sizeBufRx);

    dev->txCompleteSem = xSemaphoreCreateBinaryStatic(&dev->txCompleteSemBuffer);
    dev->rxAvailableSem = xSemaphoreCreateBinaryStatic(&dev->rxAvailableSemBuffer);

    xSemaphoreTake(dev->txCompleteSem, 0);

    // Enable UART clk
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_UART, ENABLE);

    // Enable UART Interrupt
    NVIC_InitType nvicConfig = {
        .NVIC_IRQChannel = UART_IRQn,
        .NVIC_IRQChannelPreemptionPriority = UART_IT_PRIORITY,
        .NVIC_IRQChannelCmd = ENABLE};
    NVIC_Init(&nvicConfig);

    return 0;
}

int uart_deinit(uart_t *dev)
{
    // Disable UART Interrupt
    NVIC_InitType nvicConfig = {
        .NVIC_IRQChannel = UART_IRQn,
        .NVIC_IRQChannelCmd = DISABLE};
    NVIC_Init(&nvicConfig);

    // De init uart and disable clock
    UART_DeInit();
    SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_UART, DISABLE);

    vSemaphoreDelete(dev->txCompleteSem);
    vSemaphoreDelete(dev->rxAvailableSem);

    _usart_dev = NULL;
    dev->periph = NULL;
    return 0;
}

int uart_config(uart_t *dev,
                uart_baudrate_t baudrate,
                uart_databits_t databit,
                uart_parity_t parity,
                uart_stopbit_t stopbit)
{
    UART_InitType uartConf;
    uartConf.UART_BaudRate = (uint32_t)baudrate;

    switch (databit)
    {
    case UART_DATA_5BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_5b; break;
    case UART_DATA_6BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_6b; break;
    case UART_DATA_7BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_7b; break;
    case UART_DATA_8BITS: uartConf.UART_WordLengthTransmit = UART_WordLength_8b; break;
    default:              return -1;
    }
    uartConf.UART_WordLengthReceive = uartConf.UART_WordLengthTransmit;

    switch (parity)
    {
    case UART_PARITY_NO:   uartConf.UART_Parity = UART_Parity_No; break;
    case UART_PARITY_ODD:  uartConf.UART_Parity = UART_Parity_Odd; break;
    case UART_PARITY_EVEN: uartConf.UART_Parity = UART_Parity_Even; break;
    default:               return -1;
    }

    switch (stopbit)
    {
    case UART_STOPBIT_1: uartConf.UART_StopBits = UART_StopBits_1; break;
    case UART_STOPBIT_2: uartConf.UART_StopBits = UART_StopBits_2; break;
    default:             return -1;
    }

    uartConf.UART_Mode = 0;
    if (fifo_size(&dev->fifoTx) != 0)
        uartConf.UART_Mode |= UART_Mode_Tx;
    if (fifo_size(&dev->fifoRx) != 0)
        uartConf.UART_Mode |= UART_Mode_Rx;

    uartConf.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    uartConf.UART_FifoEnable = ENABLE;

    // Disable UART prior modifying configuration registers
    UART_Cmd(DISABLE);

    // Set the uart configuration
    UART_Init(&uartConf);

    // Enable RX FIFO Full Interrupt
    UART_RxFifoIrqLevelConfig(FIFO_LEV_3_4);
    UART_TxFifoIrqLevelConfig(FIFO_LEV_1_64);
    UART_ITConfig(UART_IT_RX, ENABLE);

    // Enable UART
    UART_Cmd(ENABLE);

    return 0;
}

int uart_write(uart_t *dev, void const *buf, unsigned int nbyte)
{
    int n = 0;

    // No byte to write ?
    if (nbyte == 0)
        return 0;

    // Disable Tx interrupt
    UART_ITConfig(UART_IT_TX | UART_IT_TXFE, DISABLE);

    // Start TX, so the transfer is not complete
    xSemaphoreTake(dev->txCompleteSem, 0);

    // Clean Tx interrupt
    UART_ClearITPendingBit(UART_IT_TX | UART_IT_TXFE);

    // Transfer data of fifoTx to Uart Fifo
    while ((UART_GetFlagStatus(UART_FLAG_TXFF) == RESET) &&
           !fifo_isEmpty(&dev->fifoTx))
    {
        uint16_t byte = 0;
        FIFO_POP_BYTE((&dev->fifoTx), (uint8_t *)&byte);
        UART_SendData(byte);
    }

    if (fifo_isEmpty(&dev->fifoTx))
    {
        // Transfer data of buf to Uart Fifo
        while ((UART_GetFlagStatus(UART_FLAG_TXFF) == RESET) &&
               ((unsigned int)n < nbyte))
        {
            uint16_t byte = *((uint8_t *)buf + n);
            UART_SendData(byte);
            n++;
        }
    }

    // Transfer remaining data of buf to fifoTx
    n += fifo_push(&dev->fifoTx, (uint8_t *)buf + n, nbyte - n);

    // Enable Tx interrupt
    if (fifo_isEmpty(&dev->fifoTx))
        UART_ITConfig(UART_IT_TXFE, ENABLE); // Tx complet
    else
        UART_ITConfig(UART_IT_TX, ENABLE); // Tx UART FiFo almost empty

    return n;
}

int uart_waitWrite(uart_t *dev, unsigned int timeout_ms)
{
    int ret = 0;
    TickType_t tickOut = portMAX_DELAY;
    if (timeout_ms != UART_MAX_TIMEOUT)
        tickOut = pdMS_TO_TICKS(timeout_ms);

    // Wait complet transfer
    if (xSemaphoreTake(dev->txCompleteSem, tickOut) == pdTRUE)
    {
        vPortEnterCritical();
        if (UART_GetFlagStatus(UART_FLAG_BUSY) == RESET)
        {
            xSemaphoreGive(dev->txCompleteSem);
            ret = 1;
        }
        vPortExitCritical();
    }

    return ret;
}

int uart_read(uart_t *dev, void *buf, unsigned int nbyte)
{
    size_t n = 0;

    // No byte to read ?
    if (nbyte == 0)
        return 0;

    UART_ITConfig(UART_IT_RX, DISABLE);

    // Transfer data of fifoRx to buf
    n += fifo_pop(&dev->fifoRx, buf, nbyte);

    // Transfer data Uart Fifo to buf
    while (((unsigned int)n < nbyte) &&
           (UART_GetFlagStatus(UART_FLAG_RXFE) == RESET))
    {
        uint8_t byte = (uint8_t)UART_ReceiveData();
        *((uint8_t *)buf + n) = byte;
        n++;
    }

    // Transfer data Uart Fifo to fifoRx
    while ((UART_GetFlagStatus(UART_FLAG_RXFE) == RESET) &&
           !fifo_isFull(&dev->fifoRx))
    {
        uint16_t byte = UART_ReceiveData();
        FIFO_PUSH_BYTE((&dev->fifoRx), (uint8_t)byte);
    }

    if (fifo_isEmpty(&dev->fifoRx))
        xSemaphoreTake(dev->rxAvailableSem, 0);

    UART_ITConfig(UART_IT_RX, ENABLE);

    return n;
}

int uart_waitRead(uart_t *dev, unsigned int timeout_ms)
{
    TickType_t tickOut = portMAX_DELAY;
    if (timeout_ms != UART_MAX_TIMEOUT)
        tickOut = pdMS_TO_TICKS(timeout_ms);

    // Wait available byte in Rx fifo
    UART_RxFifoIrqLevelConfig(FIFO_LEV_1_64);
    if (xSemaphoreTake(dev->rxAvailableSem, tickOut) == pdTRUE)
        xSemaphoreGive(dev->rxAvailableSem);
    UART_RxFifoIrqLevelConfig(FIFO_LEV_3_4);

    return fifo_used(&dev->fifoRx);
}

// ISR handler -----------------------------------------------------------------
static inline void _uartIsrHandler(uart_t *dev)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // -- Manage transmit data --
    // The UART Tx FiFo is almost empty and the interrupt is enable ?
    if ((UART_GetITStatus(UART_IT_TX) == SET) &&
        (READ_BIT(UART->IMSC, UART_IT_TX) != 0))
    {
        UART_ClearITPendingBit(UART_IT_TX);

        // Transfer data Fifo to Uart Fifo
        while ((UART_GetFlagStatus(UART_FLAG_TXFF) == RESET) &&
               !fifo_isEmpty(&dev->fifoTx))
        {
            uint16_t byte = 0;
            FIFO_POP_BYTE((&dev->fifoTx), (uint8_t *)&byte);
            UART_SendData(byte);
        }

        // Disable UART Tx FiFo empty interrupt if the data Tx FiFo is empty.
        if (fifo_isEmpty(&dev->fifoTx))
        {
            UART_ITConfig(UART_IT_TX, DISABLE);
            UART_ITConfig(UART_IT_TXFE, ENABLE);
        }
    }

    // The UART Tx FiFo is empty and the interrupt is enable ?
    if ((UART_GetITStatus(UART_IT_TXFE) == SET) &&
        (READ_BIT(UART->IMSC, UART_IT_TXFE) != 0))
    {
        UART_ClearITPendingBit(UART_IT_TXFE);
        UART_ITConfig(UART_IT_TXFE, DISABLE);

        // The transfer is complet
        xSemaphoreGiveFromISR(dev->txCompleteSem, &xHigherPriorityTaskWoken);
    }

    // -- Manage receive data --
    // The UART Rx FiFo is full and the interrupt is enable ?
    if ((UART_GetITStatus(UART_IT_RX) == SET) &&
        (READ_BIT(UART->IMSC, UART_IT_RX) != 0))
    {
        UART_ClearITPendingBit(UART_IT_RX);

        // Transfer Uart Fifo to data Fifo
        while ((UART_GetFlagStatus(UART_FLAG_RXFE) == RESET))
        {
            uint16_t byte = UART_ReceiveData();
            FIFO_PUSH_BYTE((&dev->fifoRx), (uint8_t)byte);
        }

        // Data available in Rx FiFo
        xSemaphoreGiveFromISR(dev->rxAvailableSem, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void UART_IT_HANDLER()
{
    _uartIsrHandler(_usart_dev);
}

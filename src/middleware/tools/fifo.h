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
 * @file fifo.h
 * @author antoine163
 * @date 15.07.2023
 * @brief This is a generic FiFo (First input fist output)
 */

#ifndef FIFO_H
#define FIFO_H

// Include ---------------------------------------------------------------------

// Std
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Macro -----------------------------------------------------------------------
/** 
 * @brief Initialize a FiFo at the declaration.
 * 
 * This macro initialize the FiFo structure with a storage buffer.
 * 
 * @param storageBuffer Points to a uint8_t array where will be stor the payload
 * of FiFo.
 * @param size Size of @p storageBuffer.
 * 
 * For example:
 * @code
 * uint8_t myFifoBuffer[32];
 * fifo_t myFifo = FIFO_INIT(myFifoBuffer, sizeof(myFifoBuffer));
 * @endcode
 */
#define FIFO_INIT(storageBuffer, size)                                  \
    {                                                                   \
        .buffer     = storageBuffer,                                    \
        .bufferEnd  = storageBuffer + size,                             \
        .bufferSize = size,                                             \
        .writeTo    = storageBuffer,                                    \
        .readFrom   = storageBuffer,                                    \
        .used       = 0                                                 \
    }                                                                   \

/** 
 * @brief Macro to push byte in the FiFo.
 * @warning This macro don't check if there are free byte in the FiFo, you be
 * sure one byte free before call it.
 * 
 * @param fifo Points to the FiFo.
 * @param byte This is byte to push
 * 
 * @see @ref fifo_isFull()
 */
#define FIFO_PUSH_BYTE(fifo, byte)                                      \
    *fifo->writeTo = byte;                                              \
    fifo->writeTo++;                                                    \
    if (fifo->writeTo >= fifo->bufferEnd)                               \
        fifo->writeTo = fifo->buffer;                                   \
    fifo->used++;

/** 
 * @brief Macro to pop byte from the FiFo.
 * @warning This macro don't check if there are available byte in the FiFo, you
 * be sure one byte available before call it.
 * 
 * @param fifo Points to the FiFo.
 * @param byte Points on a uint8_t where will be write the byte.
 * 
 * @see @ref fifo_isEmpty()
 */
#define FIFO_POP_BYTE(fifo, byte)                                       \
    *byte = *fifo->readFrom;                                            \
    fifo->readFrom++;                                                   \
    if (fifo->readFrom >= fifo->bufferEnd)                              \
        fifo->readFrom = fifo->buffer;                                  \
    fifo->used--;

// Structure -------------------------------------------------------------------
/** 
 * @brief FiFo structure definition
 */
typedef struct
{
    uint8_t *       buffer;     //!< Points to beginning of fifo storage area.
    uint8_t *       bufferEnd;  //!< Points to the end of fifo storage area.
    size_t          bufferSize; //!< Size of fifo storage area.
    
    uint8_t *       writeTo;    //!< Points to the next unused byte in the storage area.
    uint8_t *       readFrom;   //!< Points to the first used byte in the storage area.
    size_t          used;       //!< The number of byte used in the storage area.
} fifo_t;

// Static inline functions -----------------------------------------------------

/** 
 * @brief Return id the FiFo is empty.
 * @param fifo Points to the FiFo.
 * @return @c true in the FiFo is empty, @c false otherwise.
 */
static inline bool fifo_isEmpty(fifo_t const * fifo)
{
    return (fifo->used == 0);
}

/** 
 * @brief Return id the FiFo is full.
 * @param fifo Points to the FiFo.
 * @return @c true in the FiFo is full, @c false otherwise.
 */
static inline bool fifo_isFull(fifo_t const * fifo)
{
    return (fifo->used == fifo->bufferSize);
}

/** 
 * @brief Get the capacity of the FiFo.
 * @param fifo Points to the FiFo.
 * @return The size in byte of the FiFo.
 */
static inline size_t fifo_size(fifo_t const * fifo)
{
    return fifo->bufferSize;
}

/** 
 * @brief Get the number of data used in the FiFo.
 * @param fifo Points to the FiFo.
 * @return The size used in byte of the FiFo.
 */
static inline size_t fifo_used(fifo_t const * fifo)
{
    return fifo->used;
}

/** 
 * @brief Get the number of data unused in the FiFo.
 * @param fifo Points to the FiFo.
 * @return The size unused in byte of the FiFo.
 */
static inline size_t fifo_unused(fifo_t const * fifo)
{
    return fifo->bufferSize - fifo->used;
}

// Prototype functions ---------------------------------------------------------
/** 
 * @brief Initialize a FiFo.
 * 
 * This function initialize the FiFo structure definition with a storage buffer.
 * 
 * @param fifo Points to the FiFo.
 * @param storageBuffer Points to a uint8_t array where will be stor the payload
 * of FiFo.
 * @param size Size of @p storageBuffer.
 */
void fifo_init(fifo_t * fifo, uint8_t * storageBuffer, size_t size);

/** 
 * @brief Clean a FiFo.
 * @param fifo Points to the FiFo.
 */
void fifo_clean(fifo_t * fifo);

/** 
 * @brief Adds bytes to the end of the FiFo.
 * 
 * @param fifo Points to the FiFo.
 * @param buf Points to the read buffer.
 * @param nbyte The number of byte to push.
 * 
 * @note If @p nbyte if bigger than the number unused byte in the FiFo, only the
 * unused bytes in FiFo will be push.
 * 
 * @return The number of bytes pushed.
 */
size_t fifo_push(fifo_t * fifo, uint8_t const * buf, size_t nbyte);

/** 
 * @brief Get and removes the first bytes of the FiFo.
 *
 * @param fifo Points to the FiFo.
 * @param buf Points to the write buffer.
 * @param nbyte The number of byte to pop.
 * 
 *  @note If @p nbyte if bigger than the number used byte in the FiFo, only the
 * used bytes in FiFo will be pop.
 * 
 * @return The number of bytes poped.
 */
size_t fifo_pop(fifo_t * fifo, uint8_t * buf, size_t nbyte);

/** 
 * @brief Add byte to the end of FiFo.
 * 
 * @param fifo Points to the FiFo.
 * @param byte The byte to push.
 * @return @c 1 if byte has push, @c 0 otherwhere.
 */
bool fifo_pushByte(fifo_t * fifo, uint8_t byte);

/** 
 * @brief Get and remove the first byte of FiFo.
 * 
 * @param fifo Points to the FiFo.
 * @param byte Points on a byte where will be write the byte.
 * @return @c 1 if byte has pop, @c 0 otherwhere.
 */
bool fifo_popByte(fifo_t * fifo, uint8_t * byte);

#endif // FIFO_H

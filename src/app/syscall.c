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

// Include ---------------------------------------------------------------------
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Implemented functions -------------------------------------------------------
void *_sbrk(intptr_t increment __attribute__((unused)))
{
    errno = ENOMEM;
    return (void *)(-1);
}

int _close(int fd __attribute__((unused)))
{
    errno = EBADF;
    return -1;
}

int _lseek(
    int fd __attribute__((unused)),
    int ptr __attribute__((unused)),
    int dir __attribute__((unused)))
{
    errno = EBADF;
    return -1;
}

int _read(
    int fd __attribute__((unused)),
    char *ptr __attribute__((unused)),
    int len __attribute__((unused)))
{
    errno = EBADF;
    return -1;
}

int _write(
    int fd __attribute__((unused)),
    char *ptr __attribute__((unused)),
    int len __attribute__((unused)))
{
    errno = EBADF;
    return -1;
}

void _exit(int status __attribute__((unused)))
{
    while(1);
}

int _fstat(
    int fd __attribute__((unused)),
    struct stat *st __attribute__((unused)))
{
    errno = EBADF;
    return -1;
}

int _isatty(int fd __attribute__ ((unused)))
{
    errno = EBADF;
    return -1;
}

int _getpid(int n __attribute__((unused)))
{
    return -1;
}

int _kill (
    int pid __attribute__((unused)), 
    int sig __attribute__((unused)))
{
    return -1;
}
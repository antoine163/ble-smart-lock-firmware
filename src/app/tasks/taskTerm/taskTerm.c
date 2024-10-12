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
 * @file taskTerm.h
 * @author antoine163
 * @date 25-05-2024
 * @brief Task to manage small terminale
 */

// Include ---------------------------------------------------------------------
#include "taskTerm.h"
#include "../taskApp/taskApp.h"
#include "board.h"

#include "vt100.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

// Define ----------------------------------------------------------------------
#define _TASK_TERM_PROMPT      VT100_TEXT_BOLD VT100_COLOR_GREEN "bsl>" VT100_COLOR_RESET
#define _TASK_TERM_MAX(v1, v2) (v1 > v2) ? v1 : v2
#define _TASK_TERM_MIN(v1, v2) (v1 < v2) ? v1 : v2

// Struct ----------------------------------------------------------------------
typedef struct
{
    char *name;
    int (*func)(int, char *[]);
    char *help;
} taskTermCmd_t;

// External functions ----------------------------------------------------------
extern UBaseType_t uxTaskGetSystemState2(
    TaskStatus_t *const pxTaskStatusArray,
    const UBaseType_t uxArraySize,
    configRUN_TIME_COUNTER_TYPE *const pulTotalRunTime);

// Private prototype functions -------------------------------------------------
int _taskTermReadline(char *str, size_t nbyte);

int _taskTermCmdVersion(int argc, char *argv[]);
int _taskTermCmdHelp(int argc, char *argv[]);
int _taskTermCmdVerbose(int argc, char *argv[]);
int _taskTermCmdPin(int argc, char *argv[]);
int _taskTermCmdBri(int argc, char *argv[]);
int _taskTermCmdBriTh(int argc, char *argv[]);
int _taskTermCmdConfig(int argc, char *argv[]);
int _taskTermCmdBonded(int argc, char *argv[]);
int _taskTermCmdBondedClear(int argc, char *argv[]);
int _taskTermCmdReset(int argc, char *argv[]);
int _taskTermCmdTop(int argc, char *argv[]);

// Global variables ------------------------------------------------------------
static taskTermCmd_t _taskTermCmd[] = {
    {
        .name = "version",
        .help = "Display version informations.",
        .func = _taskTermCmdVersion,
    },
    {
        .name = "help",
        .help = "Display help.",
        .func = _taskTermCmdHelp,
    },
    {
        .name = "verbose",
        .help = "Read/write verbose mode. 1 to enable, 0 (default) to disable.",
        .func = _taskTermCmdVerbose,
    },
    {
        .name = "pin",
        .help = "Read/write PIN code (default is 215426).",
        .func = _taskTermCmdPin,
    },
    {
        .name = "bri",
        .help = "Read ambient brightness.",
        .func = _taskTermCmdBri,
    },
    {
        .name = "bri-th",
        .help = "Read/write day/night threshold (default is 50%).",
        .func = _taskTermCmdBriTh,
    },
    {
        .name = "config",
        .help = "Show all configuration.",
        .func = _taskTermCmdConfig,
    },
    {
        .name = "bonded",
        .help = "Display the list of paired devices.",
        .func = _taskTermCmdBonded,
    },
    {
        .name = "bonded-clear",
        .help = "Remove all paired devices (or hold the bond button for more than 3 seconds).",
        .func = _taskTermCmdBondedClear,
    },
    {
        .name = "reset",
        .help = "Reset configuration to default.",
        .func = _taskTermCmdReset,
    },
    {
        .name = "top",
        .help = "Show tasks state.",
        .func = _taskTermCmdTop,
    }};

// Implemented functions -------------------------------------------------------
void taskTermCodeInit()
{
}

void taskTermCode(__attribute__((unused)) void *parameters)
{
    char buf[64];
    int argc;
    char *argv[4];

    while (1)
    {
        int n = _taskTermReadline(buf, sizeof(buf));
        if (n <= 0)
            continue;
        buf[n] = '\0';
        argc = 0;

        // Extra command name
        char *strChr = buf;
        argv[argc++] = strChr;

        // Extra others arguments
        while (1)
        {
            strChr = strchr(strChr, ' ');
            if (strChr == NULL)
                break;

            *strChr = '\0';
            argv[argc++] = ++strChr;
        }

        // Find and execute commande
        unsigned int i = 0;
        for (; i < sizeof(_taskTermCmd) / sizeof(taskTermCmd_t); i++)
        {
            if (strcasecmp(argv[0], _taskTermCmd[i].name) == 0)
            {
                _taskTermCmd[i].func(argc, argv);
                break;
            }
        }

        if (i >= sizeof(_taskTermCmd) / sizeof(taskTermCmd_t))
        {
            boardPrintf("'%s' unknown command! Please use 'help'.\r\n", argv[0]);
        }
    }
}

int _taskTermReadline(char *str, size_t nbyte)
{
    unsigned int str_n = 0;
    unsigned int str_i = 0;
    str[0] = '\0';

    bool vt100_escape = false;
    char vt100_cmd[8];
    unsigned int vt100_n;

    boardPrintf(_TASK_TERM_PROMPT);

    do
    {
        char c = boardReadChar(MAX_TIMEOUT);

        if (vt100_escape == true)
        {
            vt100_cmd[vt100_n++] = c;
            if (vt100_n > sizeof(vt100_cmd))
                vt100_escape = false; // vt100 cmd tool long

            if (strcmp(vt100_cmd, VT100_CURSOR_LEFT()) == 0) // <-
            {
                if (str_i <= 0)
                    continue;

                str_i--;
                boardPrintf(VT100_CURSOR_LEFT());
            }
            else if (strcmp(vt100_cmd, VT100_CURSOR_RIGHT()) == 0) // ->
            {
                if (str_i >= str_n)
                    continue;

                str_i++;
                boardPrintf(VT100_CURSOR_RIGHT());
            }
            else if (strcmp(vt100_cmd, VT100_ESC "[3~") == 0) // Supp key
            {
                if (str_n <= str_i)
                    continue;

                str_n--;

                for (unsigned int cp_i = str_i + 1; cp_i < str_n; cp_i++)
                    str[cp_i] = str[cp_i + 1];

                str[str_n] = '\0';

                boardPrintf(VT100_HIDE_CURSOR VT100_SAVE_CURSOR);
                boardPrintf(VT100_CLEAR_LINE_FROM_CURSOR);
                boardPrintf("%s", str + str_i);
                boardPrintf(VT100_RESTORE_CURSOR VT100_SHOW_CURSOR);
            }
            else
                continue;

            // End of vt100 cmd
            vt100_escape = false;
        }
        else if (c == VT100_ESC[0])
        {
            // Start vt100 cmd
            vt100_escape = true;
            memset(vt100_cmd, 0, sizeof(vt100_cmd));
            vt100_n = 0;

            vt100_cmd[vt100_n++] = c;
        }
        else if (c == '\b') // backspace
        {
            if (str_i <= 0)
                continue;

            str_i--;
            str_n--;

            for (unsigned int cp_i = str_i; cp_i < str_n; cp_i++)
                str[cp_i] = str[cp_i + 1];

            str[str_n] = '\0';

            boardPrintf(VT100_HIDE_CURSOR VT100_CURSOR_LEFT());
            boardPrintf(VT100_SAVE_CURSOR VT100_CLEAR_LINE_FROM_CURSOR);
            boardPrintf("%s", str + str_i);
            boardPrintf(VT100_RESTORE_CURSOR VT100_SHOW_CURSOR);
        }
        else if (c == '\f') // clear ctrl + L
        {
            boardPrintf(VT100_CLEAR_SCREEN VT100_RESET);
            boardPrintf(_TASK_TERM_PROMPT "%s", str);
        }
        else if (c == '\t') // TAB
        {
            if (str_n <= 0)
                continue;

            for (unsigned int i = 0; i < sizeof(_taskTermCmd) / sizeof(taskTermCmd_t); i++)
            {
                if (strstr(_taskTermCmd[i].name, str) != NULL)
                    boardPrintf("\r\n%s", _taskTermCmd[i].name);
            }

            boardPrintf("\r\n" _TASK_TERM_PROMPT "%s", str);
            if ((str_n - str_i) > 0)
                boardPrintf(VT100_CURSOR_LEFT(% u), str_n - str_i);
        }
        else if (c == '\r')
        {
            boardPrintf("\r\n");
            break; // exit boardReadline() function
        }
        else if (c == '\n')
        {
            continue;
        }
        else
        {
            if (str_n >= nbyte)
                continue;

            for (unsigned int cp_i = str_n; cp_i > str_i; cp_i--)
                str[cp_i] = str[cp_i - 1];

            str[str_i] = c;

            str_i++;
            str_n++;
            str[str_n] = '\0';

            boardPrintf(VT100_HIDE_CURSOR VT100_SAVE_CURSOR);
            boardPrintf("%s", str + str_i - 1);
            boardPrintf(VT100_RESTORE_CURSOR);
            boardPrintf(VT100_CURSOR_RIGHT() VT100_SHOW_CURSOR);
        }
    } while (1);

    str[str_n] = '\0';
    return str_n;
}

int _taskTermCmdVersion(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    boardPrintf("%s - %s\r\n", PROJECT_VERSION, __DATE__);
    return 0;
}

int _taskTermCmdHelp(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    for (unsigned int i = 0; i < sizeof(_taskTermCmd) / sizeof(taskTermCmd_t); i++)
    {
        boardPrintf(VT100_TEXT_BOLD "%-14s: " VT100_COLOR_RESET, _taskTermCmd[i].name);
        boardPrintf("%s\r\n", _taskTermCmd[i].help);
    }
    return EXIT_SUCCESS;
}

int _taskTermCmdVerbose(int argc, char *argv[])
{
    bool verbose = false;

    if (argc == 1)
        verbose = taskAppGetVerbose();
    else if (argc == 2)
    {
        if ((strcasecmp("enable", argv[1]) == 0) ||
            (strcasecmp("1", argv[1]) == 0))
        {
            verbose = true;
            taskAppSetVerbose(true);
        }
        else if ((strcasecmp("disable", argv[1]) == 0) ||
                 (strcasecmp("0", argv[1]) == 0))
        {
            verbose = false;
            taskAppSetVerbose(false);
        }
        else
        {
            boardPrintf("Invalid argument. Use 'enable' or 'disable'\r\n");
            return EXIT_FAILURE;
        }
    }
    else
    {
        boardPrintf("Error: Invalid number of arguments!\r\n");
        return EXIT_FAILURE;
    }

    boardPrintf("Verbose: %s\r\n", verbose ? "Enabled" : "Disabled");
    return EXIT_SUCCESS;
}

int _taskTermCmdPin(int argc, char *argv[])
{
    unsigned int pin;

    if (argc == 1)
        pin = taskAppGetPin();
    else if (argc == 2)
    {
        int n = sscanf(argv[1], "%u", &pin);
        if (n == 0)
        {
            boardPrintf("Error: Input must be a number!\r\n");
            return EXIT_FAILURE;
        }
        else if (taskAppSetPin(pin) < 0)
        {
            boardPrintf("Error: Pin must be 0-999999\r\n");
            return EXIT_FAILURE;
        }
    }
    else
    {
        boardPrintf("Error: Invalid number of arguments!\r\n");
        return EXIT_FAILURE;
    }

    boardPrintf("Pin: %06u\r\n", pin);
    return EXIT_SUCCESS;
}

int _taskTermCmdBri(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    float bri = boardGetBrightness() + 0.05; // 0.05 for round
    unsigned int briInt = (int)bri;
    unsigned int briFrac = (bri - (float)briInt) * 10;
    boardPrintf("Brightness: %u.%u%%\r\n", briInt, briFrac);
    return EXIT_SUCCESS;
}

int _taskTermCmdBriTh(int argc, char *argv[])
{
    float briTh = 0;
    if (argc == 1)
        briTh = taskAppGetBrightnessTh();
    else if (argc == 2)
    {
        // Tronc frac value at .x
        char *dotptr = strrchr(argv[1], '.');
        if ((dotptr != NULL) && (dotptr[1] != '\0'))
            dotptr[2] = '\0';

        unsigned int briThInt = 0;
        unsigned int briThFrac = 0;
        int n = sscanf(argv[1], "%u.%u", &briThInt, &briThFrac);
        if ((n < 1) || (n > 2))
        {
            boardPrintf("Error: Invalid input format!\r\n");
            return EXIT_FAILURE;
        }

        briTh = (float)briThInt + (float)briThFrac / 10.f;
        if (taskAppSetBrightnessTh(briTh) != 0)
        {
            boardPrintf("Error: Brightness threshold must be 0.0%% to 100.0%%\r\n");
            return EXIT_FAILURE;
        }
    }
    else
    {
        boardPrintf("Error: Invalid number of arguments!\r\n");
        return EXIT_FAILURE;
    }

    briTh += 0.05; // Round
    unsigned int briInt = (int)briTh;
    unsigned int briFrac = (briTh - (float)briInt) * 10;
    boardPrintf("Brightness threshold: %u.%u%%\r\n", briInt, briFrac);

    return EXIT_SUCCESS;
}

int _taskTermCmdConfig(int argc, char *argv[])
{
    if (argc == 1)
    {
        _taskTermCmdVerbose(1, argv);
        _taskTermCmdPin(1, argv);
        _taskTermCmdBriTh(1, argv);
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

int _taskTermCmdBonded(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    Bonded_Device_Entry_t devices[MAX_NUM_BONDED_DEVICES];
    int n = taskBleGetBonded(devices);

    if (n <= 0)
        boardPrintf("No device bonded!\r\n");
    else
        boardPrintf("       Address    Type\r\n");

    for (int i = 0; i < n; i++)
    {
        boardPrintf("0x%02x%02x%02x%02x%02x%02x    ",
                    devices[i].Address[5],
                    devices[i].Address[4],
                    devices[i].Address[3],
                    devices[i].Address[2],
                    devices[i].Address[1],
                    devices[i].Address[0]);

        switch (devices[i].Address_Type)
        {
        case 0x00: boardPrintf("Public"); break;
        case 0x01: boardPrintf("Random"); break;
        default:   break;
        }
        boardPrintf("\r\n");
    }

    return EXIT_SUCCESS;
}

int _taskTermCmdBondedClear(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    taskBleClearAllPairing();
    boardPrintf("Clearing bonded devices.\r\n");
    vTaskDelay(400 / portTICK_PERIOD_MS); // wait to clear bonded devices.
    boardReset();

    return EXIT_SUCCESS;
}

int _taskTermCmdReset(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    taskAppResetConfig();
    taskBleClearAllPairing();
    boardPrintf("Reseting config and clearing bonded devices.\r\n");
    vTaskDelay(400 / portTICK_PERIOD_MS); // Wait to write default config and clear bonded devices.
    boardReset();

    return EXIT_SUCCESS;
}

void printTaskInfo(TaskStatus_t *taskStatus, unsigned long totalRunTime)
{
    if (taskStatus->eCurrentState == eRunning)
        boardPrintf(VT100_TEXT_BOLD);

    // Task Name
    boardPrintf(" %9s ", taskStatus->pcTaskName);

    // State
    switch (taskStatus->eCurrentState)
    {
    case eRunning:   boardPrintf("  Running   "); break;
    case eReady:     boardPrintf("  Ready     "); break;
    case eBlocked:   boardPrintf("  Blocked   "); break;
    case eSuspended: boardPrintf("  Suspended "); break;
    case eDeleted:   boardPrintf("  Deleted   "); break;
    default:         boardPrintf("  Unknown   "); break;
    }

    // Priority
    boardPrintf("  %-8u ", taskStatus->uxCurrentPriority);

    // Stack Free
    boardPrintf("  %-10u ", taskStatus->usStackHighWaterMark);

    // CPU Usage
    float cpuUsage = 0.f;
    if (totalRunTime > 0)
    {
        cpuUsage = (float)taskStatus->ulRunTimeCounter * 100.f / (float)totalRunTime;
    }
    int cpuUsageInt = (int)cpuUsage;
    int cpuUsageFrac = (cpuUsage - (float)cpuUsageInt) * 10;
    boardPrintf("  %u.%u%%", cpuUsageInt, cpuUsageFrac);

    boardPrintf(VT100_COLOR_RESET "\r\n" VT100_CLEAR_LINE_FROM_CURSOR);
}

void printTasksInfo()
{
    TaskStatus_t taskStatusArray[6];
    TaskStatus_t *taskStatusArrayStor[6];
    UBaseType_t taskCount;
    unsigned long totalRunTime;

    // Get the number of tasks
    taskCount = uxTaskGetNumberOfTasks();

    // Get the task state
    taskCount = uxTaskGetSystemState2(
        taskStatusArray,
        sizeof(taskStatusArray) / sizeof(TaskStatus_t),
        &totalRunTime);

    // Stor by CPU Usage
    for (unsigned int i = 0; i < taskCount; i++)
        taskStatusArrayStor[i] = &taskStatusArray[i];

    for (unsigned int i = 0; i < taskCount - 1; i++)
    {
        if (taskStatusArrayStor[i]->ulRunTimeCounter < taskStatusArrayStor[i + 1]->ulRunTimeCounter)
        {
            TaskStatus_t *swap = taskStatusArrayStor[i];
            taskStatusArrayStor[i] = taskStatusArrayStor[i + 1];
            taskStatusArrayStor[i + 1] = swap;

            for (unsigned int ii = i; ii > 0; ii--)
            {
                if (taskStatusArrayStor[ii - 1]->ulRunTimeCounter < taskStatusArrayStor[ii]->ulRunTimeCounter)
                {
                    TaskStatus_t *swap = taskStatusArrayStor[ii];
                    taskStatusArrayStor[ii] = taskStatusArrayStor[ii - 1];
                    taskStatusArrayStor[ii - 1] = swap;
                }
            }
        }
    }

    // Print header
    boardPrintf(" Task Name   State       Priority   Stack Free   CPU Usage\r\n");

    // Iterate over all tasks and print their information
    for (UBaseType_t i = 0; i < taskCount; i++)
        printTaskInfo(taskStatusArrayStor[i], totalRunTime);
}

int _taskTermCmdTop(
    __attribute__((unused)) int argc,
    __attribute__((unused)) char *argv[])
{
    boardPrintf(VT100_CLEAR_SCREEN);

    for (;;)
    {
        boardPrintf(VT100_HIDE_CURSOR VT100_CURSOR_HOME);
        printTasksInfo();
        boardPrintf(" 'Ctrl + C' to quit.");
        boardPrintf(VT100_SHOW_CURSOR);

        char c = boardReadChar(1000);
        if (c == 3) // c == Ctrl + C ?
            break;
    }

    boardPrintf("\r\n");
    return EXIT_SUCCESS;
}

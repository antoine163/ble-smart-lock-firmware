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
 * @file freertos_tasks_c_additions.h
 * @author antoine163
 * @date 31-05-2024
 */

#ifndef FREERTOS_TASKS_C_ADDITIONS_H
#define FREERTOS_TASKS_C_ADDITIONS_H

// Functions -------------------------------------------------------------------

uint32_t lastSystemState2Time = 0;

UBaseType_t uxTaskGetSystemState2(
    TaskStatus_t *const pxTaskStatusArray,
    const UBaseType_t uxArraySize,
    configRUN_TIME_COUNTER_TYPE *const pulTotalRunTime)
{
    UBaseType_t uxTask = 0, uxQueue = configMAX_PRIORITIES;

    traceENTER_uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, pulTotalRunTime);

    vTaskSuspendAll();
    {
        /* Is there a space in the array for each task in the system? */
        if (uxArraySize >= uxCurrentNumberOfTasks)
        {
            /* Fill in an TaskStatus_t structure with information on each
             * task in the Ready state. */
            do
            {
                uxQueue--;
                uxTask = (UBaseType_t)(uxTask + prvListTasksWithinSingleList(&(pxTaskStatusArray[uxTask]), &(pxReadyTasksLists[uxQueue]), eReady));
            } while (uxQueue > (UBaseType_t)tskIDLE_PRIORITY);

            /* Fill in an TaskStatus_t structure with information on each
             * task in the Blocked state. */
            uxTask = (UBaseType_t)(uxTask + prvListTasksWithinSingleList(&(pxTaskStatusArray[uxTask]), (List_t *)pxDelayedTaskList, eBlocked));
            uxTask = (UBaseType_t)(uxTask + prvListTasksWithinSingleList(&(pxTaskStatusArray[uxTask]), (List_t *)pxOverflowDelayedTaskList, eBlocked));

#if (INCLUDE_vTaskDelete == 1)
            {
                /* Fill in an TaskStatus_t structure with information on
                 * each task that has been deleted but not yet cleaned up. */
                uxTask = (UBaseType_t)(uxTask + prvListTasksWithinSingleList(&(pxTaskStatusArray[uxTask]), &xTasksWaitingTermination, eDeleted));
            }
#endif

#if (INCLUDE_vTaskSuspend == 1)
            {
                /* Fill in an TaskStatus_t structure with information on
                 * each task in the Suspended state. */
                uxTask = (UBaseType_t)(uxTask + prvListTasksWithinSingleList(&(pxTaskStatusArray[uxTask]), &xSuspendedTaskList, eSuspended));
            }
#endif

#if (configGENERATE_RUN_TIME_STATS == 1)
            {
                if (pulTotalRunTime != NULL)
                {
#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                    portALT_GET_RUN_TIME_COUNTER_VALUE((*pulTotalRunTime));
#else
                    *pulTotalRunTime = (configRUN_TIME_COUNTER_TYPE)portGET_RUN_TIME_COUNTER_VALUE();
#endif
                }
            }
#else  /* if ( configGENERATE_RUN_TIME_STATS == 1 ) */
            {
                if (pulTotalRunTime != NULL)
                {
                    *pulTotalRunTime = 0;
                }
            }
#endif /* if ( configGENERATE_RUN_TIME_STATS == 1 ) */
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        // Reset tasks statistics
        ulTaskSwitchedInTime[0] = 0;
        lastSystemState2Time = HAL_VTimerGetCurrentTime_sysT32();
        for (unsigned int i = 0; i < uxTask; i++)
            pxTaskStatusArray[i].xHandle->ulRunTimeCounter = 0;
    }
    (void)xTaskResumeAll();

    traceRETURN_uxTaskGetSystemState(uxTask);

    return uxTask;
}

#endif // FREERTOS_TASKS_C_ADDITIONS_H

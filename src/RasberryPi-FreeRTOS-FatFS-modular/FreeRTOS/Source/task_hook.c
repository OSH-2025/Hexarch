#include "FreeRTOS.h"
#include "task.h"

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

void vTaskSetApplicationTaskTag( xTaskHandle xTask, pdTASK_HOOK_CODE pxHookFunction )
{
    tskTCB *xTCB;

    /* If xTask is NULL then we are setting our own task hook. */
    if( xTask == NULL )
    {
        xTCB = ( tskTCB * ) pxCurrentTCB;
    }
    else
    {
        xTCB = ( tskTCB * ) xTask;
    }

    /* Save the hook function in the TCB.  A critical section is required as
    the value can be accessed from an interrupt. */
    taskENTER_CRITICAL();
        xTCB->pxTaskTag = pxHookFunction;
    taskEXIT_CRITICAL();
}

pdTASK_HOOK_CODE xTaskGetApplicationTaskTag( xTaskHandle xTask )
{
    tskTCB *xTCB;
    pdTASK_HOOK_CODE xReturn;

    /* If xTask is NULL then we are setting our own task hook. */
    if( xTask == NULL )
    {
        xTCB = ( tskTCB * ) pxCurrentTCB;
    }
    else
    {
        xTCB = ( tskTCB * ) xTask;
    }

    /* Save the hook function in the TCB.  A critical section is required as
    the value can be accessed from an interrupt. */
    taskENTER_CRITICAL();
        xReturn = xTCB->pxTaskTag;
    taskEXIT_CRITICAL();

    return xReturn;
}

portBASE_TYPE xTaskCallApplicationTaskHook( xTaskHandle xTask, void *pvParameter )
{
    tskTCB *xTCB;
    portBASE_TYPE xReturn;

    /* If xTask is NULL then we are calling our own task hook. */
    if( xTask == NULL )
    {
        xTCB = ( tskTCB * ) pxCurrentTCB;
    }
    else
    {
        xTCB = ( tskTCB * ) xTask;
    }

    if( xTCB->pxTaskTag != NULL )
    {
        xReturn = xTCB->pxTaskTag( pvParameter );
    }
    else
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

#endif 
#include "FreeRTOS.h"
#include "task.h"
#include "list.h"

void vTaskStartScheduler( void )
{
    portBASE_TYPE xReturn;
    /* 初始化调度器状态 */
    xSchedulerRunning = pdTRUE;
    xReturn = xPortStartScheduler();
    /* 只有在调度器无法启动时才会到达这里 */
    configASSERT( xReturn );
}

void vTaskEndScheduler( void )
{
    portDISABLE_INTERRUPTS();
    xSchedulerRunning = pdFALSE;
    vPortEndScheduler();
}

void vTaskSuspendAll( void )
{
    ++uxSchedulerSuspended;
}

signed portBASE_TYPE xTaskResumeAll( void )
{
    register tskTCB *pxTCB;
    signed portBASE_TYPE xAlreadyYielded = pdFALSE;
    configASSERT( uxSchedulerSuspended );
    taskENTER_CRITICAL();
    {
        --uxSchedulerSuspended;
        if( uxSchedulerSuspended == ( unsigned portBASE_TYPE ) pdFALSE )
        {
            if( uxCurrentNumberOfTasks > ( unsigned portBASE_TYPE ) 0U )
            {
                portBASE_TYPE xYieldRequired = pdFALSE;
                while( listLIST_IS_EMPTY( ( xList * ) &xPendingReadyList ) == pdFALSE )
                {
                    pxTCB = ( tskTCB * ) listGET_OWNER_OF_HEAD_ENTRY(  ( ( xList * ) &xPendingReadyList ) );
                    vListRemove( &( pxTCB->xEventListItem ) );
                    vListRemove( &( pxTCB->xGenericListItem ) );
                    prvAddTaskToReadyQueue( pxTCB );
                    if( pxTCB->uxPriority >= pxCurrentTCB->uxPriority )
                    {
                        xYieldRequired = pdTRUE;
                    }
                }
                /* 处理挂起期间丢失的tick等（略） */
                if( xYieldRequired != pdFALSE )
                {
                    xAlreadyYielded = pdTRUE;
                    portYIELD_WITHIN_API();
                }
            }
        }
    }
    taskEXIT_CRITICAL();
    return xAlreadyYielded;
}

#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )
portBASE_TYPE xTaskGetSchedulerState( void )
{
    portBASE_TYPE xReturn;
    if( xSchedulerRunning == pdFALSE )
    {
        xReturn = taskSCHEDULER_NOT_STARTED;
    }
    else
    {
        if( uxSchedulerSuspended == ( unsigned portBASE_TYPE ) pdFALSE )
        {
            xReturn = taskSCHEDULER_RUNNING;
        }
        else
        {
            xReturn = taskSCHEDULER_SUSPENDED;
        }
    }
    return xReturn;
}
#endif 
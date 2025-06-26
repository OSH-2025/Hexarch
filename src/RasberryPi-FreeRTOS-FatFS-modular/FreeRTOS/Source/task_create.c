#include "FreeRTOS.h"
#include "task.h"
#include "StackMacros.h"
#include <string.h>

// 迁移自tasks.c的xTaskGenericCreate实现
signed portBASE_TYPE xTaskGenericCreate(
    pdTASK_CODE pxTaskCode,
    const signed char * const pcName,
    unsigned short usStackDepth,
    void *pvParameters,
    unsigned portBASE_TYPE uxPriority,
    xTaskHandle *pxCreatedTask,
    portSTACK_TYPE *puxStackBuffer,
    const xMemoryRegion * const xRegions)
{
    signed portBASE_TYPE xReturn;
    tskTCB * pxNewTCB;

    configASSERT( pxTaskCode );
    configASSERT( ( ( uxPriority & ( ~portPRIVILEGE_BIT ) ) < configMAX_PRIORITIES ) );

    // 分配TCB和堆栈
    pxNewTCB = prvAllocateTCBAndStack( usStackDepth, puxStackBuffer );

    if( pxNewTCB != NULL )
    {
        portSTACK_TYPE *pxTopOfStack;

        #if( portUSING_MPU_WRAPPERS == 1 )
            portBASE_TYPE xRunPrivileged;
            if( ( uxPriority & portPRIVILEGE_BIT ) != 0U )
            {
                xRunPrivileged = pdTRUE;
            }
            else
            {
                xRunPrivileged = pdFALSE;
            }
            uxPriority &= ~portPRIVILEGE_BIT;
        #endif

        #if( portSTACK_GROWTH < 0 )
        {
            pxTopOfStack = pxNewTCB->pxStack + ( usStackDepth - ( unsigned short ) 1 );
            pxTopOfStack = ( portSTACK_TYPE * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ( portPOINTER_SIZE_TYPE ) ~portBYTE_ALIGNMENT_MASK  ) );
            configASSERT( ( ( ( unsigned long ) pxTopOfStack & ( unsigned long ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
        }
        #else
        {
            pxTopOfStack = pxNewTCB->pxStack;
            configASSERT( ( ( ( unsigned long ) pxNewTCB->pxStack & ( unsigned long ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
            pxNewTCB->pxEndOfStack = pxNewTCB->pxStack + ( usStackDepth - 1 );
        }
        #endif

        prvInitialiseTCBVariables( pxNewTCB, pcName, uxPriority, xRegions, usStackDepth );

        #if( portUSING_MPU_WRAPPERS == 1 )
        {
            pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters, xRunPrivileged );
        }
        #else
        {
            pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );
        }
        #endif

        portALIGNMENT_ASSERT_pxCurrentTCB( ( ( ( unsigned long ) pxNewTCB->pxTopOfStack & ( unsigned long ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

        if( ( void * ) pxCreatedTask != NULL )
        {
            *pxCreatedTask = ( xTaskHandle ) pxNewTCB;
        }

        taskENTER_CRITICAL();
        {
            uxCurrentNumberOfTasks++;
            if( pxCurrentTCB == NULL )
            {
                pxCurrentTCB =  pxNewTCB;
                if( uxCurrentNumberOfTasks == ( unsigned portBASE_TYPE ) 1 )
                {
                    prvInitialiseTaskLists();
                }
            }
            else
            {
                if( xSchedulerRunning == pdFALSE )
                {
                    if( pxCurrentTCB->uxPriority <= uxPriority )
                    {
                        pxCurrentTCB = pxNewTCB;
                    }
                }
            }
            if( pxNewTCB->uxPriority > uxTopUsedPriority )
            {
                uxTopUsedPriority = pxNewTCB->uxPriority;
            }
            #if ( configUSE_TRACE_FACILITY == 1 )
            {
                pxNewTCB->uxTCBNumber = uxTaskNumber;
            }
            #endif
            uxTaskNumber++;
            prvAddTaskToReadyQueue( pxNewTCB );
            xReturn = pdPASS;
            portSETUP_TCB( pxNewTCB );
            traceTASK_CREATE( pxNewTCB );
        }
        taskEXIT_CRITICAL();
    }
    else
    {
        xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
        traceTASK_CREATE_FAILED();
    }

    if( xReturn == pdPASS )
    {
        if( xSchedulerRunning != pdFALSE )
        {
            if( pxCurrentTCB->uxPriority < uxPriority )
            {
                portYIELD_WITHIN_API();
            }
        }
    }

    return xReturn;
} 
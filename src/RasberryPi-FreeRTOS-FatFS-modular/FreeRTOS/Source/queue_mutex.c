#include "queue.h"
#include "list.h"
#include "task.h"

xQueueHandle xQueueCreateMutex( unsigned char ucQueueType ) PRIVILEGED_FUNCTION;

// 互斥量与信号量相关实现

xQueueHandle xQueueCreateMutex( unsigned char ucQueueType )
{
    xQUEUE *pxNewQueue;
    ( void ) ucQueueType;
    pxNewQueue = ( xQUEUE * ) pvPortMalloc( sizeof( xQUEUE ) );
    if( pxNewQueue != NULL )
    {
        pxNewQueue->pxMutexHolder = NULL;
        pxNewQueue->uxQueueType = queueQUEUE_IS_MUTEX;
        pxNewQueue->pcWriteTo = NULL;
        pxNewQueue->pcReadFrom = NULL;
        pxNewQueue->uxMessagesWaiting = ( unsigned portBASE_TYPE ) 0U;
        pxNewQueue->uxLength = ( unsigned portBASE_TYPE ) 1U;
        pxNewQueue->uxItemSize = ( unsigned portBASE_TYPE ) 0U;
        pxNewQueue->xRxLock = queueUNLOCKED;
        pxNewQueue->xTxLock = queueUNLOCKED;
#if ( configUSE_TRACE_FACILITY == 1 )
        pxNewQueue->ucQueueType = ucQueueType;
#endif
        vListInitialise( &( pxNewQueue->xTasksWaitingToSend ) );
        vListInitialise( &( pxNewQueue->xTasksWaitingToReceive ) );
        traceCREATE_MUTEX( pxNewQueue );
        xQueueGenericSend( pxNewQueue, NULL, ( portTickType ) 0U, queueSEND_TO_BACK );
    }
    else
    {
        traceCREATE_MUTEX_FAILED();
    }
    configASSERT( pxNewQueue );
    return pxNewQueue;
}

#if ( ( configUSE_MUTEXES == 1 ) && ( INCLUDE_xQueueGetMutexHolder == 1 ) )
void* xQueueGetMutexHolder( xQueueHandle xSemaphore )
{
    void *pxReturn;
    taskENTER_CRITICAL();
    {
        if( xSemaphore->uxQueueType == queueQUEUE_IS_MUTEX )
        {
            pxReturn = ( void * ) xSemaphore->pxMutexHolder;
        }
        else
        {
            pxReturn = NULL;
        }
    }
    taskEXIT_CRITICAL();
    return pxReturn;
}
#endif

#if ( configUSE_RECURSIVE_MUTEXES == 1 )
portBASE_TYPE xQueueGiveMutexRecursive( xQueueHandle pxMutex )
{
    portBASE_TYPE xReturn;
    configASSERT( pxMutex );
    if( pxMutex->pxMutexHolder == xTaskGetCurrentTaskHandle() )
    {
        traceGIVE_MUTEX_RECURSIVE( pxMutex );
        ( pxMutex->uxRecursiveCallCount )--;
        if( pxMutex->uxRecursiveCallCount == 0 )
        {
            xQueueGenericSend( pxMutex, NULL, queueMUTEX_GIVE_BLOCK_TIME, queueSEND_TO_BACK );
        }
        xReturn = pdPASS;
    }
    else
    {
        xReturn = pdFAIL;
        traceGIVE_MUTEX_RECURSIVE_FAILED( pxMutex );
    }
    return xReturn;
}

portBASE_TYPE xQueueTakeMutexRecursive( xQueueHandle pxMutex, portTickType xBlockTime )
{
    portBASE_TYPE xReturn;
    configASSERT( pxMutex );
    traceTAKE_MUTEX_RECURSIVE( pxMutex );
    if( pxMutex->pxMutexHolder == xTaskGetCurrentTaskHandle() )
    {
        ( pxMutex->uxRecursiveCallCount )++;
        xReturn = pdPASS;
    }
    else
    {
        xReturn = xQueueGenericReceive( pxMutex, NULL, xBlockTime, pdFALSE );
        if( xReturn == pdPASS )
        {
            ( pxMutex->uxRecursiveCallCount )++;
        }
        else
        {
            traceTAKE_MUTEX_RECURSIVE_FAILED( pxMutex );
        }
    }
    return xReturn;
}
#endif

#if configUSE_COUNTING_SEMAPHORES == 1
xQueueHandle xQueueCreateCountingSemaphore( unsigned portBASE_TYPE uxCountValue, unsigned portBASE_TYPE uxInitialCount )
{
    xQueueHandle pxHandle;
    pxHandle = xQueueGenericCreate( ( unsigned portBASE_TYPE ) uxCountValue, queueSEMAPHORE_QUEUE_ITEM_LENGTH, queueQUEUE_TYPE_COUNTING_SEMAPHORE );
    if( pxHandle != NULL )
    {
        pxHandle->uxMessagesWaiting = uxInitialCount;
        traceCREATE_COUNTING_SEMAPHORE();
    }
    else
    {
        traceCREATE_COUNTING_SEMAPHORE_FAILED();
    }
    configASSERT( pxHandle );
    return pxHandle;
}
#endif




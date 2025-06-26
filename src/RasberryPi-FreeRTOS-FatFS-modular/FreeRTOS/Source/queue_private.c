#include "queue.h"
#include "list.h"
#include "task.h"
#include <string.h>

/*
 * Copies an item into the queue, either at the front of the queue or the
 * back of the queue.
 */
static void prvCopyDataToQueue( xQUEUE *pxQueue, const void *pvItemToQueue, portBASE_TYPE xPosition ) PRIVILEGED_FUNCTION;
/*
 * Copies an item out of a queue.
 */
static void prvCopyDataFromQueue( xQUEUE * const pxQueue, const void *pvBuffer ) PRIVILEGED_FUNCTION;
/*
 * Unlocks a queue locked by a call to prvLockQueue.  Locking a queue does not
 * prevent an ISR from adding or removing items to the queue, but does prevent
 * an ISR from removing tasks from the queue event lists.  If an ISR finds a
 * queue is locked it will instead increment the appropriate queue lock count
 * to indicate that a task may require unblocking.  When the queue in unlocked
 * these lock counts are inspected, and the appropriate action taken.
 */
static void prvUnlockQueue( xQueueHandle pxQueue ) PRIVILEGED_FUNCTION;
/*
 * Macro to mark a queue as locked.  Locking a queue prevents an ISR from
 * accessing the queue event lists.
 */
#define prvLockQueue( pxQueue )								\
	taskENTER_CRITICAL();									\
	{														\
		if( ( pxQueue )->xRxLock == queueUNLOCKED )			\
		{													\
			( pxQueue )->xRxLock = queueLOCKED_UNMODIFIED;	\
		}													\
		if( ( pxQueue )->xTxLock == queueUNLOCKED )			\
		{													\
			( pxQueue )->xTxLock = queueLOCKED_UNMODIFIED;	\
		}													\
	}														\
	taskEXIT_CRITICAL()
/*-----------------------------------------------------------*/
/*
 * Uses a critical section to determine if there is any space in a queue.
 *
 * @return pdTRUE if there is no space, otherwise pdFALSE;
 */
static signed portBASE_TYPE prvIsQueueFull( const xQueueHandle pxQueue ) PRIVILEGED_FUNCTION;
/*
 * Uses a critical section to determine if there is any data in a queue.
 *
 * @return pdTRUE if the queue contains no items, otherwise pdFALSE.
 */
static signed portBASE_TYPE prvIsQueueEmpty( const xQueueHandle pxQueue ) PRIVILEGED_FUNCTION;

static void prvCopyDataToQueue( xQUEUE *pxQueue, const void *pvItemToQueue, portBASE_TYPE xPosition )
{
    if( pxQueue->uxItemSize == ( unsigned portBASE_TYPE ) 0 )
    {
#if ( configUSE_MUTEXES == 1 )
        if( pxQueue->uxQueueType == queueQUEUE_IS_MUTEX )
        {
            vTaskPriorityDisinherit( ( void * ) pxQueue->pxMutexHolder );
            pxQueue->pxMutexHolder = NULL;
        }
#endif
    }
    else if( xPosition == queueSEND_TO_BACK )
    {
        memcpy( ( void * ) pxQueue->pcWriteTo, pvItemToQueue, ( unsigned ) pxQueue->uxItemSize );
        pxQueue->pcWriteTo += pxQueue->uxItemSize;
        if( pxQueue->pcWriteTo >= pxQueue->pcTail )
        {
            pxQueue->pcWriteTo = pxQueue->pcHead;
        }
    }
    else
    {
        memcpy( ( void * ) pxQueue->pcReadFrom, pvItemToQueue, ( unsigned ) pxQueue->uxItemSize );
        pxQueue->pcReadFrom -= pxQueue->uxItemSize;
        if( pxQueue->pcReadFrom < pxQueue->pcHead )
        {
            pxQueue->pcReadFrom = ( pxQueue->pcTail - pxQueue->uxItemSize );
        }
    }
    ++( pxQueue->uxMessagesWaiting );
}

static void prvCopyDataFromQueue( xQUEUE * const pxQueue, const void *pvBuffer )
{
    if( pxQueue->uxQueueType != queueQUEUE_IS_MUTEX )
    {
        pxQueue->pcReadFrom += pxQueue->uxItemSize;
        if( pxQueue->pcReadFrom >= pxQueue->pcTail )
        {
            pxQueue->pcReadFrom = pxQueue->pcHead;
        }
        memcpy( ( void * ) pvBuffer, ( void * ) pxQueue->pcReadFrom, ( unsigned ) pxQueue->uxItemSize );
    }
}

static void prvUnlockQueue( xQueueHandle pxQueue )
{
    taskENTER_CRITICAL();
    while( pxQueue->xTxLock > queueLOCKED_UNMODIFIED )
    {
        if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
        {
            if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
            {
                vTaskMissedYield();
            }
            --( pxQueue->xTxLock );
        }
        else
        {
            break;
        }
    }
    pxQueue->xTxLock = queueUNLOCKED;
    taskEXIT_CRITICAL();

    taskENTER_CRITICAL();
    while( pxQueue->xRxLock > queueLOCKED_UNMODIFIED )
    {
        if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
        {
            if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
            {
                vTaskMissedYield();
            }
            --( pxQueue->xRxLock );
        }
        else
        {
            break;
        }
    }
    pxQueue->xRxLock = queueUNLOCKED;
    taskEXIT_CRITICAL();
}

static signed portBASE_TYPE prvIsQueueEmpty( const xQueueHandle pxQueue )
{
    signed portBASE_TYPE xReturn;
    taskENTER_CRITICAL();
    xReturn = ( pxQueue->uxMessagesWaiting == ( unsigned portBASE_TYPE ) 0 );
    taskEXIT_CRITICAL();
    return xReturn;
}

static signed portBASE_TYPE prvIsQueueFull( const xQueueHandle pxQueue )
{
    signed portBASE_TYPE xReturn;
    taskENTER_CRITICAL();
    xReturn = ( pxQueue->uxMessagesWaiting == pxQueue->uxLength );
    taskEXIT_CRITICAL();
    return xReturn;
}
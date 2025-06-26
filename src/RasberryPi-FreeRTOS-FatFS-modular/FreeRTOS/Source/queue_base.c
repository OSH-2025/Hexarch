#include "queue.h"
#include "list.h"
#include "task.h"
#include <stdlib.h>
#include <string.h>

// 私有结构体定义
typedef struct QueueDefinition
{
    signed char *pcHead;
    signed char *pcTail;
    signed char *pcWriteTo;
    signed char *pcReadFrom;
    xList xTasksWaitingToSend;
    xList xTasksWaitingToReceive;
    volatile unsigned portBASE_TYPE uxMessagesWaiting;
    unsigned portBASE_TYPE uxLength;
    unsigned portBASE_TYPE uxItemSize;
    volatile signed portBASE_TYPE xRxLock;
    volatile signed portBASE_TYPE xTxLock;
#if ( configUSE_TRACE_FACILITY == 1 )
    unsigned char ucQueueNumber;
    unsigned char ucQueueType;
#endif
} xQUEUE;

typedef xQUEUE * xQueueHandle;

// 私有函数声明
extern void prvCopyDataToQueue( xQUEUE *pxQueue, const void *pvItemToQueue, portBASE_TYPE xPosition );
extern signed portBASE_TYPE prvIsQueueFull( const xQueueHandle pxQueue );
extern void prvUnlockQueue( xQueueHandle pxQueue );
extern void prvLockQueue( xQueueHandle pxQueue );
extern void prvCopyDataFromQueue( xQUEUE * const pxQueue, const void *pvBuffer );

// 基本操作实现
portBASE_TYPE xQueueGenericReset( xQueueHandle pxQueue, portBASE_TYPE xNewQueue )
{
    configASSERT( pxQueue );
    taskENTER_CRITICAL();
    {
        pxQueue->pcTail = pxQueue->pcHead + ( pxQueue->uxLength * pxQueue->uxItemSize );
        pxQueue->uxMessagesWaiting = ( unsigned portBASE_TYPE ) 0U;
        pxQueue->pcWriteTo = pxQueue->pcHead;
        pxQueue->pcReadFrom = pxQueue->pcHead + ( ( pxQueue->uxLength - ( unsigned portBASE_TYPE ) 1U ) * pxQueue->uxItemSize );
        pxQueue->xRxLock = queueUNLOCKED;
        pxQueue->xTxLock = queueUNLOCKED;
        if( xNewQueue == pdFALSE )
        {
            if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
            {
                if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) == pdTRUE )
                {
                    portYIELD_WITHIN_API();
                }
            }
        }
        else
        {
            vListInitialise( &( pxQueue->xTasksWaitingToSend ) );
            vListInitialise( &( pxQueue->xTasksWaitingToReceive ) );
        }
    }
    taskEXIT_CRITICAL();
    return pdPASS;
}

xQueueHandle xQueueGenericCreate( unsigned portBASE_TYPE uxQueueLength, unsigned portBASE_TYPE uxItemSize, unsigned char ucQueueType )
{
    xQUEUE *pxNewQueue;
    size_t xQueueSizeInBytes;
    xQueueHandle xReturn = NULL;
    ( void ) ucQueueType;
    if( uxQueueLength > ( unsigned portBASE_TYPE ) 0 )
    {
        pxNewQueue = ( xQUEUE * ) pvPortMalloc( sizeof( xQUEUE ) );
        if( pxNewQueue != NULL )
        {
            xQueueSizeInBytes = ( size_t ) ( uxQueueLength * uxItemSize ) + ( size_t ) 1;
            pxNewQueue->pcHead = ( signed char * ) pvPortMalloc( xQueueSizeInBytes );
            if( pxNewQueue->pcHead != NULL )
            {
                pxNewQueue->uxLength = uxQueueLength;
                pxNewQueue->uxItemSize = uxItemSize;
                xQueueGenericReset( pxNewQueue, pdTRUE );
#if ( configUSE_TRACE_FACILITY == 1 )
                pxNewQueue->ucQueueType = ucQueueType;
#endif
                traceQUEUE_CREATE( pxNewQueue );
                xReturn = pxNewQueue;
            }
            else
            {
                traceQUEUE_CREATE_FAILED( ucQueueType );
                vPortFree( pxNewQueue );
            }
        }
    }
    configASSERT( xReturn );
    return xReturn;
}

signed portBASE_TYPE xQueueGenericSend( xQueueHandle pxQueue, const void * const pvItemToQueue, portTickType xTicksToWait, portBASE_TYPE xCopyPosition )
{
    signed portBASE_TYPE xEntryTimeSet = pdFALSE;
    xTimeOutType xTimeOut;
    configASSERT( pxQueue );
    configASSERT( !( ( pvItemToQueue == NULL ) && ( pxQueue->uxItemSize != ( unsigned portBASE_TYPE ) 0U ) ) );
    for( ;; )
    {
        taskENTER_CRITICAL();
        {
            if( pxQueue->uxMessagesWaiting < pxQueue->uxLength )
            {
                traceQUEUE_SEND( pxQueue );
                prvCopyDataToQueue( pxQueue, pvItemToQueue, xCopyPosition );
                if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
                {
                    if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) == pdTRUE )
                    {
                        portYIELD_WITHIN_API();
                    }
                }
                taskEXIT_CRITICAL();
                return pdPASS;
            }
            else
            {
                if( xTicksToWait == ( portTickType ) 0 )
                {
                    taskEXIT_CRITICAL();
                    traceQUEUE_SEND_FAILED( pxQueue );
                    return errQUEUE_FULL;
                }
                else if( xEntryTimeSet == pdFALSE )
                {
                    vTaskSetTimeOutState( &xTimeOut );
                    xEntryTimeSet = pdTRUE;
                }
            }
        }
        taskEXIT_CRITICAL();
        vTaskSuspendAll();
        prvLockQueue( pxQueue );
        if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
        {
            if( prvIsQueueFull( pxQueue ) != pdFALSE )
            {
                traceBLOCKING_ON_QUEUE_SEND( pxQueue );
                vTaskPlaceOnEventList( &( pxQueue->xTasksWaitingToSend ), xTicksToWait );
                prvUnlockQueue( pxQueue );
                if( xTaskResumeAll() == pdFALSE )
                {
                    portYIELD_WITHIN_API();
                }
            }
            else
            {
                prvUnlockQueue( pxQueue );
                ( void ) xTaskResumeAll();
            }
        }
        else
        {
            prvUnlockQueue( pxQueue );
            ( void ) xTaskResumeAll();
            traceQUEUE_SEND_FAILED( pxQueue );
            return errQUEUE_FULL;
        }
    }
}

signed portBASE_TYPE xQueueGenericReceive( xQueueHandle pxQueue, void * const pvBuffer, portTickType xTicksToWait, portBASE_TYPE xJustPeeking )
{
    // ... 迁移原有实现 ...
}

unsigned portBASE_TYPE uxQueueMessagesWaiting( const xQueueHandle pxQueue )
{
    unsigned portBASE_TYPE uxReturn;
    configASSERT( pxQueue );
    taskENTER_CRITICAL();
    uxReturn = pxQueue->uxMessagesWaiting;
    taskEXIT_CRITICAL();
    return uxReturn;
}

void vQueueDelete( xQueueHandle pxQueue )
{
    configASSERT( pxQueue );
    traceQUEUE_DELETE( pxQueue );
    vQueueUnregisterQueue( pxQueue );
    vPortFree( pxQueue->pcHead );
    vPortFree( pxQueue );
} 
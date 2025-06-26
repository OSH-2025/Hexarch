#include "queue.h"
#include "list.h"
#include "task.h"

signed portBASE_TYPE xQueueGenericSendFromISR( xQueueHandle pxQueue, const void * const pvItemToQueue, signed portBASE_TYPE *pxHigherPriorityTaskWoken, portBASE_TYPE xCopyPosition ) PRIVILEGED_FUNCTION;
signed portBASE_TYPE xQueueReceiveFromISR( xQueueHandle pxQueue, void * const pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken ) PRIVILEGED_FUNCTION;
signed portBASE_TYPE xQueueIsQueueEmptyFromISR( const xQueueHandle pxQueue ) PRIVILEGED_FUNCTION;
signed portBASE_TYPE xQueueIsQueueFullFromISR( const xQueueHandle pxQueue ) PRIVILEGED_FUNCTION;
unsigned portBASE_TYPE uxQueueMessagesWaitingFromISR( const xQueueHandle pxQueue ) PRIVILEGED_FUNCTION;

signed portBASE_TYPE xQueueGenericSendFromISR( xQueueHandle pxQueue, const void * const pvItemToQueue, signed portBASE_TYPE *pxHigherPriorityTaskWoken, portBASE_TYPE xCopyPosition )
{
signed portBASE_TYPE xReturn;
unsigned portBASE_TYPE uxSavedInterruptStatus;

	configASSERT( pxQueue );
	configASSERT( !( ( pvItemToQueue == NULL ) && ( pxQueue->uxItemSize != ( unsigned portBASE_TYPE ) 0U ) ) );

	/* Similar to xQueueGenericSend, except we don't block if there is no room
	in the queue.  Also we don't directly wake a task that was blocked on a
	queue read, instead we return a flag to say whether a context switch is
	required or not (i.e. has a task with a higher priority than us been woken
	by this	post). */
	uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
	{
		if( pxQueue->uxMessagesWaiting < pxQueue->uxLength )
		{
			traceQUEUE_SEND_FROM_ISR( pxQueue );

			prvCopyDataToQueue( pxQueue, pvItemToQueue, xCopyPosition );

			/* If the queue is locked we do not alter the event list.  This will
			be done when the queue is unlocked later. */
			if( pxQueue->xTxLock == queueUNLOCKED )
			{
				if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToReceive ) ) == pdFALSE )
				{
					if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToReceive ) ) != pdFALSE )
					{
						/* The task waiting has a higher priority so record that a
						context	switch is required. */
						if( pxHigherPriorityTaskWoken != NULL )
						{
							*pxHigherPriorityTaskWoken = pdTRUE;
						}
					}
				}
			}
			else
			{
				/* Increment the lock count so the task that unlocks the queue
				knows that data was posted while it was locked. */
				++( pxQueue->xTxLock );
			}

			xReturn = pdPASS;
		}
		else
		{
			traceQUEUE_SEND_FROM_ISR_FAILED( pxQueue );
			xReturn = errQUEUE_FULL;
		}
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xQueueReceiveFromISR( xQueueHandle pxQueue, void * const pvBuffer, signed portBASE_TYPE *pxHigherPriorityTaskWoken )
{
signed portBASE_TYPE xReturn;
unsigned portBASE_TYPE uxSavedInterruptStatus;

	configASSERT( pxQueue );
	configASSERT( !( ( pvBuffer == NULL ) && ( pxQueue->uxItemSize != ( unsigned portBASE_TYPE ) 0U ) ) );

	uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
	{
		/* We cannot block from an ISR, so check there is data available. */
		if( pxQueue->uxMessagesWaiting > ( unsigned portBASE_TYPE ) 0 )
		{
			traceQUEUE_RECEIVE_FROM_ISR( pxQueue );

			prvCopyDataFromQueue( pxQueue, pvBuffer );
			--( pxQueue->uxMessagesWaiting );

			/* If the queue is locked we will not modify the event list.  Instead
			we update the lock count so the task that unlocks the queue will know
			that an ISR has removed data while the queue was locked. */
			if( pxQueue->xRxLock == queueUNLOCKED )
			{
				if( listLIST_IS_EMPTY( &( pxQueue->xTasksWaitingToSend ) ) == pdFALSE )
				{
					if( xTaskRemoveFromEventList( &( pxQueue->xTasksWaitingToSend ) ) != pdFALSE )
					{
						/* The task waiting has a higher priority than us so
						force a context switch. */
						if( pxHigherPriorityTaskWoken != NULL )
						{
							*pxHigherPriorityTaskWoken = pdTRUE;
						}
					}
				}
			}
			else
			{
				/* Increment the lock count so the task that unlocks the queue
				knows that data was removed while it was locked. */
				++( pxQueue->xRxLock );
			}

			xReturn = pdPASS;
		}
		else
		{
			xReturn = pdFAIL;
			traceQUEUE_RECEIVE_FROM_ISR_FAILED( pxQueue );
		}
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xQueueIsQueueEmptyFromISR( const xQueueHandle pxQueue )
{
signed portBASE_TYPE xReturn;

	configASSERT( pxQueue );
	xReturn = ( pxQueue->uxMessagesWaiting == ( unsigned portBASE_TYPE ) 0 );

	return xReturn;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xQueueIsQueueFullFromISR( const xQueueHandle pxQueue )
{
signed portBASE_TYPE xReturn;

	configASSERT( pxQueue );
	xReturn = ( pxQueue->uxMessagesWaiting == pxQueue->uxLength );

	return xReturn;
}
/*-----------------------------------------------------------*/

unsigned portBASE_TYPE uxQueueMessagesWaitingFromISR( const xQueueHandle pxQueue )
{
unsigned portBASE_TYPE uxReturn;

	configASSERT( pxQueue );

	uxReturn = pxQueue->uxMessagesWaiting;

	return uxReturn;
}
/*-----------------------------------------------------------*/
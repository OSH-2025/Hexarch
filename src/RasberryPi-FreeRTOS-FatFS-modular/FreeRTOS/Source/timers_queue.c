#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "timers_private.c" // 若有必要可调整为头文件

#if ( configUSE_TIMERS == 1 )

portBASE_TYPE xTimerGenericCommand( xTimerHandle xTimer, portBASE_TYPE xCommandID, portTickType xOptionalValue, signed portBASE_TYPE *pxHigherPriorityTaskWoken, portTickType xBlockTime )
{
    portBASE_TYPE xReturn = pdFAIL;
    xTIMER_MESSAGE xMessage;
    if( xTimerQueue != NULL )
    {
        xMessage.xMessageID = xCommandID;
        xMessage.xMessageValue = xOptionalValue;
        xMessage.pxTimer = ( xTIMER * ) xTimer;
        if( pxHigherPriorityTaskWoken == NULL )
        {
            if( xTaskGetSchedulerState() == taskSCHEDULER_RUNNING )
            {
                xReturn = xQueueSendToBack( xTimerQueue, &xMessage, xBlockTime );
            }
            else
            {
                xReturn = xQueueSendToBack( xTimerQueue, &xMessage, tmrNO_DELAY );
            }
        }
        else
        {
            xReturn = xQueueSendToBackFromISR( xTimerQueue, &xMessage, pxHigherPriorityTaskWoken );
        }
        traceTIMER_COMMAND_SEND( xTimer, xCommandID, xOptionalValue, xReturn );
    }
    return xReturn;
}

#endif /* configUSE_TIMERS == 1 */ 
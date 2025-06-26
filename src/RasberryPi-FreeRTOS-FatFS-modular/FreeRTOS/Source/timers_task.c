#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "timers_private.c" // 若有必要可调整为头文件

#if ( configUSE_TIMERS == 1 )

extern xQueueHandle xTimerQueue;
#if ( INCLUDE_xTimerGetTimerDaemonTaskHandle == 1 )
extern xTaskHandle xTimerTaskHandle;
#endif

portBASE_TYPE xTimerCreateTimerTask( void )
{
    portBASE_TYPE xReturn = pdFAIL;
    prvCheckForValidListAndQueue();
    if( xTimerQueue != NULL )
    {
#if ( INCLUDE_xTimerGetTimerDaemonTaskHandle == 1 )
        if( xTimerTaskHandle == NULL )
#endif
        {
            xReturn = xTaskCreate( prvTimerTask, ( const signed char * ) "Tmr Svc", configTIMER_TASK_STACK_DEPTH, NULL, configTIMER_TASK_PRIORITY, &xTimerTaskHandle );
        }
    }
    configASSERT( xReturn );
    return xReturn;
}

#if ( INCLUDE_xTimerGetTimerDaemonTaskHandle == 1 )
xTaskHandle xTimerGetTimerDaemonTaskHandle( void )
{
    configASSERT( ( xTimerTaskHandle != NULL ) );
    return xTimerTaskHandle;
}
#endif

#endif /* configUSE_TIMERS == 1 */ 
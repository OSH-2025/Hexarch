#include "FreeRTOS.h"
#include "task.h"
#include "StackMacros.h"

// vTaskDelayUntil迁移
void vTaskDelayUntil( portTickType * const pxPreviousWakeTime, portTickType xTimeIncrement )
{
    portTickType xTimeToWake;
    portBASE_TYPE xAlreadyYielded, xShouldDelay = pdFALSE;

    configASSERT( pxPreviousWakeTime );
    configASSERT( ( xTimeIncrement > 0U ) );

    vTaskSuspendAll();
    {
        xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

        if( xTickCount < *pxPreviousWakeTime )
        {
            if( ( xTimeToWake < *pxPreviousWakeTime ) && ( xTimeToWake > xTickCount ) )
            {
                xShouldDelay = pdTRUE;
            }
        }
        else
        {
            if( ( xTimeToWake < *pxPreviousWakeTime ) || ( xTimeToWake > xTickCount ) )
            {
                xShouldDelay = pdTRUE;
            }
        }

        *pxPreviousWakeTime = xTimeToWake;

        if( xShouldDelay != pdFALSE )
        {
            traceTASK_DELAY_UNTIL();
            vListRemove( ( xListItem * ) &( pxCurrentTCB->xGenericListItem ) );
            prvAddCurrentTaskToDelayedList( xTimeToWake );
        }
    }
    xAlreadyYielded = xTaskResumeAll();

    if( xAlreadyYielded == pdFALSE )
    {
        portYIELD_WITHIN_API();
    }
}

// vTaskDelay迁移
void vTaskDelay( portTickType xTicksToDelay )
{
    portTickType xTimeToWake;
    signed portBASE_TYPE xAlreadyYielded = pdFALSE;

    if( xTicksToDelay > ( portTickType ) 0U )
    {
        vTaskSuspendAll();
        {
            traceTASK_DELAY();
            xTimeToWake = xTickCount + xTicksToDelay;
            vListRemove( ( xListItem * ) &( pxCurrentTCB->xGenericListItem ) );
            prvAddCurrentTaskToDelayedList( xTimeToWake );
        }
        xAlreadyYielded = xTaskResumeAll();
    }

    if( xAlreadyYielded == pdFALSE )
    {
        portYIELD_WITHIN_API();
    }
} 